#pragma once

#include "formats/FormatResult.h"

#include <map>
#include <string>

namespace edi::formats {

struct ToonPacket {
    std::string kind;
    std::string title;
    std::map<std::string, std::string> fields;
};

FormatResult<std::string> exportToonPacket(const ToonPacket &packet, const std::string &source = {});

} // namespace edi::formats
