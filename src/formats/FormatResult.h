#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace edi::formats {

struct FormatMessage {
    std::string source;
    std::string code;
    std::string message;
};

using FormatError = FormatMessage;
using FormatWarning = FormatMessage;

template <typename T>
struct FormatResult {
    bool ok = false;
    std::optional<T> value;
    std::vector<FormatWarning> warnings;
    std::vector<FormatError> errors;

    static FormatResult<T> success(T value)
    {
        FormatResult<T> result;
        result.ok = true;
        result.value = std::move(value);
        return result;
    }

    static FormatResult<T> failure(std::string source, std::string code, std::string message)
    {
        FormatResult<T> result;
        result.ok = false;
        result.errors.push_back({std::move(source), std::move(code), std::move(message)});
        return result;
    }
};

} // namespace edi::formats
