#include "formats/MessagePackValue.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace edi::formats {

namespace {

void putByte(ByteBuffer &out, std::uint8_t b)
{
    out.push_back(b);
}

// Big-endian writers for the fixed-width msgpack integer/length fields.
void putU16(ByteBuffer &out, std::uint16_t v)
{
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void putU32(ByteBuffer &out, std::uint32_t v)
{
    out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xff));
    out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xff));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(v & 0xff));
}

void putU64(ByteBuffer &out, std::uint64_t v)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xff));
    }
}

void encodeInt(ByteBuffer &out, std::int64_t v)
{
    if (v >= 0 && v <= 0x7f) {
        putByte(out, static_cast<std::uint8_t>(v)); // positive fixint
        return;
    }
    if (v < 0 && v >= -32) {
        putByte(out, static_cast<std::uint8_t>(v)); // negative fixint (0xe0..0xff)
        return;
    }
    if (v >= std::numeric_limits<std::int8_t>::min() && v <= std::numeric_limits<std::int8_t>::max()) {
        putByte(out, 0xd0);
        putByte(out, static_cast<std::uint8_t>(static_cast<std::int8_t>(v)));
        return;
    }
    if (v >= std::numeric_limits<std::int16_t>::min() && v <= std::numeric_limits<std::int16_t>::max()) {
        putByte(out, 0xd1);
        putU16(out, static_cast<std::uint16_t>(static_cast<std::int16_t>(v)));
        return;
    }
    if (v >= std::numeric_limits<std::int32_t>::min() && v <= std::numeric_limits<std::int32_t>::max()) {
        putByte(out, 0xd2);
        putU32(out, static_cast<std::uint32_t>(static_cast<std::int32_t>(v)));
        return;
    }
    putByte(out, 0xd3);
    putU64(out, static_cast<std::uint64_t>(v));
}

void encodeDouble(ByteBuffer &out, double v)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    putByte(out, 0xcb);
    putU64(out, bits);
}

void encodeString(ByteBuffer &out, const std::string &s)
{
    const std::size_t n = s.size();
    if (n <= 31) {
        putByte(out, static_cast<std::uint8_t>(0xa0 | n)); // fixstr
    } else if (n <= 0xff) {
        putByte(out, 0xd9);
        putByte(out, static_cast<std::uint8_t>(n));
    } else if (n <= 0xffff) {
        putByte(out, 0xda);
        putU16(out, static_cast<std::uint16_t>(n));
    } else {
        putByte(out, 0xdb);
        putU32(out, static_cast<std::uint32_t>(n));
    }
    out.insert(out.end(), s.begin(), s.end());
}

void encodeValue(ByteBuffer &out, const MsgPackValue &value);

void encodeArray(ByteBuffer &out, const std::vector<MsgPackValue> &items)
{
    const std::size_t n = items.size();
    if (n <= 15) {
        putByte(out, static_cast<std::uint8_t>(0x90 | n)); // fixarray
    } else if (n <= 0xffff) {
        putByte(out, 0xdc);
        putU16(out, static_cast<std::uint16_t>(n));
    } else {
        putByte(out, 0xdd);
        putU32(out, static_cast<std::uint32_t>(n));
    }
    for (const auto &item : items) {
        encodeValue(out, item);
    }
}

void encodeMap(ByteBuffer &out, const std::vector<std::pair<std::string, MsgPackValue>> &entries)
{
    const std::size_t n = entries.size();
    if (n <= 15) {
        putByte(out, static_cast<std::uint8_t>(0x80 | n)); // fixmap
    } else if (n <= 0xffff) {
        putByte(out, 0xde);
        putU16(out, static_cast<std::uint16_t>(n));
    } else {
        putByte(out, 0xdf);
        putU32(out, static_cast<std::uint32_t>(n));
    }
    for (const auto &entry : entries) {
        encodeString(out, entry.first);
        encodeValue(out, entry.second);
    }
}

void encodeValue(ByteBuffer &out, const MsgPackValue &value)
{
    switch (value.type) {
    case MsgPackValue::Type::Nil:
        putByte(out, 0xc0);
        break;
    case MsgPackValue::Type::Bool:
        putByte(out, value.boolValue ? 0xc3 : 0xc2);
        break;
    case MsgPackValue::Type::Int:
        encodeInt(out, value.intValue);
        break;
    case MsgPackValue::Type::Double:
        encodeDouble(out, value.doubleValue);
        break;
    case MsgPackValue::Type::String:
        encodeString(out, value.stringValue);
        break;
    case MsgPackValue::Type::Array:
        encodeArray(out, value.arrayValue);
        break;
    case MsgPackValue::Type::Map:
        encodeMap(out, value.mapValue);
        break;
    }
}

// Cursor-based decoder. Every read checks bounds and signals truncation by
// setting `ok = false`, which unwinds the recursion to a single failure.
struct Cursor {
    const ByteBuffer &bytes;
    std::size_t pos = 0;
    bool ok = true;

    std::uint8_t u8()
    {
        if (pos >= bytes.size()) {
            ok = false;
            return 0;
        }
        return bytes[pos++];
    }

    std::uint16_t u16()
    {
        const std::uint16_t hi = u8();
        const std::uint16_t lo = u8();
        return static_cast<std::uint16_t>((hi << 8) | lo);
    }

    std::uint32_t u32()
    {
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v = (v << 8) | u8();
        }
        return v;
    }

    std::uint64_t u64()
    {
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v = (v << 8) | u8();
        }
        return v;
    }

    std::string str(std::size_t n)
    {
        if (pos + n > bytes.size()) {
            pos = bytes.size();
            ok = false;
            return {};
        }
        std::string s(reinterpret_cast<const char *>(bytes.data()) + pos, n);
        pos += n;
        return s;
    }
};

