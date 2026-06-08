#pragma once

#include "formats/FormatResult.h"

#include <cstdint>
#include <string>
#include <vector>

namespace edi::formats {

using ByteBuffer = std::vector<std::uint8_t>;

struct MessagePackSummary {
    std::string schema;
    int version = 0;
    std::size_t byteSize = 0;
    std::size_t recordCount = 0;
};

FormatResult<MessagePackSummary> inspectMessagePack(const ByteBuffer &bytes, const std::string &source = {});

} // namespace edi::formats
