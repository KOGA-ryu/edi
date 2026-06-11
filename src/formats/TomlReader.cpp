#include "formats/TomlReader.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace edi::formats {

namespace {

std::string trim(std::string value)
{
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char ch) {
        return !isSpace(static_cast<unsigned char>(ch));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char ch) {
        return !isSpace(static_cast<unsigned char>(ch));
    }).base(), value.end());
    return value;
}

bool isQuoted(const std::string &value)
{
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

// Decodes a TOML basic-string body (the text between the quotes). Strict on
// purpose, and a deliberate SUBSET of TOML basic strings: only the escapes
// our writer emits (\\ \" \n \t \r and ASCII \uXXXX) decode; anything else
// — including TOML's legal \b, \f, \UXXXXXXXX — is a named failure rather
// than a second silent meaning. tomllib reads everything this dialect
// writes; the reverse is not the contract.
bool decodeBasicString(const std::string &body, std::string &out, std::string &problem)
{
    out.clear();
    out.reserve(body.size());
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char ch = body[i];
        if (ch == '"') {
            problem = "unescaped quote inside string value";
            return false;
        }
        if (ch != '\\') {
            out += ch;
            continue;
        }
        if (i + 1 >= body.size()) {
            problem = "dangling backslash at end of string value";
            return false;
        }
        const char escape = body[++i];
        switch (escape) {
        case '\\': out += '\\'; break;
        case '"': out += '"'; break;
        case 'n': out += '\n'; break;
        case 't': out += '\t'; break;
        case 'r': out += '\r'; break;
        case 'u': {
            if (i + 4 >= body.size()) {
                problem = "truncated \\u escape in string value";
                return false;
            }
            unsigned int code = 0;
            for (int digit = 1; digit <= 4; ++digit) {
                const char hex = body[i + digit];
                code <<= 4;
                if (hex >= '0' && hex <= '9') {
                    code |= static_cast<unsigned int>(hex - '0');
                } else if (hex >= 'a' && hex <= 'f') {
                    code |= static_cast<unsigned int>(hex - 'a' + 10);
                } else if (hex >= 'A' && hex <= 'F') {
                    code |= static_cast<unsigned int>(hex - 'A' + 10);
                } else {
                    problem = "invalid \\u escape in string value";
                    return false;
                }
            }
            if (code > 0x7F) {
                problem = "non-ASCII \\u escape not supported";
                return false;
            }
            out += static_cast<char>(code);
            i += 4;
            break;
        }
        default:
            problem = std::string("unknown escape \\") + escape + " in string value";
            return false;
        }
    }
    return true;
}

} // namespace

FormatResult<StaticConfig> readTomlStaticConfig(const std::string &text, const std::string &source)
{
    StaticConfig config;
    std::istringstream stream(text);
    std::string line;
    int lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            return FormatResult<StaticConfig>::failure(source, FormatResultCode::SyntaxError, "expected key = value on line " + std::to_string(lineNumber));
        }
        std::string key = trim(line.substr(0, equals));
        std::string rawValue = trim(line.substr(equals + 1));
        if (key.empty()) {
            return FormatResult<StaticConfig>::failure(source, FormatResultCode::EmptyKey, "empty key on line " + std::to_string(lineNumber));
        }
        if (config.find(key) != config.end()) {
            return FormatResult<StaticConfig>::failure(source, FormatResultCode::DuplicateKey, "duplicate key on line " + std::to_string(lineNumber));
        }
        if (!isQuoted(rawValue)) {
            return FormatResult<StaticConfig>::failure(source, FormatResultCode::SyntaxError, "expected quoted string value on line " + std::to_string(lineNumber));
        }
        std::string value;
        std::string problem;
        if (!decodeBasicString(rawValue.substr(1, rawValue.size() - 2), value, problem)) {
            return FormatResult<StaticConfig>::failure(source, FormatResultCode::SyntaxError, problem + " on line " + std::to_string(lineNumber));
        }
        config[std::move(key)] = std::move(value);
    }
    return FormatResult<StaticConfig>::success(std::move(config));
}

} // namespace edi::formats
