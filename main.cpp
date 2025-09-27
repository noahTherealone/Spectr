#include "LogSettings.h"
#include "Parser.h"
#include "Synthesizer.h"
#include "Session.h"
#include <iostream>
#include <string>
#ifdef _WIN32
    #include <windows.h>
#endif
#include <filesystem>

std::filesystem::path getExecutableDir() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#else
    char buf[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buf, PATH_MAX);
    return std::filesystem::path(std::string(buf, count)).parent_path();
#endif
}

enum class SpectrMode {
    SynthesizeFile,
    Session
};

int main(int argc, char** argv) {

    auto exeDir = getExecutableDir();

    SpectrMode mode = SpectrMode::Session;
    std::string path = "demo.spectr";

    int sampleRate = 44100;

    bool sessionSetup = false;
    
    std::string* setupPath = nullptr;

    LogSettings logSettings;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (sessionSetup) {
            if (arg.starts_with("--")) {
                std::cout << "Invalid setup path " << arg << std::endl;
                return 1;
            }

            if (arg.ends_with(".spectr"))
                setupPath = new std::string(arg);
            else {
                setupPath = new std::string((exeDir / "std" / (arg + ".spectr")).string());
            }
            
            sessionSetup = false;
            continue;
        }
        if (arg.ends_with(".spectr")) path = arg;
        else if (arg == "session") mode = SpectrMode::Session;
        else if (arg == "--setup") {
            if (mode != SpectrMode::Session) {
                std::cout << "--setup flag is only valid in session mode" << std::endl;
                return 1;
            }

            sessionSetup = true;
        }
        else if (arg == "--log-raw")    logSettings.logRaw = true;
        else if (arg == "--log-tokens") logSettings.logTokens = true;
        else if (arg == "--log-parsed") logSettings.logParsed = true;
        else if (arg == "--debug")      logSettings.setDebug();
        else std::cout << "\033[0;31mUnexpected flag " + arg + "\033[0m" << std::endl;
    }

    if (mode == SpectrMode::Session)
        return initSession(&logSettings, exeDir.string(), setupPath);

    std::cout << path << std::endl;

    PlaybackEventStream events;

    std::unique_ptr<_Interpreter> backend = std::make_unique<Interpreter>(&events, &logSettings);
    Parser parser(backend.get(), &logSettings);
    
    try {
        parser.parseFile(path);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    // I'm too dumb to move that vector of unique pointers right now, fix later!!!
    //
    //std::vector<std::unique_ptr<PlaybackEvent>> mEvents = std::move(events.getAllEvents());
    //
    //std::cout << "\033[1;32mResult>\033[0m" << std::endl
    //    << "Collected " << std::to_string(mEvents.size())
    //    << " playback event" << (mEvents.size() != 1 ? "s" : "") << "." << std::endl;
    //
    //auto buffer = render(std::move(mEvents), sampleRate);
    //
    //std::cout << "Wrote buffer (" << std::to_string(buffer.size()) << ")" << std::endl;
    //
    //writeWav("test.wav", buffer, sampleRate);

    return 0;
}