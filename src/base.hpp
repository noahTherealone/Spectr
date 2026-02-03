#pragma once

#include <string>
#include <vector>
#include <algorithm>

struct SpectrError;

std::string sourcePos(const std::string& path, const std::vector<size_t>& offsets, size_t index);