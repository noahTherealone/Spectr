#pragma once

#include "miniaudio.h"
#include "RtMidi.h"

void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount);

int initSession(const LogSettings& logSettings, const std::string& exeDir, const std::string* setupPath = nullptr);