// Asset zoo (pillar A) — realize chain R1d: the curated six-shape tester catalog.
//
// Pins the DATA the forge->bake->mint->persist->manifest chain hands the realizer:
// that testerAssetCatalog() yields the SIX curated, manifest-fit tester shapes
// (breadth slice R1d-1), each carrying the exact id/name/category/meshRef the
// realizer resolves, and that exportZooToToon over the catalog emits all six
// curated rows with their catalog-relative meshRefs. Pure int main()+assert, links
// edi_zoo_core ONLY (no Qt, no drafting) — the same isolation the other zoo
// slices hold.
#include "zoo/AssetZoo.h"
#include "zoo/AssetZooOps.h"
#include "zoo/ZooTesterCatalog.h"
#include "zoo/ZooToonExport.h"

#include "EdiAssert.h"
#include <optional>
#include <string>

using namespace edi::zoo;

int main()
{
    const std::vector<AssetRecord> catalog = testerAssetCatalog();

    // Exactly the six tester shapes PLUS the three tree variants (L5b forest
    // payoff) PLUS the six container assets (game-asset library batch 1), and
    // every record is BOTH curated and manifest-fit — the two preconditions the
    // persist/manifest seam relies on (the exporter emits curated-only and assumes
    // zooManifestFieldProblem == nullopt; the tree texture names bark_brown/
    // leaf_green are reserved-char-clean and the container rows carry none).
    EDI_CHECK(catalog.size() == 15);
    for (const AssetRecord &record : catalog) {
        EDI_CHECK(record.curated);
        EDI_CHECK(!zooManifestFieldProblem(record).has_value());
    }

    // The expected fifteen rows, in catalog order: serial (1-based) + identity fields
    // the realizer reads, plus the two slot-colour texture names on the trees
    // (empty on the shapes). Pinned as a table so a drift in any id/name/category/
    // meshRef/textureRefs is a hard failure.
    struct Expect {
        int serial;
        const char *name;
        const char *category;
        const char *meshRef;
        const char *texture0; // nullptr -> no texture refs (the shapes)
        const char *texture1;
    };
    const Expect expected[] = {
        {1, "star6", "ornament", "meshes/star6.blend", nullptr, nullptr},
        {2, "square", "floor", "meshes/square.blend", nullptr, nullptr},
        {3, "circle", "column", "meshes/circle.blend", nullptr, nullptr},
        {4, "hexagon", "column", "meshes/hexagon.blend", nullptr, nullptr},
        {5, "corner_l", "wall", "meshes/corner_l.blend", nullptr, nullptr},
        {6, "ring", "archway", "meshes/ring.blend", nullptr, nullptr},
        {7, "tree_a", "tree", "meshes/tree_a.blend", "bark_brown", "leaf_green"},
        {8, "tree_b", "tree", "meshes/tree_b.blend", "bark_brown", "leaf_green"},
        {9, "tree_c", "tree", "meshes/tree_c.blend", "bark_brown", "leaf_green"},
        {10, "barrel", "container", "meshes/barrel.blend", nullptr, nullptr},
        {11, "crate", "container", "meshes/crate.blend", nullptr, nullptr},
        {12, "chest", "container", "meshes/chest.blend", nullptr, nullptr},
        {13, "sack", "container", "meshes/sack.blend", nullptr, nullptr},
        {14, "pot", "container", "meshes/pot.blend", nullptr, nullptr},
        {15, "bucket", "container", "meshes/bucket.blend", nullptr, nullptr},
    };

    for (std::size_t i = 0; i < catalog.size(); ++i) {
        const AssetRecord &record = catalog[i];
        const Expect &want = expected[i];
        EDI_CHECK(record.id == assetIdForSerial(want.serial)); // id format single-sourced
        EDI_CHECK(record.name == want.name);
        EDI_CHECK(record.category == want.category);
        EDI_CHECK(record.meshRef == want.meshRef);
        if (want.texture0 == nullptr) {
            EDI_CHECK(record.textureRefs.empty());
        } else {
            EDI_CHECK(record.textureRefs.size() == 2);
            EDI_CHECK(record.textureRefs[0] == want.texture0); // baked-slot order
            EDI_CHECK(record.textureRefs[1] == want.texture1);
        }
    }

    // The manifest the realizer reads, built from the catalog. Byte-pin the header
    // (fifteen curated records -> assets[15]{...}:) and the exact row for each asset,
    // so a drift in the row shape, a meshRef key, or the curated count is a hard fail.
    AssetZoo zoo;
    zoo.assets = catalog;
    const std::string manifest = exportZooToToon(zoo);

    const std::string expectedHeader =
        "assets[15]{id,name,category,meshRef,proxyRef,textures,sockets}:\n";
    EDI_CHECK(manifest.find(expectedHeader) != std::string::npos);

    // The tree rows carry the two slot colours as a `·`-joined run in the textures
    // cell (bark_brown·leaf_green). The run has no comma/space, so cell() leaves it
    // BARE — byte-pin that exact unquoted run so a join/order drift is a hard fail.
    const std::string expectedRows[] = {
        "  asset_0001,star6,ornament,meshes/star6.blend,\"\",\"\",\"\"\n",
        "  asset_0002,square,floor,meshes/square.blend,\"\",\"\",\"\"\n",
        "  asset_0003,circle,column,meshes/circle.blend,\"\",\"\",\"\"\n",
        "  asset_0004,hexagon,column,meshes/hexagon.blend,\"\",\"\",\"\"\n",
        "  asset_0005,corner_l,wall,meshes/corner_l.blend,\"\",\"\",\"\"\n",
        "  asset_0006,ring,archway,meshes/ring.blend,\"\",\"\",\"\"\n",
        "  asset_0007,tree_a,tree,meshes/tree_a.blend,\"\",bark_brown·leaf_green,\"\"\n",
        "  asset_0008,tree_b,tree,meshes/tree_b.blend,\"\",bark_brown·leaf_green,\"\"\n",
        "  asset_0009,tree_c,tree,meshes/tree_c.blend,\"\",bark_brown·leaf_green,\"\"\n",
        // The six container rows carry no texture override (empty cell), matching
        // the shape rows' shape.
        "  asset_0010,barrel,container,meshes/barrel.blend,\"\",\"\",\"\"\n",
        "  asset_0011,crate,container,meshes/crate.blend,\"\",\"\",\"\"\n",
        "  asset_0012,chest,container,meshes/chest.blend,\"\",\"\",\"\"\n",
        "  asset_0013,sack,container,meshes/sack.blend,\"\",\"\",\"\"\n",
        "  asset_0014,pot,container,meshes/pot.blend,\"\",\"\",\"\"\n",
        "  asset_0015,bucket,container,meshes/bucket.blend,\"\",\"\",\"\"\n",
    };
    for (const std::string &row : expectedRows) {
        EDI_CHECK(manifest.find(row) != std::string::npos);
    }

    return 0;
}
