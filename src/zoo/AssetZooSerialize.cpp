#include "zoo/AssetZooSerialize.h"

#include <cstdint>
#include <utility>

namespace edi::zoo {

using edi::formats::ByteBuffer;
using edi::formats::FormatResult;
using edi::formats::FormatResultCode;
using edi::formats::MsgPackValue;

namespace {

constexpr const char *kAssetZooSchema = "edi.zoo";
constexpr std::int64_t kAssetZooVersion = 1;

// Tolerant readers — the zoo's small local copies of the document codec's helpers
// (asString/asInt/asBool/child), so the module reads MessagePack without linking
// the drafting serializer for them.
std::int64_t asInt(const MsgPackValue *v, std::int64_t fallback)
{
    if (!v) return fallback;
    if (v->type == MsgPackValue::Type::Int) return v->intValue;
    if (v->type == MsgPackValue::Type::Double) return static_cast<std::int64_t>(v->doubleValue);
    return fallback;
}

double asDouble(const MsgPackValue *v, double fallback)
{
    if (!v) return fallback;
    if (v->type == MsgPackValue::Type::Double) return v->doubleValue;
    if (v->type == MsgPackValue::Type::Int) return static_cast<double>(v->intValue);
    return fallback;
}

bool asBool(const MsgPackValue *v, bool fallback)
{
    if (v && v->type == MsgPackValue::Type::Bool) return v->boolValue;
    return fallback;
}

std::string asString(const MsgPackValue *v, const std::string &fallback)
{
    if (v && v->type == MsgPackValue::Type::String) return v->stringValue;
    return fallback;
}

const MsgPackValue *child(const MsgPackValue &map, const std::string &key)
{
    return map.type == MsgPackValue::Type::Map ? map.find(key) : nullptr;
}

MsgPackValue assetValue(const AssetRecord &asset)
{
    std::vector<MsgPackValue> textures;
    textures.reserve(asset.textureRefs.size());
    for (const std::string &ref : asset.textureRefs) {
        textures.push_back(MsgPackValue::text(ref));
    }
    // sockets: an array of socket maps; the anchor is a 2-element [x,y] array,
    // parity with drafting's pointValue (number() per element).
    std::vector<MsgPackValue> sockets;
    sockets.reserve(asset.sockets.size());
    for (const AssetSocket &socket : asset.sockets) {
        sockets.push_back(MsgPackValue::map({
            {"name", MsgPackValue::text(socket.name)},
            {"type", MsgPackValue::text(socket.type)},
            {"anchor", MsgPackValue::array({MsgPackValue::number(socket.anchor.x),
                                            MsgPackValue::number(socket.anchor.y)})},
        }));
    }
    return MsgPackValue::map({
        {"id", MsgPackValue::text(asset.id)},
        {"name", MsgPackValue::text(asset.name)},
        {"category", MsgPackValue::text(asset.category)},
        {"mesh_ref", MsgPackValue::text(asset.meshRef)},
        {"proxy_ref", MsgPackValue::text(asset.proxyRef)},
        {"curated", MsgPackValue::boolean(asset.curated)},
        {"texture_refs", MsgPackValue::array(std::move(textures))},
        {"sockets", MsgPackValue::array(std::move(sockets))},
    });
}

AssetRecord readAsset(const MsgPackValue &v)
{
    AssetRecord asset;
    asset.id = asString(child(v, "id"), asset.id);
    asset.name = asString(child(v, "name"), asset.name);
    asset.category = asString(child(v, "category"), asset.category);
    asset.meshRef = asString(child(v, "mesh_ref"), asset.meshRef);
    asset.proxyRef = asString(child(v, "proxy_ref"), asset.proxyRef);
    asset.curated = asBool(child(v, "curated"), asset.curated);
    if (const MsgPackValue *textures = child(v, "texture_refs");
        textures && textures->type == MsgPackValue::Type::Array) {
        for (const MsgPackValue &ref : textures->arrayValue) {
            if (ref.type == MsgPackValue::Type::String) {
                asset.textureRefs.push_back(ref.stringValue);
            }
        }
    }
    // sockets: missing key => empty vector (additive). The anchor is read like
    // drafting's readPoint — a 2-element array, else defaults to {0, 0}.
    if (const MsgPackValue *sockets = child(v, "sockets");
        sockets && sockets->type == MsgPackValue::Type::Array) {
        for (const MsgPackValue &item : sockets->arrayValue) {
            if (item.type != MsgPackValue::Type::Map) {
                continue;
            }
            AssetSocket socket;
            socket.name = asString(child(item, "name"), socket.name);
            socket.type = asString(child(item, "type"), socket.type);
            if (const MsgPackValue *anchor = child(item, "anchor");
                anchor && anchor->type == MsgPackValue::Type::Array
                && anchor->arrayValue.size() == 2) {
                socket.anchor.x = asDouble(&anchor->arrayValue[0], 0.0);
                socket.anchor.y = asDouble(&anchor->arrayValue[1], 0.0);
            }
            asset.sockets.push_back(std::move(socket));
        }
    }
    return asset;
}

} // namespace

MsgPackValue assetZooToValue(const AssetZoo &zoo)
{
    std::vector<MsgPackValue> assets;
    assets.reserve(zoo.assets.size());
    for (const AssetRecord &asset : zoo.assets) {
        assets.push_back(assetValue(asset));
    }
    MsgPackValue zooMap = MsgPackValue::map({
        {"assets", MsgPackValue::array(std::move(assets))},
    });
    return MsgPackValue::map({
        {"schema", MsgPackValue::text(kAssetZooSchema)},
        {"version", MsgPackValue::integer(kAssetZooVersion)},
        {"zoo", std::move(zooMap)},
    });
}

FormatResult<AssetZoo> assetZooFromValue(const MsgPackValue &value)
{
    if (value.type != MsgPackValue::Type::Map) {
        return FormatResult<AssetZoo>::failure({}, FormatResultCode::SyntaxError,
                                               "asset zoo root is not a map");
    }
    if (asString(value.find("schema"), {}) != kAssetZooSchema) {
        return FormatResult<AssetZoo>::failure({}, FormatResultCode::UnsupportedSchema,
                                               "unsupported asset zoo schema");
    }
    const MsgPackValue *versionValue = value.find("version");
    if (!versionValue) {
        return FormatResult<AssetZoo>::failure({}, FormatResultCode::MissingSchema,
                                               "asset zoo version is missing");
    }
    if (asInt(versionValue, -1) != kAssetZooVersion) {
        return FormatResult<AssetZoo>::failure({}, FormatResultCode::UnsupportedVersion,
                                               "unsupported asset zoo version");
    }

    // Tolerant additive read: a file with no "zoo"/"assets" key (or a future writer
    // that drops an empty catalog) decodes to an empty zoo, never an error.
    AssetZoo zoo;
    if (const MsgPackValue *zooValue = value.find("zoo");
        zooValue && zooValue->type == MsgPackValue::Type::Map) {
        if (const MsgPackValue *assets = child(*zooValue, "assets");
            assets && assets->type == MsgPackValue::Type::Array) {
            for (const MsgPackValue &asset : assets->arrayValue) {
                if (asset.type == MsgPackValue::Type::Map) {
                    zoo.assets.push_back(readAsset(asset));
                }
            }
        }
    }
    return FormatResult<AssetZoo>::success(std::move(zoo));
}

ByteBuffer encodeAssetZoo(const AssetZoo &zoo)
{
    ByteBuffer bytes;
    bytes.push_back(edi::formats::ediMessagePackMagic0);
    bytes.push_back(edi::formats::ediMessagePackMagic1);
    bytes.push_back(edi::formats::ediMessagePackMagic2);
    bytes.push_back(edi::formats::ediMessagePackMagic3);
    bytes.push_back(static_cast<std::uint8_t>(edi::formats::ediMessagePackSupportedVersion));
    ByteBuffer payload = edi::formats::encodeMessagePack(assetZooToValue(zoo));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

FormatResult<AssetZoo> decodeAssetZoo(const ByteBuffer &bytes, const std::string &source)
{
    if (bytes.size() < 5) {
        return FormatResult<AssetZoo>::failure(source, FormatResultCode::EmptyBuffer,
                                               "asset zoo file is too small");
    }
    if (bytes[0] != edi::formats::ediMessagePackMagic0
        || bytes[1] != edi::formats::ediMessagePackMagic1
        || bytes[2] != edi::formats::ediMessagePackMagic2
        || bytes[3] != edi::formats::ediMessagePackMagic3) {
        return FormatResult<AssetZoo>::failure(source, FormatResultCode::UnsupportedSchema,
                                               "asset zoo file magic marker is unsupported");
    }
    if (bytes[4] != edi::formats::ediMessagePackSupportedVersion) {
        return FormatResult<AssetZoo>::failure(source, FormatResultCode::UnsupportedVersion,
                                               "asset zoo file envelope version is unsupported");
    }
    const ByteBuffer payload(bytes.begin() + 5, bytes.end());
    FormatResult<MsgPackValue> decoded = edi::formats::decodeMessagePack(payload, source);
    if (!decoded.ok || !decoded.value) {
        return FormatResult<AssetZoo>::failure(source, decoded.code,
                                               decoded.message.empty() ? "asset zoo payload is malformed"
                                                                       : decoded.message);
    }
    return assetZooFromValue(*decoded.value);
}

} // namespace edi::zoo
