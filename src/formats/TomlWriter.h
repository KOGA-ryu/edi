#pragma once

#include "formats/TomlReader.h"

#include <string>

namespace edi::formats {

FormatResult<std::string> writeTomlStaticConfig(const StaticConfig &config, const std::string &source = {});

} // namespace edi::formats
