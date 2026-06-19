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

#include <cassert>
#include <optional>
#include <string>

using namespace edi::zoo;

int main()
{
    const std::vector<AssetRecord> catalog = testerAssetCatalog();

    // Exactly the six tester shapes, and every record is BOTH curated and
    // manifest-fit — the two preconditions the persist/manifest seam relies on
    // (the exporter emits curated-only and assumes zooManifestFieldProblem == nullopt).
    assert(catalog.size() == 6);
    for (const AssetRecord &record : catalog) {
        assert(record.curated);
        assert(!zooManifestFieldProblem(record).has_value());
    }

    // The expected six rows, in catalog order: serial (1-based) + identity fields
    // the realizer reads. Pinned as a table so a drift in any id/name/category/
    // meshRef is a hard failure.
    struct Expect {
        int serial;
        const char *name;
        const char *category;
        const char *meshRef;
    };
    const Expect expected[] = {
        {1, "star6", "ornament", "meshes/star6.blend"},
        {2, "square", "floor", "meshes/square.blend"},
        {3, "circle", "column", "meshes/circle.blend"},
        {4, "hexagon", "column", "meshes/hexagon.blend"},
        {5, "corner_l", "wall", "meshes/corner_l.blend"},
        {6, "ring", "archway", "meshes/ring.blend"},
    };

    for (std::size_t i = 0; i < catalog.size(); ++i) {
        const AssetRecord &record = catalog[i];
        const Expect &want = expected[i];
        assert(record.id == assetIdForSerial(want.serial)); // id format single-sourced
        assert(record.name == want.name);
        assert(record.category == want.category);
        assert(record.meshRef == want.meshRef);
    }

    // The manifest the realizer reads, built from the catalog. Byte-pin the header
    // (six curated records -> assets[6]{...}:) and the exact row for each shape, so
    // a drift in the row shape, a meshRef key, or the curated count is a hard failure.
    AssetZoo zoo;
    zoo.assets = catalog;
    const std::string manifest = exportZooToToon(zoo);

    const std::string expectedHeader =
        "assets[6]{id,name,category,meshRef,proxyRef,textures,sockets}:\n";
    assert(manifest.find(expectedHeader) != std::string::npos);

    const std::string expectedRows[] = {
        "  asset_0001,star6,ornament,meshes/star6.blend,\"\",\"\",\"\"\n",
        "  asset_0002,square,floor,meshes/square.blend,\"\",\"\",\"\"\n",
        "  asset_0003,circle,column,meshes/circle.blend,\"\",\"\",\"\"\n",
        "  asset_0004,hexagon,column,meshes/hexagon.blend,\"\",\"\",\"\"\n",
        "  asset_0005,corner_l,wall,meshes/corner_l.blend,\"\",\"\",\"\"\n",
        "  asset_0006,ring,archway,meshes/ring.blend,\"\",\"\",\"\"\n",
    };
    for (const std::string &row : expectedRows) {
        assert(manifest.find(row) != std::string::npos);
    }

    return 0;
}
