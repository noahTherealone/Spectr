#include "PlaybackEvents.h"

void PlaybackEventStream::push(const PlaybackEvent& e/*, const std::string* label*/) {
    std::lock_guard<std::mutex> lock(mutex_);

    events.push_back(std::make_unique<PlaybackEvent>(e));
    /*if (label) {
        //if (labelMap.contains(*label))
        //    release(label, e.onset);

        labelMap.emplace(*label, events.back().get());
        std::cout << "Labels:";
        for (const auto [key, value] : labelMap) {
            std::cout << " " + key;
        }
        std::cout << std::endl;
    }*/
    std::cout << "Pushed an event. Now there are: " + std::to_string(events.size()) + ".\n";
    cv_.notify_one();
}

/*template<class It>
void PlaybackEventStream::pushAll(It begin, It end) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = begin; it != end; ++it) {
        events.push_back(*it);
    }
    cv_.notify_one();
}*/

void PlaybackEventStream::release(const std::string* label, double globalTime) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!label) {
        events.back()->release(globalTime);
        std::cout << "Released last of " + std::to_string(events.size()) + " events.\n";
        return;
    }
    
    size_t deleted = 0;
    for (size_t i = 0; i < events.size();) {
        if (events[i]->label && *events[i]->label == *label) {
            events.erase(events.begin() + i);
            deleted++;
            continue;
        }

        i++;
    }

    /*if (labelMap.contains(*label)) {
        labelMap[*label]->release(globalTime);
        labelMap.erase(*label);
        std::cout << "Released '" + *label + ".\n";
        return;
    }*/

    std::cout << "Released " + std::to_string(deleted) + " label" + (deleted != 1 ? "s" : "") + " '" + *label + ".\n";
}

/*PlaybackEvent PlaybackEventStream::waitAndPop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&]{ return !events.empty(); });
    auto ev = events.front();
    events.erase(events.begin());
    return ev;
}

std::optional<PlaybackEvent> PlaybackEventStream::tryPop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (events.empty()) return std::nullopt;
    auto ev = events.front();
    events.erase(events.begin());
    return ev;
}*/

std::vector<PlaybackEvent*> PlaybackEventStream::getActiveEvents(double timeNow) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PlaybackEvent*> active;

    auto it = events.begin();
    while (it != events.end()) {
        if (it->get()->isActive(timeNow)) {
            active.push_back(it->get());  // copy (safe for audio thread)
            ++it;
        } else if (it->get()->isExpired(timeNow)) {
            /*for (auto lit = labelMap.begin(); lit != labelMap.end(); lit++) {
                if (lit->second == it->get()) {
                    labelMap.erase(lit->first);
                    break;
                }
            }*/

            // this event is finished, erase it
            it = events.erase(it);
        } else {
            ++it; // not yet started
        }
    }

    return active;
}