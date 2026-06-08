#include "formats/MessagePackReader.h"

namespace edi::formats {

FormatResult<MessagePackRecordSet> readMessagePackRecordSet(const ByteBuffer &bytes, const std::string &source)
{
    auto summary = inspectMessagePack(bytes, source);
    if (!summary.ok || !summary.value) {
        if (!summary.errors.empty()) {
            const auto &error = summary.errors.front();
            return FormatResult<MessagePackRecordSet>::failure(error.source, error.code, error.message);
        }
        return FormatResult<MessagePackRecordSet>::failure(source, FormatResultCode::SyntaxError, "MessagePack inspection failed");
    }

    MessagePackRecordSet records;
    records.schema = summary.value->schema;
    records.version = summary.value->version;
    records.recordCount = summary.value->recordCount;
    return FormatResult<MessagePackRecordSet>::success(records);
}

} // namespace edi::formats
