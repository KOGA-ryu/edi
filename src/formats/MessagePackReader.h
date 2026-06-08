#pragma once

#include "formats/MessagePackWriter.h"

namespace edi::formats {

FormatResult<MessagePackRecordSet> readMessagePackRecordSet(const ByteBuffer &bytes, const std::string &source = {});

} // namespace edi::formats
