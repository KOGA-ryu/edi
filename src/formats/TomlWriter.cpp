#include "formats/TomlWriter.h"

#include <sstream>

namespace edi::formats {

FormatResult<std::string> writeTomlStaticConfig(const StaticConfig &config, const std::string &)
{
    std::ostringstream output;
    for (const auto &[key, value] : config) {
        output << key << " = \"" << value << "\"\n";
    }
    return FormatResult<std::string>::success(output.str());
}

} // namespace edi::formats
