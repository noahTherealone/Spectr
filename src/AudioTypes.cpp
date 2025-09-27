#include "AudioTypes.h"

#include <iostream>

const Signal Signal::empty{ 1.0f, { 0.0f, 0.0f } };
const Signal Signal::unit{ 1.0f, { 1.0f, 0.0f } };

const Spectrum Spectrum::empty{ };
const Spectrum Spectrum::unit{ Signal::unit };

float WavetableOsc::wave(double time, const Signal& factor) const {
    double phase = time * factor.freq;
    phase = phase - std::floor(phase);
    double pos = phase * table.size();
    int i = static_cast<int>(pos);
    double frac = pos - i;
    switch (interp) {
        case InterpMode::None:
            return table[i % table.size()];
        case InterpMode::Linear: {
            float s0 = table[ i    % table.size()];
            float s1 = table[(i+1) % table.size()];
            return ((1.0 - frac) * s0 + frac * s1) * (std::abs(factor.amp) / std::abs(reference.amp));
        }
        case InterpMode::Quadratic: {
            float s0 = table[ i    % table.size()];
            float s1 = table[(i+1) % table.size()];
            float s2 = table[(i+2) % table.size()];
            return (s0 + frac * (s1 - s0 + 0.5f*(frac-1)*(s2 + s0 - 2*s1))) * (std::abs(factor.amp) / std::abs(reference.amp));
        }
    }
}

size_t WavetableOsc::sampleNumber() const {
    return table.size();
}

std::string WavetableOsc::to_string() const {
    return "Wavetable oscillator (" + std::to_string(sampleNumber()) + " samples)";
}

std::unique_ptr<Oscillator> WavetableOsc::clone() const {
    return std::make_unique<WavetableOsc>(*this);
}

float CompoundOsc::wave(double time, const Signal& factor) const {
    float sum = 0.f;
    for (auto osc : partials) {
        sum += osc.wave(time, factor);
    }
    return sum;
}

size_t CompoundOsc::sampleNumber() const {
    size_t num = 0;
    for (auto osc : partials) {
        num += osc.sampleNumber();
    }
    return num;
}

std::string CompoundOsc::to_string() const {
    return "Compound oscillator (" + std::to_string(sampleNumber()) + " samples, "
        + std::to_string(partials.size()) + " partials)";
}

std::unique_ptr<Oscillator> CompoundOsc::clone() const {
    return std::make_unique<CompoundOsc>(*this);
}