#include "formats/TomlWriter.h"

#include <cstdio>
#include <sstream>

namespace edi::formats {

namespace {

// TOML basic-string escaping. Without it a value containing a quote or
// backslash writes a file that tomllib (and our own reader) cannot read
// back — the writer must never produce an unreadable document. TOML
// forbids unescaped control characters AND DEL (0x7F); tomllib enforces
// both, so both are escaped.
std::string escapeBasicString(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\r':
            escaped += "\\r";
            break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20 || static_cast<unsigned char>(ch) == 0x7F) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04X", ch);
                escaped += buffer;
            } else {
                escaped += ch;
            }
        }
    }
    return escaped;
}

} // namespace

FormatResult<std::string> writeTomlStaticConfig(const StaticConfig &config, const std::string &)
{
    std::ostringstream output;
    for (const auto &[key, value] : config) {
        if (key.empty()) {
            return FormatResult<std::string>::failure({}, FormatResultCode::EmptyKey, "empty TOML key cannot be written");
        }
        output << key << " = \"" << escapeBasicString(value) << "\"\n";
    }
    return FormatResult<std::string>::success(output.str());
}

} // namespace edi::formats
