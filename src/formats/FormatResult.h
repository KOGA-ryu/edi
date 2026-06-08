#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace edi::formats {

enum class FormatResultCode {
    None,
    EmptySource,
    SyntaxError,
    EmptyKey,
    DuplicateKey,
    UnsupportedSchema,
    MissingSchema,
    UnsupportedVersion,
    EmptyBuffer,
    InvalidRecordCount
};

struct FormatMessage {
    std::string source;
    FormatResultCode code = FormatResultCode::None;
    std::string message;
};

using FormatError = FormatMessage;
using FormatWarning = FormatMessage;

const char *formatResultCodeName(FormatResultCode code);
FormatResultCode formatResultCodeFromName(const std::string &code);

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
        return failure(std::move(source), formatResultCodeFromName(code), std::move(message));
    }

    static FormatResult<T> failure(std::string source, FormatResultCode code, std::string message)
    {
        FormatResult<T> result;
        result.ok = false;
        result.errors.push_back({std::move(source), std::move(code), std::move(message)});
        return result;
    }
};

} // namespace edi::formats
