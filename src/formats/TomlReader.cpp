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

std::string unquote(std::string value)
{
    value = trim(std::move(value));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
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
            return FormatResult<StaticConfig>::failure(source, "toml.syntax", "expected key = value on line " + std::to_string(lineNumber));
        }
        std::string key = trim(line.substr(0, equals));
        std::string value = unquote(line.substr(equals + 1));
        if (key.empty()) {
            return FormatResult<StaticConfig>::failure(source, "toml.empty_key", "empty key on line " + std::to_string(lineNumber));
        }
        config[std::move(key)] = std::move(value);
    }
    return FormatResult<StaticConfig>::success(std::move(config));
}

} // namespace edi::formats
