#pragma once

#include "formats/FormatResult.h"

#include <map>
#include <string>

namespace edi::formats {

using StaticConfig = std::map<std::string, std::string>;

FormatResult<StaticConfig> readTomlStaticConfig(const std::string &text, const std::string &source = {});

} // namespace edi::formats
