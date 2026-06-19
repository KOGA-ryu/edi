// Asset zoo (pillar A) — slice A1: the data model + ops + MessagePack codec.
//
// Asserts the LAYOUT (a fresh zoo is empty), the OPS (mint resumes above existing
// ids and never reuses one, add rejects empty/duplicate ids, find/index/curate/
// remove by id, the curated view filters), and the CODEC (a full record round-trips
// through the shared EDIM envelope; reads are additive + tolerant; a wrong schema,
// version, or magic is rejected). Pure int main()+assert, like the other core
// slices — the zoo core links no Qt.
#include "zoo/AssetZoo.h"
#include "zoo/AssetZooOps.h"
#include "zoo/AssetZooSerialize.h"

#include <cassert>
#include <string>
#include <vector>

using namespace edi::zoo;
using edi::formats::ByteBuffer;
using edi::formats::MsgPackValue;

int main()
{
    // S0: a fresh zoo is empty; its serial starts at 0, so the first mint is _0001.
    {
        AssetZoo zoo;
        assert(zoo.assets.empty());
        assert(highestAssetIdSerial(zoo) == 0);
        assert(mintAssetId(zoo) == "asset_0001");
    }

    // add mints sequentially and rejects empty/duplicate ids.
    {
        AssetZoo zoo;
        const AssetId a = mintAssetId(zoo);
        assert(addAsset(zoo, makeAssetRecord(a, "crypt_wall", "wall")));
        const AssetId b = mintAssetId(zoo); // resumes ABOVE the one just added
        assert(b == "asset_0002");
        assert(addAsset(zoo, makeAssetRecord(b, "stone_floor", "floor")));
        assert(zoo.assets.size() == 2);

        assert(!addAsset(zoo, makeAssetRecord("", "nameless", "wall"))); // empty id
        assert(!addAsset(zoo, makeAssetRecord(a, "dup", "wall")));       // duplicate id
        assert(zoo.assets.size() == 2);
    }

    // find / index / curate / remove by id; the curated view filters; mint never
    // reuses a removed serial.
    {
        AssetZoo zoo;
        const AssetId a = mintAssetId(zoo);
        addAsset(zoo, makeAssetRecord(a, "crypt_wall", "wall"));
        const AssetId b = mintAssetId(zoo);
        addAsset(zoo, makeAssetRecord(b, "stone_floor", "floor"));

        assert(findAsset(zoo, a) != nullptr);
        assert(findAsset(zoo, a)->name == "crypt_wall");
        assert(findAsset(zoo, "asset_9999") == nullptr);
        assert(assetIndexById(zoo, b).value() == 1);

        assert(curatedAssets(zoo).empty()); // nothing greenlit yet
        assert(curateAsset(zoo, a, true));
        assert(!curateAsset(zoo, "asset_9999", true)); // absent id
        const std::vector<const AssetRecord *> curated = curatedAssets(zoo);
        assert(curated.size() == 1);
        assert(curated.front()->id == a);

        assert(removeAsset(zoo, a));
        assert(!removeAsset(zoo, a)); // already gone
        assert(zoo.assets.size() == 1);
        // mint still resumes above the SURVIVING max serial (b), never reusing a.
        assert(mintAssetId(zoo) == "asset_0003");
    }

    // A full record round-trips through the EDIM envelope, every field preserved —
    // including sockets, field-by-field (name, type, anchor.x, anchor.y).
    {
        AssetZoo zoo;
        AssetRecord wall = makeAssetRecord(mintAssetId(zoo), "crypt_wall", "wall");
        wall.meshRef = "blender:crypt_wall.glb";
        wall.proxyRef = "block_0007";
        wall.curated = true;
        wall.textureRefs = {"stone_diffuse", "stone_normal"};
        assert(addSocket(wall, AssetSocket{"north_door", "door", Anchor2D{1.5, -2.25}}));
        assert(addSocket(wall, AssetSocket{"east_edge", "edge", Anchor2D{4.0, 3.75}}));
        assert(addAsset(zoo, wall));

        const ByteBuffer bytes = encodeAssetZoo(zoo);
        const auto decoded = decodeAssetZoo(bytes, "round-trip");
        assert(decoded.ok && decoded.value);
        assert(decoded.value->assets.size() == 1);
        const AssetRecord &got = decoded.value->assets.front();
        assert(got.id == "asset_0001");
        assert(got.name == "crypt_wall");
        assert(got.category == "wall");
        assert(got.meshRef == "blender:crypt_wall.glb");
        assert(got.proxyRef == "block_0007");
        assert(got.curated);
        assert(got.textureRefs.size() == 2);
        assert(got.textureRefs[0] == "stone_diffuse");
        assert(got.textureRefs[1] == "stone_normal");

        assert(got.sockets.size() == 2);
        assert(got.sockets[0].name == "north_door");
        assert(got.sockets[0].type == "door");
        assert(got.sockets[0].anchor.x == 1.5);
        assert(got.sockets[0].anchor.y == -2.25);
        assert(got.sockets[1].name == "east_edge");
        assert(got.sockets[1].type == "edge");
        assert(got.sockets[1].anchor.x == 4.0);
        assert(got.sockets[1].anchor.y == 3.75);
    }

    // A record with no sockets round-trips to an empty socket vector (tolerant —
    // the missing/empty "sockets" key is never an error).
    {
        AssetZoo zoo;
        assert(addAsset(zoo, makeAssetRecord(mintAssetId(zoo), "bare_wall", "wall")));
        const ByteBuffer bytes = encodeAssetZoo(zoo);
        const auto decoded = decodeAssetZoo(bytes, "no-sockets");
        assert(decoded.ok && decoded.value);
        assert(decoded.value->assets.front().sockets.empty());
    }

    // addSocket: empty name rejected; duplicate name (within the record) rejected,
    // leaving the vector unchanged; a distinct name accepted.
    {
        AssetRecord record = makeAssetRecord("asset_0001", "crypt_wall", "wall");
        assert(!addSocket(record, AssetSocket{"", "door", Anchor2D{0.0, 0.0}})); // empty name
        assert(record.sockets.empty());
        assert(addSocket(record, AssetSocket{"north_door", "door", Anchor2D{1.0, 2.0}}));
        assert(record.sockets.size() == 1);
        assert(!addSocket(record, AssetSocket{"north_door", "edge", Anchor2D{3.0, 4.0}})); // dup
        assert(record.sockets.size() == 1);
        assert(addSocket(record, AssetSocket{"east_edge", "edge", Anchor2D{3.0, 4.0}}));
        assert(record.sockets.size() == 2);

        // findSocket: hit by name, nullptr for an absent name.
        const AssetSocket *hit = findSocket(record, "east_edge");
        assert(hit != nullptr);
        assert(hit->type == "edge");
        assert(hit->anchor.x == 3.0);
        assert(findSocket(record, "no_such_socket") == nullptr);
    }

    // An empty zoo round-trips to an empty zoo (the "no catalog yet" baseline).
    {
        const ByteBuffer bytes = encodeAssetZoo(AssetZoo{});
        const auto decoded = decodeAssetZoo(bytes, "empty");
        assert(decoded.ok && decoded.value);
        assert(decoded.value->assets.empty());
    }

    // Tolerant read: a record map missing the optional keys defaults gracefully
    // (additive — a future field absent in an older file is not an error).
    {
        MsgPackValue sparseAsset = MsgPackValue::map({
            {"id", MsgPackValue::text("asset_0001")},
            {"name", MsgPackValue::text("bare")},
            // no category / mesh_ref / proxy_ref / curated / texture_refs
        });
        MsgPackValue root = MsgPackValue::map({
            {"schema", MsgPackValue::text("edi.zoo")},
            {"version", MsgPackValue::integer(1)},
            {"zoo", MsgPackValue::map({
                {"assets", MsgPackValue::array({sparseAsset})},
            })},
        });
        const auto decoded = assetZooFromValue(root);
        assert(decoded.ok && decoded.value);
        const AssetRecord &got = decoded.value->assets.front();
        assert(got.id == "asset_0001");
        assert(got.name == "bare");
        assert(got.category.empty());
        assert(got.meshRef.empty());
        assert(got.proxyRef.empty());
        assert(!got.curated);
        assert(got.textureRefs.empty());
    }

    // A wrong schema, a wrong version, and bad magic are each rejected.
    {
        MsgPackValue wrongSchema = MsgPackValue::map({
            {"schema", MsgPackValue::text("edi.drawing")},
            {"version", MsgPackValue::integer(1)},
        });
        assert(!assetZooFromValue(wrongSchema).ok);

        MsgPackValue wrongVersion = MsgPackValue::map({
            {"schema", MsgPackValue::text("edi.zoo")},
            {"version", MsgPackValue::integer(999)},
        });
        assert(!assetZooFromValue(wrongVersion).ok);

        ByteBuffer badMagic = encodeAssetZoo(AssetZoo{});
        badMagic[0] = 'X';
        assert(!decodeAssetZoo(badMagic, "bad-magic").ok);
    }

    return 0;
}
