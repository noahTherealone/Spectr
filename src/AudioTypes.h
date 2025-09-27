#pragma once

#include <complex>
#include <vector>
#include <numbers>
#include <iostream> // only for testing, remove later!
#include <memory>

struct Signal {
    float freq = 0.0f;
    std::complex<float> amp = { 1.0f, 0.0f };

    static const Signal empty;
    static const Signal unit;

    inline std::string to_string() const {
        return std::to_string(freq) + ":" + std::to_string(std::abs(amp));
    }

    Signal() = default;
    Signal(float f, const std::complex<float>& a = std::complex(1.0f, 0.0f))
        : freq(f), amp(a) { }
};

inline Signal operator*(float a, const Signal& b) {
    return Signal(a * b.freq, b.amp);
}

inline Signal operator*(const Signal& a, float b) {
    return Signal(a.freq, a.amp * b);
}

inline Signal operator*(const Signal& a, const Signal& b) {
    return Signal(a.freq * b.freq, a.amp * b.amp);
}

inline Signal operator/(const Signal& a, float b) {
    return Signal(a.freq, a.amp / b);
}

struct Spectrum : public std::vector<Signal> {
    using std::vector<Signal>::vector; // inherit constructors

    static const Spectrum empty;
    static const Spectrum unit;

    inline std::string to_string() const {
        std::string str = "{ ";
        for (Signal s : *this) {
            str.append(s.to_string() + " ");
        }

        str.append("}");
        return str;
    }
};

inline Spectrum operator+(const Spectrum& a, const Spectrum& b) {
    Spectrum result = a;
    result.insert(result.end(), b.begin(), b.end());
    // possibly decide to merge or decide not to merge later
    return result;
}

inline Spectrum operator*(float a, const Spectrum& spec) {
    Spectrum result = spec;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = a * result[i];
    
    return result;
}

inline Spectrum operator*(const Spectrum& spec, float b) {
    Spectrum result = spec;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = result[i] * b;
    
    return result;
}

inline Spectrum operator*(const Signal& sig, const Spectrum& spec) {
    Spectrum result = spec;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = sig * result[i];
    
    return result;
}

inline Spectrum operator*(const Spectrum& spec, const Signal& sig) {
    return sig * spec;
}

inline Spectrum operator*(const Spectrum& a, const Spectrum& b) {
    Spectrum result = Spectrum();
    for (size_t i = 0; i < a.size(); ++i) {
        Spectrum product = a[i] * b;
        result.insert(result.end(), product.begin(), product.end());
    }

    return result;
}

inline Spectrum operator+(float a, const Signal& b) {
    return Spectrum{ Signal(a, 1.0), b };
}

inline Spectrum operator+(const Signal& a, float b) {
    return Spectrum{ a, Signal(b, 1.0) };
}

inline Spectrum operator+(const Signal& a, const Signal& b) {
    return Spectrum{ a, b };
}

inline Spectrum operator+(const Signal& a, const Spectrum& b) {
    Spectrum result = b;
    result.insert(result.end(), a);
    return result;
}

inline Spectrum operator+(const Spectrum& a, const Signal& b) {
    Spectrum result = a;
    result.push_back(b);
    return result;
}

inline Spectrum operator/(const Spectrum& a, float b) {
    Spectrum result = a;
    for (Signal sig : result)
        sig = sig / b;
    
    return result;
}

enum class OscPrim {
    Sine,
    Square,
    Saw,
};

struct Oscillator {
    Signal reference; // nominal freq/amp of this oscillator
    virtual float wave(double time, const Signal& factor) const = 0;
    virtual size_t sampleNumber() const = 0;
    virtual std::string to_string() const = 0;
    virtual std::unique_ptr<Oscillator> clone() const = 0;
    virtual ~Oscillator() { }
};

const size_t WAVETABLE_LENGTH = 2048;

struct WavetableOsc : public Oscillator {
    enum class InterpMode {
        None,
        Linear,
        Quadratic
    };

    std::vector<float> table;
    InterpMode interp = InterpMode::Linear;

    float wave(double time, const Signal& factor) const override;
    size_t sampleNumber() const override;
    std::string to_string() const override;
    std::unique_ptr<Oscillator> clone() const override;

