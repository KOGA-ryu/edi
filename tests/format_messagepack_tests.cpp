#include "formats/MessagePackReader.h"
#include "formats/MessagePackWriter.h"

#include <cassert>

using namespace edi::formats;

int main()
{
    MessagePackRecordSet records;
    records.recordCount = 3;
    records.version = 7;

    auto write = writeMessagePackRecordSet(records, "fixture");
    assert(write.ok);
    assert(write.value);

    auto inspect = inspectMessagePack(*write.value, "fixture");
    assert(inspect.ok);
    assert(inspect.value);
    assert(inspect.value->recordCount == 3);

    auto read = readMessagePackRecordSet(*write.value, "fixture");
    assert(read.ok);
    assert(read.value);
    assert(read.value->recordCount == 3);
    assert(read.value->version == 7);

    auto empty = inspectMessagePack({}, "fixture");
    assert(!empty.ok);

    return 0;
}
