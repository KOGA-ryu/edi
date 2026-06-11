#pragma once

#include <charconv>
#include <cmath>
#include <string>
#include <system_error>

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

// from_chars, not strtod: strtod honors LC_NUMERIC, and QApplication sets
// the C locale from the environment on Unix — under a comma-decimal locale
// the loader would reject the very "1.5" the to_chars writer emits.
// from_chars is locale-independent by definition, the reader the writer
// deserves. One divergence handled: from_chars rejects the leading '+'
// strtod accepted (and TOML permits), so it is skipped explicitly.
inline bool parseNumberText(const std::string &text, double &value)
{
    if (text.empty()) {
        return false;
    }
    const char *first = text.data();
    const char *last = first + text.size();
    if (*first == '+') {
        ++first;
        if (first == last || *first == '-') {
            return false;
        }
    }
    const auto result = std::from_chars(first, last, value);
    return result.ec == std::errc{} && result.ptr == last && std::isfinite(value);
}

inline bool parseIntText(const std::string &text, int &value)
{
    if (text.empty()) {
        return false;
    }
    // from_chars reports out-of-range natively — strtol clamped on
    // overflow and the long->int cast then truncated, so a 12-digit
    // vertex count parsed "successfully" to garbage.
    const char *first = text.data();
    const char *last = first + text.size();
    if (*first == '+') {
        ++first;
        if (first == last || *first == '-') {
            return false;
        }
    }
    const auto result = std::from_chars(first, last, value, 10);
    return result.ec == std::errc{} && result.ptr == last;
}

} // namespace edi::recipe
