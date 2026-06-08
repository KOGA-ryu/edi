#include "formats/MessagePackInspector.h"

namespace edi::formats {

FormatResult<MessagePackSummary> inspectMessagePack(const ByteBuffer &bytes, const std::string &source)
{
    if (bytes.empty()) {
        return FormatResult<MessagePackSummary>::failure(source, "msgpack.empty", "MessagePack buffer is empty");
    }

    MessagePackSummary summary;
    summary.schema = "edi.placeholder";
    summary.version = 1;
    summary.byteSize = bytes.size();
    summary.recordCount = bytes.front();
    return FormatResult<MessagePackSummary>::success(summary);
}

} // namespace edi::formats
