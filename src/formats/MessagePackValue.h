#pragma once

#include "formats/FormatResult.h"
#include "formats/MessagePackInspector.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace edi::formats {

// Plain-data MessagePack value. Recursive members (vector of incomplete type)
// are well-formed since C++17. No Qt, no behavior — pure data.
struct MsgPackValue {
    enum class Type { Nil, Bool, Int, Double, String, Array, Map };

    Type type = Type::Nil;
    bool boolValue = false;
    std::int64_t intValue = 0;
    double doubleValue = 0.0;
    std::string stringValue;
    std::vector<MsgPackValue> arrayValue;
    std::vector<std::pair<std::string, MsgPackValue>> mapValue;

    static MsgPackValue nil() { return MsgPackValue{}; }
    static MsgPackValue boolean(bool v) { MsgPackValue m; m.type = Type::Bool; m.boolValue = v; return m; }
    static MsgPackValue integer(std::int64_t v) { MsgPackValue m; m.type = Type::Int; m.intValue = v; return m; }
    static MsgPackValue number(double v) { MsgPackValue m; m.type = Type::Double; m.doubleValue = v; return m; }
    static MsgPackValue text(std::string v) { MsgPackValue m; m.type = Type::String; m.stringValue = std::move(v); return m; }
    static MsgPackValue array(std::vector<MsgPackValue> v) { MsgPackValue m; m.type = Type::Array; m.arrayValue = std::move(v); return m; }
    static MsgPackValue map(std::vector<std::pair<std::string, MsgPackValue>> v) { MsgPackValue m; m.type = Type::Map; m.mapValue = std::move(v); return m; }

    bool isNil() const { return type == Type::Nil; }

    // Map lookup helper; returns nullptr when absent. Forward-compatible reads
    // tolerate missing keys by checking the result.
    const MsgPackValue *find(const std::string &key) const
    {
        for (const auto &entry : mapValue) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }
};

// Real msgpack wire format. encode picks the smallest representation for ints,
// strings, arrays, and maps; doubles always use float64 for byte-stability.
ByteBuffer encodeMessagePack(const MsgPackValue &value);

// Decodes exactly one value and requires the whole buffer to be consumed.
// Truncated or trailing-garbage buffers are rejected.
FormatResult<MsgPackValue> decodeMessagePack(const ByteBuffer &bytes, const std::string &source = {});

} // namespace edi::formats
