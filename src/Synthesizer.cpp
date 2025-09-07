#include "Synthesizer.h"
#include <fstream>
#include <cstdint>
#include <iostream>

void writeWav(const std::string& filename, const std::vector<float> samples, int sampleRate) {
    int numSamples = samples.size();
    int byteRate = sampleRate * 2; // mono 16-bit
    int blockAlign = 2;

    std::ofstream out(filename, std::ios::binary);

    // RIFF header
    out.write("RIFF", 4);
    uint32_t chunkSize = 36 + numSamples * 2;
    out.write(reinterpret_cast<const char*>(&chunkSize), 4);
    out.write("WAVE", 4);

    // fmt subchunk
    out.write("fmt ", 4);
    uint32_t subChunk1Size = 16; // PCM
    uint16_t audioFormat = 1;    // PCM
    uint16_t numChannels = 1;    // mono
    out.write(reinterpret_cast<const char*>(&subChunk1Size), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    uint16_t bitsPerSample = 16;
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data subchunk
    out.write("data", 4);
    uint32_t dataSize = numSamples * 2;
    out.write(reinterpret_cast<const char*>(&dataSize), 4);

    // samples
    for (float s : samples) {
        int16_t val = static_cast<int16_t>(std::max(-1.f, std::min(1.f, s)) * 32767);
        out.write(reinterpret_cast<const char*>(&val), 2);
    }
}

std::vector<float> render(const std::vector<std::unique_ptr<PlaybackEvent>>& events, int sampleRate) {
    // Find max duration
    double maxTime = 0.0;
    for (auto& e : events)
        maxTime = std::max(maxTime, e->onset + e->duration);

    int totalSamples = static_cast<int>(maxTime * sampleRate);
    std::vector<float> buffer(totalSamples, 0.0f);

    // Render each event
    for (auto& e : events) {
        int startSample = static_cast<int>(e->onset * sampleRate);
        int numSamples  = static_cast<int>(e->duration * sampleRate);

        // phase accumulator
        double phase = 0.0;
        double phaseInc = e->signal.freq / sampleRate;

        for (int i = startSample; i < startSample + numSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            float sample = e->sampleAt(t);
            buffer[i] += sample;

            phase += phaseInc;
            if (phase >= 1.0) phase -= 1.0;
        }
    }

    return buffer;
}
