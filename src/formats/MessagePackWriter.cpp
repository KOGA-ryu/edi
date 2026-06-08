#include "formats/MessagePackWriter.h"

#include <limits>

namespace edi::formats {

FormatResult<ByteBuffer> writeMessagePackRecordSet(const MessagePackRecordSet &records, const std::string &source)
{
    if (records.schema != "edi.placeholder") {
        return FormatResult<ByteBuffer>::failure(source, FormatResultCode::UnsupportedSchema, "only edi.placeholder schema is supported");
    }
    if (records.version != ediMessagePackSupportedVersion) {
        return FormatResult<ByteBuffer>::failure(source, FormatResultCode::UnsupportedVersion, "unsupported MessagePack placeholder version");
    }
    if (records.recordCount > std::numeric_limits<std::uint8_t>::max()) {
        return FormatResult<ByteBuffer>::failure(source, FormatResultCode::InvalidRecordCount, "record count does not fit placeholder encoding");
    }

    ByteBuffer bytes;
    bytes.push_back(ediMessagePackMagic0);
    bytes.push_back(ediMessagePackMagic1);
    bytes.push_back(ediMessagePackMagic2);
    bytes.push_back(ediMessagePackMagic3);
    bytes.push_back(static_cast<std::uint8_t>(records.version));
    bytes.push_back(static_cast<std::uint8_t>(records.recordCount));
    return FormatResult<ByteBuffer>::success(std::move(bytes));
}

} // namespace edi::formats
