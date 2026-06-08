#include "formats/MessagePackWriter.h"

#include <limits>

namespace edi::formats {

FormatResult<ByteBuffer> writeMessagePackRecordSet(const MessagePackRecordSet &records, const std::string &source)
{
    if (records.recordCount > std::numeric_limits<std::uint8_t>::max()) {
        return FormatResult<ByteBuffer>::failure(source, "msgpack.record_count_range", "record count does not fit placeholder encoding");
    }

    ByteBuffer bytes;
    bytes.push_back(static_cast<std::uint8_t>(records.recordCount));
    bytes.push_back(static_cast<std::uint8_t>(records.version));
    return FormatResult<ByteBuffer>::success(std::move(bytes));
}

} // namespace edi::formats
