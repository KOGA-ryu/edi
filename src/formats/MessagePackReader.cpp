#include "formats/MessagePackReader.h"

namespace edi::formats {

FormatResult<MessagePackRecordSet> readMessagePackRecordSet(const ByteBuffer &bytes, const std::string &source)
{
    auto summary = inspectMessagePack(bytes, source);
    if (!summary.ok || !summary.value) {
        return FormatResult<MessagePackRecordSet>::failure(source, "msgpack.inspect_failed", "MessagePack inspection failed");
    }

    MessagePackRecordSet records;
    records.schema = summary.value->schema;
    records.version = bytes.size() > 1 ? bytes[1] : summary.value->version;
    records.recordCount = summary.value->recordCount;
    return FormatResult<MessagePackRecordSet>::success(records);
}

} // namespace edi::formats
