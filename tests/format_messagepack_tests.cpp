#include "formats/MessagePackReader.h"
#include "formats/MessagePackWriter.h"

#include <cassert>
#include <limits>

using namespace edi::formats;

int main()
{
    MessagePackRecordSet records;
    records.recordCount = 3;
    records.version = ediMessagePackSupportedVersion;

    auto write = writeMessagePackRecordSet(records, "fixture");
    assert(write.ok);
    assert(write.value);
    assert(write.value->size() == 6);
    assert((*write.value)[0] == ediMessagePackMagic0);

    auto inspect = inspectMessagePack(*write.value, "fixture");
    assert(inspect.ok);
    assert(inspect.value);
    assert(inspect.value->recordCount == 3);

    auto read = readMessagePackRecordSet(*write.value, "fixture");
    assert(read.ok);
    assert(read.value);
    assert(read.value->recordCount == 3);
    assert(read.value->version == ediMessagePackSupportedVersion);

    auto empty = inspectMessagePack({}, "fixture");
    assert(!empty.ok);
    assert(empty.code == FormatResultCode::EmptyBuffer);
    assert(empty.errors.front().code == FormatResultCode::EmptyBuffer);

    ByteBuffer badSchema = {'B', 'A', 'D', '!', 1, 0};
    auto unsupportedSchema = inspectMessagePack(badSchema, "fixture");
    assert(!unsupportedSchema.ok);
    assert(unsupportedSchema.code == FormatResultCode::UnsupportedSchema);
    assert(unsupportedSchema.errors.front().code == FormatResultCode::UnsupportedSchema);

    ByteBuffer badVersion = {ediMessagePackMagic0, ediMessagePackMagic1, ediMessagePackMagic2, ediMessagePackMagic3, 99, 0};
    auto unsupportedVersion = inspectMessagePack(badVersion, "fixture");
    assert(!unsupportedVersion.ok);
    assert(unsupportedVersion.errors.front().code == FormatResultCode::UnsupportedVersion);

    MessagePackRecordSet unsupportedWriterSchema;
    unsupportedWriterSchema.schema = "other";
    auto badWriteSchema = writeMessagePackRecordSet(unsupportedWriterSchema, "fixture");
    assert(!badWriteSchema.ok);
    assert(badWriteSchema.errors.front().code == FormatResultCode::UnsupportedSchema);

    MessagePackRecordSet unsupportedWriterVersion;
    unsupportedWriterVersion.version = 99;
    auto badWriteVersion = writeMessagePackRecordSet(unsupportedWriterVersion, "fixture");
    assert(!badWriteVersion.ok);
    assert(badWriteVersion.errors.front().code == FormatResultCode::UnsupportedVersion);

    MessagePackRecordSet tooManyRecords;
    tooManyRecords.recordCount = static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max()) + 1;
    auto tooMany = writeMessagePackRecordSet(tooManyRecords, "fixture");
    assert(!tooMany.ok);
    assert(tooMany.code == FormatResultCode::InvalidRecordCount);
    assert(tooMany.errors.front().code == FormatResultCode::InvalidRecordCount);

    return 0;
}
