#pragma once

#include "formats/MessagePackInspector.h"

namespace edi::formats {

struct MessagePackRecordSet {
    std::string schema = "edi.placeholder";
    int version = 1;
    std::size_t recordCount = 0;
};

FormatResult<ByteBuffer> writeMessagePackRecordSet(const MessagePackRecordSet &records, const std::string &source = {});

} // namespace edi::formats
