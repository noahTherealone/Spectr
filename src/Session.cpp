#define MINIAUDIO_IMPLEMENTATION

#include "PlaybackEvents.h"
#include "Parser.h"
#include "Interpreter.hpp"
#include "Session.h"

#include <vector>
#include <memory>
#include <mutex>
#include <iostream>

// Your PlaybackEventStream, thread-safe queue of events
PlaybackEventStream gEventStream;
std::mutex gMutex;

ma_device device;
RtMidiIn *midiin;
Parser parser;

void midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData) {
    if (message->size() < 3) return;

    unsigned char status = message->at(0);
    unsigned char data1  = message->at(1); // note number or controller
    unsigned char data2  = message->at(2); // velocity or value

    int channel = status & 0x0F;
    int type    = status & 0xF0;

    std::string spectrCode;

    if (type == 0x90 && data2 > 0) { 
        // Note on

        float freq = 440 * exp2f((float)((int)data1 - 69) / 12.0f);

        spectrCode = "'midiC" + std::to_string(channel) + "N" + std::to_string((int)data1) +
                     " play timbre " + std::to_string(freq) + ":0.2"/* + std::to_string((int)data2)*/;
        
        std::cout << "Note on " + std::to_string((int)data1) + ": " + spectrCode + "\n";
    } else if (type == 0x80 || (type == 0x90 && data2 == 0)) { 
        // Note off

        spectrCode = "release 'midiC" + std::to_string(channel) + "N" + std::to_string((int)data1);

        std::cout << "Note off " + std::to_string((int)data1) + ": " + spectrCode + "\n";
    }

    if (!spectrCode.empty()) {
        // std::lock_guard<std::mutex> lock(gMutex);
        parser.parseCode(spectrCode);
    }

}

void setupMidiIn() {
#ifdef _WIN32
    midiin = new RtMidiIn(RtMidi::WINDOWS_MM);
#else
    midiin = new RtMidiIn(RtMidi::LINUX_ALSA);
#endif

    size_t nPorts = midiin->getPortCount();
    std::cout << "Found " << nPorts << " MIDI input ports\n";

    for (size_t i = 0; i < nPorts; i++) {
        if (((std::string)midiin->getPortName(i)).starts_with("Midi Through"))
            continue;

        midiin->openPort(i);
        midiin->setCallback(&midiCallback);
        midiin->ignoreTypes(false, false, false); // don't ignore sysex, timing, active sense
        std::cout << "Opened port " << i << ": " << midiin->getPortName(i) << '\n';

        return;
    }

    if (nPorts == 0) {
        std::cout << "No MIDI input ports available!" << std::endl;
    }
}

double gStreamTime = 0.0;
double gSampleRate = 44100.0;

int activeEvents;

// Audio callback
void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
    float* out = (float*)output;
    (void)input; // unused

    // std::lock_guard<std::mutex> lock(gMutex);

    for (uint32_t i = 0; i < frameCount; i++) {
        float sample = 0.0f;
        double t = gStreamTime + i / gSampleRate;

        auto active = gEventStream.getActiveEvents(t);
        activeEvents = active.size();

        for (auto& ev : active) {
            float s = ev->sampleAt(t);
            sample += s;
        }

        out[i*2 + 0] = sample;
        out[i*2 + 1] = sample;
    }

    gStreamTime += (double)frameCount / gSampleRate;
}

int initSession(LogSettings* logSettings, const std::string& exeDir, const std::string* setupPath) {
    Interpreter backend = Interpreter(&gEventStream, logSettings, &gStreamTime);
    parser = Parser(&backend, logSettings);

    bool hidAll = logSettings->hideAll;
    logSettings->hideAll = true;
    parser.parseFile(exeDir + "/std/private/session_init.spectr");
    logSettings->hideAll = hidAll;

    if (setupPath) {
        std::cout << "---- SETUP ----" << std::endl;
        parser.parseFile(*setupPath);
    }

    setupMidiIn();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = 44100;
    config.dataCallback      = dataCallback;

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return -1;
    }

    ma_device_start(&device);

    std::cout << "\033[1;35mInput>\033[0m" << std::endl;

    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.starts_with("\\quit")) break;
        else if (line.starts_with("\\active")) {
            std::cout << std::to_string(activeEvents) << std::endl;
            continue;
        }
        else if (line.starts_with("\\midi")) {
            setupMidiIn();
            continue;
        }
        else if (line.starts_with("\\run ")) {
            std::string path = line.substr(5);
            parser.parseFile(path);
        }

        // std::lock_guard<std::mutex> lock(gMutex);
        parser.parseCode(line); // sends parsed code to backend pushes events to the stream

        std::cout << "\033[1;35mInput>\033[0m" << std::endl;
    }

    ma_device_uninit(&device);
    return 0;
}