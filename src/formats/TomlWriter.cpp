#include "formats/TomlWriter.h"

#include <sstream>

namespace edi::formats {

FormatResult<std::string> writeTomlStaticConfig(const StaticConfig &config, const std::string &)
{
    std::ostringstream output;
    for (const auto &[key, value] : config) {
        if (key.empty()) {
            return FormatResult<std::string>::failure({}, FormatResultCode::EmptyKey, "empty TOML key cannot be written");
        }
        output << key << " = \"" << value << "\"\n";
    }
    return FormatResult<std::string>::success(output.str());
}

} // namespace edi::formats
