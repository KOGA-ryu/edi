#include "formats/ToonExport.h"

#include <sstream>

namespace edi::formats {

FormatResult<std::string> exportToonPacket(const ToonPacket &packet, const std::string &source)
{
    if (packet.kind.empty()) {
        return FormatResult<std::string>::failure(source, "toon.kind_required", "TOON packet kind is required");
    }

    std::ostringstream output;
    output << "kind: " << packet.kind << "\n";
    if (!packet.title.empty()) {
        output << "title: " << packet.title << "\n";
    }
    for (const auto &[key, value] : packet.fields) {
        output << key << ": " << value << "\n";
    }
    return FormatResult<std::string>::success(output.str());
}

} // namespace edi::formats
