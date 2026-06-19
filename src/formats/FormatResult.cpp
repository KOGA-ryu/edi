#include "formats/FormatResult.h"

namespace edi::formats {

const char *formatResultCodeName(FormatResultCode code)
{
    switch (code) {
    case FormatResultCode::None:
        return "none";
    case FormatResultCode::EmptySource:
        return "empty_source";
    case FormatResultCode::SyntaxError:
        return "syntax_error";
    case FormatResultCode::EmptyKey:
        return "empty_key";
    case FormatResultCode::DuplicateKey:
        return "duplicate_key";
    case FormatResultCode::UnsupportedSchema:
        return "unsupported_schema";
    case FormatResultCode::MissingSchema:
        return "missing_schema";
    case FormatResultCode::UnsupportedVersion:
        return "unsupported_version";
    case FormatResultCode::EmptyBuffer:
        return "empty_buffer";
    case FormatResultCode::InvalidRecordCount:
        return "invalid_record_count";
    case FormatResultCode::IoError:
        return "io_error";
    }
    return "unknown";
}

FormatResultCode formatResultCodeFromName(const std::string &code)
{
    if (code == "empty_source") {
        return FormatResultCode::EmptySource;
    }
    if (code == "syntax_error" || code == "toml.syntax") {
        return FormatResultCode::SyntaxError;
    }
    if (code == "empty_key" || code == "toml.empty_key") {
        return FormatResultCode::EmptyKey;
    }
    if (code == "duplicate_key") {
        return FormatResultCode::DuplicateKey;
    }
    if (code == "unsupported_schema") {
        return FormatResultCode::UnsupportedSchema;
    }
    if (code == "missing_schema") {
        return FormatResultCode::MissingSchema;
    }
    if (code == "unsupported_version") {
        return FormatResultCode::UnsupportedVersion;
    }
    if (code == "empty_buffer" || code == "msgpack.empty") {
        return FormatResultCode::EmptyBuffer;
    }
    if (code == "invalid_record_count" || code == "msgpack.record_count_range") {
        return FormatResultCode::InvalidRecordCount;
    }
    if (code == "io_error") {
        return FormatResultCode::IoError;
    }
    return FormatResultCode::SyntaxError;
}

} // namespace edi::formats
