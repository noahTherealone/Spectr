#pragma once

#include "AudioTypes.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <iostream>

struct PlaybackEvent {
    std::shared_ptr<Oscillator> osc;
    Signal signal;
    bool open = true;
    double onset, duration;

    std::optional<std::string> label;

    PlaybackEvent(std::shared_ptr<Oscillator> o, const Signal& sig, double ons, const std::string* l)
        : osc(std::move(o)), signal(sig), onset(ons), label(l ? std::optional<std::string>(*l) : std::nullopt) { }
    
    bool isActive(double globalTime) {
        return globalTime >= onset && (open || globalTime < duration);
    }

    bool isExpired(double globalTime) {
        return !open && globalTime >= onset + duration;
    }

    void release(double globalTime) {
        if (!open) {
            std::cout << "Warning: Tried releasing already closed event" << std::endl;
            return;
        }

        // std::cout << std::to_string(onset + (globalTime - onset)) + ", " + std::to_string(globalTime) + "\n";
        duration = globalTime - onset; // check for non-negative duration later
        open = false;

        // and maybe create and return a trail later
    }

    float sampleAt(double globalTime) const {
        double localTime = globalTime - onset;
        if (localTime < 0.0 || (!open && localTime > duration)) return 0.0f; // inactive
        return osc->wave(localTime, signal);
    }
};

class PlaybackEventStream {
public:
    // push one event
    void push(const PlaybackEvent& ev/*, const std::string* label = nullptr*/);

    // push many events at once
    template<class It>
    void pushAll(It begin, It end);

    void release(const std::string* label, double globalTime);

    /*
    // blocking pop (for e.g. dedicated render thread)
    PlaybackEvent waitAndPop();

    // non-blocking pop (for audio callback)
    std::optional<PlaybackEvent> tryPop();
    */

    std::vector<PlaybackEvent*> getActiveEvents(double timeNow);

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events.empty();
    }

    std::vector<std::unique_ptr<PlaybackEvent>> getAllEvents() {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::move(events);
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::unique_ptr<PlaybackEvent>> events;
    //std::unordered_map<std::string, PlaybackEvent*> labelMap;
};