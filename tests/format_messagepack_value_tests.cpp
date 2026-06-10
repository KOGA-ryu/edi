#include "formats/MessagePackValue.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace edi::formats;

namespace {

MsgPackValue roundTrip(const MsgPackValue &value)
{
    ByteBuffer bytes = encodeMessagePack(value);
    auto decoded = decodeMessagePack(bytes, "fixture");
    assert(decoded.ok);
    assert(decoded.value);
    return *decoded.value;
}

void expectInt(std::int64_t v)
{
    MsgPackValue r = roundTrip(MsgPackValue::integer(v));
    assert(r.type == MsgPackValue::Type::Int);
    assert(r.intValue == v);
}

void expectString(const std::string &s)
{
    MsgPackValue r = roundTrip(MsgPackValue::text(s));
    assert(r.type == MsgPackValue::Type::String);
    assert(r.stringValue == s);
}

} // namespace

int main()
{
    // Scalars.
    {
        MsgPackValue r = roundTrip(MsgPackValue::nil());
        assert(r.type == MsgPackValue::Type::Nil);
    }
    {
        MsgPackValue t = roundTrip(MsgPackValue::boolean(true));
        MsgPackValue f = roundTrip(MsgPackValue::boolean(false));
        assert(t.type == MsgPackValue::Type::Bool && t.boolValue);
        assert(f.type == MsgPackValue::Type::Bool && !f.boolValue);
    }

    // Integer boundaries across every encoding band.
    expectInt(0);
    expectInt(1);
    expectInt(127);   // positive fixint max
    expectInt(128);   // promotes to int family
    expectInt(-1);
    expectInt(-32);   // negative fixint min
    expectInt(-33);   // promotes to int8
    expectInt(255);
    expectInt(256);
    expectInt(-128);
    expectInt(-129);
    expectInt(32767);
    expectInt(32768);
    expectInt(-32768);
    expectInt(-32769);
    expectInt(2147483647);
    expectInt(2147483648LL);
    expectInt(-2147483648LL);
    expectInt(-2147483649LL);
    expectInt(std::numeric_limits<std::int64_t>::max());
    expectInt(std::numeric_limits<std::int64_t>::min());

    // Doubles (including special-ish values).
    for (double d : {0.0, 1.5, -3.25, 3.14159265358979, 1e300, -1e-300}) {
        MsgPackValue r = roundTrip(MsgPackValue::number(d));
        assert(r.type == MsgPackValue::Type::Double);
        assert(r.doubleValue == d);
    }

    // Strings: empty, fixstr, str8, str16 boundaries.
    expectString("");
    expectString("edi.drawing");
    expectString(std::string(31, 'x'));   // fixstr max
    expectString(std::string(32, 'y'));   // str8
    expectString(std::string(255, 'z'));  // str8 max
    expectString(std::string(256, 'w'));  // str16
    expectString(std::string(70000, 'q')); // str32

    // Nested array + map round-trip.
    {
        MsgPackValue inner = MsgPackValue::array({
            MsgPackValue::integer(10),
            MsgPackValue::text("a"),
            MsgPackValue::boolean(false),
        });
        MsgPackValue doc = MsgPackValue::map({
            {"schema", MsgPackValue::text("edi.drawing")},
            {"version", MsgPackValue::integer(1)},
            {"ratio", MsgPackValue::number(0.5)},
            {"items", inner},
            {"nothing", MsgPackValue::nil()},
        });
        MsgPackValue r = roundTrip(doc);
        assert(r.type == MsgPackValue::Type::Map);
        assert(r.mapValue.size() == 5);
        const MsgPackValue *schema = r.find("schema");
        assert(schema && schema->stringValue == "edi.drawing");
        const MsgPackValue *items = r.find("items");
        assert(items && items->type == MsgPackValue::Type::Array);
        assert(items->arrayValue.size() == 3);
        assert(items->arrayValue[0].intValue == 10);
        assert(items->arrayValue[1].stringValue == "a");
        assert(r.find("absent") == nullptr);
        const MsgPackValue *nothing = r.find("nothing");
        assert(nothing && nothing->isNil());
    }

    // Larger array forcing array16 framing.
    {
        std::vector<MsgPackValue> big;
        for (int i = 0; i < 100; ++i) {
            big.push_back(MsgPackValue::integer(i));
        }
        MsgPackValue r = roundTrip(MsgPackValue::array(big));
        assert(r.arrayValue.size() == 100);
        assert(r.arrayValue[99].intValue == 99);
    }

    // Empty buffer rejected.
    {
        auto empty = decodeMessagePack({}, "fixture");
        assert(!empty.ok);
        assert(empty.code == FormatResultCode::EmptyBuffer);
    }

    // Truncated buffer rejected: encode a string, lop off trailing bytes.
    {
        ByteBuffer bytes = encodeMessagePack(MsgPackValue::text("hello world"));
        bytes.resize(bytes.size() - 3);
        auto truncated = decodeMessagePack(bytes, "fixture");
        assert(!truncated.ok);
        assert(truncated.code == FormatResultCode::SyntaxError);
    }

    // Trailing garbage rejected.
    {
        ByteBuffer bytes = encodeMessagePack(MsgPackValue::integer(7));
        bytes.push_back(0xc0);
        auto trailing = decodeMessagePack(bytes, "fixture");
        assert(!trailing.ok);
    }

    // Truncated array length rejected.
    {
        ByteBuffer bytes = encodeMessagePack(MsgPackValue::array({MsgPackValue::integer(1), MsgPackValue::integer(2)}));
        bytes.pop_back();
        auto truncated = decodeMessagePack(bytes, "fixture");
        assert(!truncated.ok);
    }

    // float32 (0xca) decodes even though the encoder only emits float64: a
    // foreign buffer with a float32 0.5 (0x3f000000) widens to 0.5.
    {
        ByteBuffer bytes = {0xca, 0x3f, 0x00, 0x00, 0x00};
        auto decoded = decodeMessagePack(bytes, "fixture");
        assert(decoded.ok);
        assert(decoded.value->type == MsgPackValue::Type::Double);
        assert(decoded.value->doubleValue == 0.5);
    }
    // uint64 with the high bit set (0xcf) decodes through the int64 family.
    {
        ByteBuffer bytes = {0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xd2};
        auto decoded = decodeMessagePack(bytes, "fixture");
        assert(decoded.ok);
        assert(decoded.value->type == MsgPackValue::Type::Int);
        assert(decoded.value->intValue == 1234);
    }

    // A hostile array32 length (near 2^32) must be rejected as truncated rather
    // than triggering a multi-GB speculative reserve / bad_alloc.
    {
        ByteBuffer bytes = {0xdd, 0xff, 0xff, 0xff, 0xff}; // array32 with ~4e9 elements
        auto hostile = decodeMessagePack(bytes, "fixture");
        assert(!hostile.ok);
    }
    {
        ByteBuffer bytes = {0xdf, 0xff, 0xff, 0xff, 0xff}; // map32 with ~4e9 entries
        auto hostile = decodeMessagePack(bytes, "fixture");
        assert(!hostile.ok);
    }

    return 0;
}
