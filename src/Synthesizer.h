#pragma once

#include <memory>
#include "PlaybackEvents.h"
#include "AudioTypes.h"

void writeWav(const std::string& fileName, const std::vector<float> samples, int sampleRate);

std::vector<float> render(const std::vector<PlaybackEvent>& events, int sampleRate);