    WavetableOsc(Signal sig, OscPrim shape, int sampleRate) {
        reference = sig;
        float amp = std::abs(sig.amp);
        if (shape == OscPrim::Sine) {
            for (size_t i = 0; i < WAVETABLE_LENGTH; ++i) {
                table.push_back(amp * sin(2 * 3.1415926535897932384626433 * (double)i / WAVETABLE_LENGTH));
            }
        }
        else if (shape == OscPrim::Square) {
            size_t i = 0;
            for (; i+i < WAVETABLE_LENGTH; ++i) {
                table.push_back(amp);
            }
            for (; i < WAVETABLE_LENGTH; ++i) {
                table.push_back(-amp);
            }
        }
        else if (shape == OscPrim::Saw) {
            for (size_t i = 0; i < WAVETABLE_LENGTH; ++i) {
                table.push_back(amp * (1.0 - 2 * (double)i / WAVETABLE_LENGTH));
            }
        }
    }
};

struct CompoundOsc : public Oscillator {
    std::vector<WavetableOsc> partials;

    float wave(double time, const Signal& factor) const override;

    inline void addPartial(const WavetableOsc partial) {
        partials.push_back(partial);
    }

    size_t sampleNumber() const override;
    std::string to_string() const override;
    std::unique_ptr<Oscillator> clone() const override;

    CompoundOsc() = default;

    CompoundOsc(Spectrum spectrum, OscPrim shape, int sampleRate) {
        if (spectrum.size() == 0) throw std::runtime_error("Tried creating compound oscillator from empty spectrum.");

        reference = Signal(spectrum[0].freq, 0);
        for (Signal signal : spectrum) {
            partials.push_back(WavetableOsc(signal, shape, sampleRate));

            if (reference.freq < signal.freq)
                reference.freq = signal.freq;

            reference.amp += signal.amp; // add more sophisticated amplitude calculation later in tandem with addition overloads
        }
    }

    // that used to fix something, but then it broke something, now we dont use it
    // but in case anything breaks, try bringing it back maybe
    //CompoundOsc(const CompoundOsc&) = delete;
    //CompoundOsc& operator=(const CompoundOsc&) = delete;

    //CompoundOsc(CompoundOsc&&) noexcept = default;
    //CompoundOsc& operator=(CompoundOsc&&) noexcept = default;
};

static inline CompoundOsc operator+(const WavetableOsc& a, const WavetableOsc& b) {
    CompoundOsc osc;
    osc.partials = { a, b };

    if (a.reference.freq <= b.reference.freq)
        osc.reference = a.reference;
    else
        osc.reference = b.reference;
    
    // add more sophisticated amp calculation later (and maybe do so in a helper function for addition operators)
    osc.reference.amp = a.reference.amp + b.reference.amp;

    return osc;
}

static inline CompoundOsc operator+(const CompoundOsc& a, const WavetableOsc& b) {
    CompoundOsc osc;
    for (int i = 0; i < a.partials.size(); ++i) {
        osc.partials.push_back(a.partials[i]);
    }
    osc.partials.push_back(b);

    if (a.reference.freq <= b.reference.freq)
        osc.reference = a.reference;
    else
        osc.reference = b.reference;
    
    // add more sophisticated amp calculation later (and maybe do so in a helper function for addition operators)
    osc.reference.amp = a.reference.amp + b.reference.amp;

    return osc;
}

static inline CompoundOsc operator+(const WavetableOsc& a, const CompoundOsc& b) {
    CompoundOsc osc;
    osc.partials.push_back(a);
    for (int i = 0; i < b.partials.size(); ++i) {
        osc.partials.push_back(b.partials[i]);
    }

    if (a.reference.freq <= b.reference.freq)
        osc.reference = a.reference;
    else
        osc.reference = b.reference;
    
    // add more sophisticated amp calculation later (and maybe do so in a helper function for addition operators)
    osc.reference.amp = a.reference.amp + b.reference.amp;

    return osc;
}

static inline CompoundOsc operator+(const CompoundOsc& a, const CompoundOsc& b) {
    CompoundOsc osc;
    for (int i = 0; i < a.partials.size(); ++i) {
        osc.partials.push_back(a.partials[i]);
    }
    for (int i = 0; i < b.partials.size(); ++i) {
        osc.partials.push_back(b.partials[i]);
    }

    if (a.reference.freq <= b.reference.freq)
        osc.reference = a.reference;
    else
        osc.reference = b.reference;
    
    // add more sophisticated amp calculation later (and maybe do so in a helper function for addition operators)
    osc.reference.amp = a.reference.amp + b.reference.amp;

    return osc;
}