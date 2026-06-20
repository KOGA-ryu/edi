#include "formats/MessagePackReader.h"
#include "formats/MessagePackWriter.h"

#include "EdiAssert.h"
#include <limits>

using namespace edi::formats;

int main()
{
    MessagePackRecordSet records;
    records.recordCount = 3;
    records.version = ediMessagePackSupportedVersion;

    auto write = writeMessagePackRecordSet(records, "fixture");
    EDI_CHECK(write.ok);
    EDI_CHECK(write.value);
    EDI_CHECK(write.value->size() == 6);
    EDI_CHECK((*write.value)[0] == ediMessagePackMagic0);

    auto inspect = inspectMessagePack(*write.value, "fixture");
    EDI_CHECK(inspect.ok);
    EDI_CHECK(inspect.value);
    EDI_CHECK(inspect.value->recordCount == 3);

    auto read = readMessagePackRecordSet(*write.value, "fixture");
    EDI_CHECK(read.ok);
    EDI_CHECK(read.value);
    EDI_CHECK(read.value->recordCount == 3);
    EDI_CHECK(read.value->version == ediMessagePackSupportedVersion);

    auto empty = inspectMessagePack({}, "fixture");
    EDI_CHECK(!empty.ok);
    EDI_CHECK(empty.code == FormatResultCode::EmptyBuffer);
    EDI_CHECK(empty.errors.front().code == FormatResultCode::EmptyBuffer);

    ByteBuffer badSchema = {'B', 'A', 'D', '!', 1, 0};
    auto unsupportedSchema = inspectMessagePack(badSchema, "fixture");
    EDI_CHECK(!unsupportedSchema.ok);
    EDI_CHECK(unsupportedSchema.code == FormatResultCode::UnsupportedSchema);
    EDI_CHECK(unsupportedSchema.errors.front().code == FormatResultCode::UnsupportedSchema);

    ByteBuffer badVersion = {ediMessagePackMagic0, ediMessagePackMagic1, ediMessagePackMagic2, ediMessagePackMagic3, 99, 0};
    auto unsupportedVersion = inspectMessagePack(badVersion, "fixture");
    EDI_CHECK(!unsupportedVersion.ok);
    EDI_CHECK(unsupportedVersion.errors.front().code == FormatResultCode::UnsupportedVersion);

    MessagePackRecordSet unsupportedWriterSchema;
    unsupportedWriterSchema.schema = "other";
    auto badWriteSchema = writeMessagePackRecordSet(unsupportedWriterSchema, "fixture");
    EDI_CHECK(!badWriteSchema.ok);
    EDI_CHECK(badWriteSchema.errors.front().code == FormatResultCode::UnsupportedSchema);

    MessagePackRecordSet unsupportedWriterVersion;
    unsupportedWriterVersion.version = 99;
    auto badWriteVersion = writeMessagePackRecordSet(unsupportedWriterVersion, "fixture");
    EDI_CHECK(!badWriteVersion.ok);
    EDI_CHECK(badWriteVersion.errors.front().code == FormatResultCode::UnsupportedVersion);

    MessagePackRecordSet tooManyRecords;
    tooManyRecords.recordCount = static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max()) + 1;
    auto tooMany = writeMessagePackRecordSet(tooManyRecords, "fixture");
    EDI_CHECK(!tooMany.ok);
    EDI_CHECK(tooMany.code == FormatResultCode::InvalidRecordCount);
    EDI_CHECK(tooMany.errors.front().code == FormatResultCode::InvalidRecordCount);

    return 0;
}
