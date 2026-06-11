#pragma once

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <string>

namespace edi::recipe {

// Shared by every recipe text format (shaper recipes, op streams): numbers
// persist as the SHORTEST text that round-trips the double exactly, and
// parse strictly finite. One source of truth — two formats disagreeing on
// number text would be a quiet fork.

inline std::string numberKeyText(double value)
{
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

inline bool parseNumberText(const std::string &text, double &value)
{
    if (text.empty()) {
        return false;
    }
    char *end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end == text.c_str() + text.size() && std::isfinite(value);
}

inline bool parseIntText(const std::string &text, int &value)
{
    if (text.empty()) {
        return false;
    }
    char *end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end != text.c_str() + text.size()) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

} // namespace edi::recipe