MsgPackValue decodeValue(Cursor &c);

// A declared length can be attacker-controlled (up to 2^32-1 for array32/map32)
// in a corrupt buffer; every element needs at least one byte, so the real count
// can never exceed the bytes left. Cap the reserve to avoid a huge speculative
// allocation — the bounded loop still rejects truncation via c.ok.
std::size_t cappedReserve(const Cursor &c, std::size_t n)
{
    const std::size_t remaining = c.pos <= c.bytes.size() ? c.bytes.size() - c.pos : 0;
    return std::min(n, remaining);
}

MsgPackValue decodeArray(Cursor &c, std::size_t n)
{
    std::vector<MsgPackValue> items;
    items.reserve(cappedReserve(c, n));
    for (std::size_t i = 0; i < n && c.ok; ++i) {
        items.push_back(decodeValue(c));
    }
    return MsgPackValue::array(std::move(items));
}

MsgPackValue decodeMap(Cursor &c, std::size_t n)
{
    std::vector<std::pair<std::string, MsgPackValue>> entries;
    entries.reserve(cappedReserve(c, n));
    for (std::size_t i = 0; i < n && c.ok; ++i) {
        const std::uint8_t keyTag = c.u8();
        std::string key;
        if (keyTag >= 0xa0 && keyTag <= 0xbf) {
            key = c.str(keyTag & 0x1f);
        } else if (keyTag == 0xd9) {
            key = c.str(c.u8());
        } else if (keyTag == 0xda) {
            key = c.str(c.u16());
        } else if (keyTag == 0xdb) {
            key = c.str(c.u32());
        } else {
            c.ok = false; // non-string map key is unsupported by this schema
            return MsgPackValue::nil();
        }
        MsgPackValue v = decodeValue(c);
        entries.emplace_back(std::move(key), std::move(v));
    }
    return MsgPackValue::map(std::move(entries));
}

MsgPackValue decodeValue(Cursor &c)
{
    if (!c.ok) {
        return MsgPackValue::nil();
    }
    const std::uint8_t tag = c.u8();
    if (!c.ok) {
        return MsgPackValue::nil();
    }

    if (tag <= 0x7f) {
        return MsgPackValue::integer(tag); // positive fixint
    }
    if (tag >= 0xe0) {
        return MsgPackValue::integer(static_cast<std::int8_t>(tag)); // negative fixint
    }
    if (tag >= 0xa0 && tag <= 0xbf) {
        return MsgPackValue::text(c.str(tag & 0x1f)); // fixstr
    }
    if (tag >= 0x90 && tag <= 0x9f) {
        return decodeArray(c, tag & 0x0f); // fixarray
    }
    if (tag >= 0x80 && tag <= 0x8f) {
        return decodeMap(c, tag & 0x0f); // fixmap
    }

    switch (tag) {
    case 0xc0:
        return MsgPackValue::nil();
    case 0xc2:
        return MsgPackValue::boolean(false);
    case 0xc3:
        return MsgPackValue::boolean(true);
    case 0xca: {
        const std::uint32_t bits = c.u32();
        float f = 0.0f;
        std::memcpy(&f, &bits, sizeof(f));
        return MsgPackValue::number(static_cast<double>(f));
    }
    case 0xcb: {
        const std::uint64_t bits = c.u64();
        double d = 0.0;
        std::memcpy(&d, &bits, sizeof(d));
        return MsgPackValue::number(d);
    }
    case 0xcc:
        return MsgPackValue::integer(c.u8());
    case 0xcd:
        return MsgPackValue::integer(c.u16());
    case 0xce:
        return MsgPackValue::integer(c.u32());
    case 0xcf:
        return MsgPackValue::integer(static_cast<std::int64_t>(c.u64()));
    case 0xd0:
        return MsgPackValue::integer(static_cast<std::int8_t>(c.u8()));
    case 0xd1:
        return MsgPackValue::integer(static_cast<std::int16_t>(c.u16()));
    case 0xd2:
        return MsgPackValue::integer(static_cast<std::int32_t>(c.u32()));
    case 0xd3:
        return MsgPackValue::integer(static_cast<std::int64_t>(c.u64()));
    case 0xd9:
        return MsgPackValue::text(c.str(c.u8()));
    case 0xda:
        return MsgPackValue::text(c.str(c.u16()));
    case 0xdb:
        return MsgPackValue::text(c.str(c.u32()));
    case 0xdc:
        return decodeArray(c, c.u16());
    case 0xdd:
        return decodeArray(c, c.u32());
    case 0xde:
        return decodeMap(c, c.u16());
    case 0xdf:
        return decodeMap(c, c.u32());
    default:
        c.ok = false; // unsupported tag (bin/ext/fixext not used by this schema)
        return MsgPackValue::nil();
    }
}

} // namespace

ByteBuffer encodeMessagePack(const MsgPackValue &value)
{
    ByteBuffer out;
    encodeValue(out, value);
    return out;
}

FormatResult<MsgPackValue> decodeMessagePack(const ByteBuffer &bytes, const std::string &source)
{
    if (bytes.empty()) {
        return FormatResult<MsgPackValue>::failure(source, FormatResultCode::EmptyBuffer, "MessagePack buffer is empty");
    }
    Cursor c{bytes};
    MsgPackValue value = decodeValue(c);
    if (!c.ok) {
        return FormatResult<MsgPackValue>::failure(source, FormatResultCode::SyntaxError, "MessagePack buffer is truncated or malformed");
    }
    if (c.pos != bytes.size()) {
        return FormatResult<MsgPackValue>::failure(source, FormatResultCode::SyntaxError, "MessagePack buffer has trailing bytes");
    }
    return FormatResult<MsgPackValue>::success(std::move(value));
}

} // namespace edi::formats
