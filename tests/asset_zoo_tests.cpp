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

#include "EdiAssert.h"
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
        EDI_CHECK(zoo.assets.empty());
        EDI_CHECK(highestAssetIdSerial(zoo) == 0);
        EDI_CHECK(mintAssetId(zoo) == "asset_0001");
    }

    // add mints sequentially and rejects empty/duplicate ids.
    {
        AssetZoo zoo;
        const AssetId a = mintAssetId(zoo);
        EDI_CHECK(addAsset(zoo, makeAssetRecord(a, "crypt_wall", "wall")));
        const AssetId b = mintAssetId(zoo); // resumes ABOVE the one just added
        EDI_CHECK(b == "asset_0002");
        EDI_CHECK(addAsset(zoo, makeAssetRecord(b, "stone_floor", "floor")));
        EDI_CHECK(zoo.assets.size() == 2);

        EDI_CHECK(!addAsset(zoo, makeAssetRecord("", "nameless", "wall"))); // empty id
        EDI_CHECK(!addAsset(zoo, makeAssetRecord(a, "dup", "wall")));       // duplicate id
        EDI_CHECK(zoo.assets.size() == 2);
    }

    // find / index / curate / remove by id; the curated view filters; mint never
    // reuses a removed serial.
    {
        AssetZoo zoo;
        const AssetId a = mintAssetId(zoo);
        addAsset(zoo, makeAssetRecord(a, "crypt_wall", "wall"));
        const AssetId b = mintAssetId(zoo);
        addAsset(zoo, makeAssetRecord(b, "stone_floor", "floor"));

        EDI_CHECK(findAsset(zoo, a) != nullptr);
        EDI_CHECK(findAsset(zoo, a)->name == "crypt_wall");
        EDI_CHECK(findAsset(zoo, "asset_9999") == nullptr);
        EDI_CHECK(assetIndexById(zoo, b).value() == 1);

        EDI_CHECK(curatedAssets(zoo).empty()); // nothing greenlit yet
        EDI_CHECK(curateAsset(zoo, a, true));
        EDI_CHECK(!curateAsset(zoo, "asset_9999", true)); // absent id
        const std::vector<const AssetRecord *> curated = curatedAssets(zoo);
        EDI_CHECK(curated.size() == 1);
        EDI_CHECK(curated.front()->id == a);

        EDI_CHECK(removeAsset(zoo, a));
        EDI_CHECK(!removeAsset(zoo, a)); // already gone
        EDI_CHECK(zoo.assets.size() == 1);
        // mint still resumes above the SURVIVING max serial (b), never reusing a.
        EDI_CHECK(mintAssetId(zoo) == "asset_0003");
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
        EDI_CHECK(addSocket(wall, AssetSocket{"north_door", "door", Anchor2D{1.5, -2.25}}));
        EDI_CHECK(addSocket(wall, AssetSocket{"east_edge", "edge", Anchor2D{4.0, 3.75}}));
        EDI_CHECK(addAsset(zoo, wall));

        const ByteBuffer bytes = encodeAssetZoo(zoo);
        const auto decoded = decodeAssetZoo(bytes, "round-trip");
        EDI_CHECK(decoded.ok && decoded.value);
        EDI_CHECK(decoded.value->assets.size() == 1);
        const AssetRecord &got = decoded.value->assets.front();
        EDI_CHECK(got.id == "asset_0001");
        EDI_CHECK(got.name == "crypt_wall");
        EDI_CHECK(got.category == "wall");
        EDI_CHECK(got.meshRef == "blender:crypt_wall.glb");
        EDI_CHECK(got.proxyRef == "block_0007");
        EDI_CHECK(got.curated);
        EDI_CHECK(got.textureRefs.size() == 2);
        EDI_CHECK(got.textureRefs[0] == "stone_diffuse");
        EDI_CHECK(got.textureRefs[1] == "stone_normal");

        EDI_CHECK(got.sockets.size() == 2);
        EDI_CHECK(got.sockets[0].name == "north_door");
        EDI_CHECK(got.sockets[0].type == "door");
        EDI_CHECK(got.sockets[0].anchor.x == 1.5);
        EDI_CHECK(got.sockets[0].anchor.y == -2.25);
        EDI_CHECK(got.sockets[1].name == "east_edge");
        EDI_CHECK(got.sockets[1].type == "edge");
        EDI_CHECK(got.sockets[1].anchor.x == 4.0);
        EDI_CHECK(got.sockets[1].anchor.y == 3.75);
    }

    // A record with no sockets round-trips to an empty socket vector (tolerant —
    // the missing/empty "sockets" key is never an error).
    {
        AssetZoo zoo;
        EDI_CHECK(addAsset(zoo, makeAssetRecord(mintAssetId(zoo), "bare_wall", "wall")));
        const ByteBuffer bytes = encodeAssetZoo(zoo);
        const auto decoded = decodeAssetZoo(bytes, "no-sockets");
        EDI_CHECK(decoded.ok && decoded.value);
        EDI_CHECK(decoded.value->assets.front().sockets.empty());
    }

    // addSocket: empty name rejected; duplicate name (within the record) rejected,
    // leaving the vector unchanged; a distinct name accepted.
    {
        AssetRecord record = makeAssetRecord("asset_0001", "crypt_wall", "wall");
        EDI_CHECK(!addSocket(record, AssetSocket{"", "door", Anchor2D{0.0, 0.0}})); // empty name
        EDI_CHECK(record.sockets.empty());
        EDI_CHECK(addSocket(record, AssetSocket{"north_door", "door", Anchor2D{1.0, 2.0}}));
        EDI_CHECK(record.sockets.size() == 1);
        EDI_CHECK(!addSocket(record, AssetSocket{"north_door", "edge", Anchor2D{3.0, 4.0}})); // dup
        EDI_CHECK(record.sockets.size() == 1);
        EDI_CHECK(addSocket(record, AssetSocket{"east_edge", "edge", Anchor2D{3.0, 4.0}}));
        EDI_CHECK(record.sockets.size() == 2);

        // findSocket: hit by name, nullptr for an absent name.
        const AssetSocket *hit = findSocket(record, "east_edge");
        EDI_CHECK(hit != nullptr);
        EDI_CHECK(hit->type == "edge");
        EDI_CHECK(hit->anchor.x == 3.0);
        EDI_CHECK(findSocket(record, "no_such_socket") == nullptr);
    }

    // An empty zoo round-trips to an empty zoo (the "no catalog yet" baseline).
    {
        const ByteBuffer bytes = encodeAssetZoo(AssetZoo{});
        const auto decoded = decodeAssetZoo(bytes, "empty");
        EDI_CHECK(decoded.ok && decoded.value);
        EDI_CHECK(decoded.value->assets.empty());
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
        EDI_CHECK(decoded.ok && decoded.value);
        const AssetRecord &got = decoded.value->assets.front();
        EDI_CHECK(got.id == "asset_0001");
        EDI_CHECK(got.name == "bare");
        EDI_CHECK(got.category.empty());
        EDI_CHECK(got.meshRef.empty());
        EDI_CHECK(got.proxyRef.empty());
        EDI_CHECK(!got.curated);
        EDI_CHECK(got.textureRefs.empty());
    }

    // A wrong schema, a wrong version, and bad magic are each rejected.
    {
        MsgPackValue wrongSchema = MsgPackValue::map({
            {"schema", MsgPackValue::text("edi.drawing")},
            {"version", MsgPackValue::integer(1)},
        });
        EDI_CHECK(!assetZooFromValue(wrongSchema).ok);

        MsgPackValue wrongVersion = MsgPackValue::map({
            {"schema", MsgPackValue::text("edi.zoo")},
            {"version", MsgPackValue::integer(999)},
        });
        EDI_CHECK(!assetZooFromValue(wrongVersion).ok);

        ByteBuffer badMagic = encodeAssetZoo(AssetZoo{});
        badMagic[0] = 'X';
        EDI_CHECK(!decodeAssetZoo(badMagic, "bad-magic").ok);
    }

    return 0;
}
