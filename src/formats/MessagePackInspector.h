#pragma once

#include "formats/FormatResult.h"

#include <cstdint>
#include <string>
#include <vector>

namespace edi::formats {

using ByteBuffer = std::vector<std::uint8_t>;

inline constexpr std::uint8_t ediMessagePackMagic0 = 'E';
inline constexpr std::uint8_t ediMessagePackMagic1 = 'D';
inline constexpr std::uint8_t ediMessagePackMagic2 = 'I';
inline constexpr std::uint8_t ediMessagePackMagic3 = 'M';
inline constexpr int ediMessagePackSupportedVersion = 1;

struct MessagePackSummary {
    std::string schema;
    int version = 0;
    std::size_t byteSize = 0;
    std::size_t recordCount = 0;
};

FormatResult<MessagePackSummary> inspectMessagePack(const ByteBuffer &bytes, const std::string &source = {});

} // namespace edi::formats
