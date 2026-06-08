#include "formats/MessagePackInspector.h"

namespace edi::formats {

FormatResult<MessagePackSummary> inspectMessagePack(const ByteBuffer &bytes, const std::string &source)
{
    if (bytes.empty()) {
        return FormatResult<MessagePackSummary>::failure(source, FormatResultCode::EmptyBuffer, "MessagePack buffer is empty");
    }
    if (bytes.size() < 6) {
        return FormatResult<MessagePackSummary>::failure(source, FormatResultCode::MissingSchema, "MessagePack placeholder header is incomplete");
    }
    if (bytes[0] != ediMessagePackMagic0
        || bytes[1] != ediMessagePackMagic1
        || bytes[2] != ediMessagePackMagic2
        || bytes[3] != ediMessagePackMagic3) {
        return FormatResult<MessagePackSummary>::failure(source, FormatResultCode::UnsupportedSchema, "MessagePack placeholder schema marker is unsupported");
    }
    if (bytes[4] != ediMessagePackSupportedVersion) {
        return FormatResult<MessagePackSummary>::failure(source, FormatResultCode::UnsupportedVersion, "MessagePack placeholder version is unsupported");
    }

    MessagePackSummary summary;
    summary.schema = "edi.placeholder";
    summary.version = bytes[4];
    summary.byteSize = bytes.size();
    summary.recordCount = bytes[5];
    return FormatResult<MessagePackSummary>::success(summary);
}

} // namespace edi::formats
