// Asset zoo (pillar A) — realize chain R1c: the curated tester catalog.
//
// Pins the DATA the forge→bake→mint→persist→manifest chain hands the realizer:
// that testerAssetCatalog() yields ONLY curated, manifest-fit records, that the
// depth-first star6 record carries the exact id/name/category/meshRef the
// realizer resolves, and that exportZooToToon over the catalog emits the curated
// star6 row with its catalog-relative meshRef. Pure int main()+assert, links
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

    // Non-empty, and every record is BOTH curated and manifest-fit — the two
    // preconditions the persist/manifest seam relies on (the exporter emits
    // curated-only and assumes zooManifestFieldProblem == nullopt).
    assert(!catalog.empty());
    for (const AssetRecord &record : catalog) {
        assert(record.curated);
        assert(!zooManifestFieldProblem(record).has_value());
    }

    // The depth-first star6 record, by EXACT identity fields the realizer reads.
    const AssetRecord *star6 = nullptr;
    for (const AssetRecord &record : catalog) {
        if (record.name == "star6") {
            star6 = &record;
            break;
        }
    }
    assert(star6 != nullptr);
    assert(star6->id == assetIdForSerial(1)); // "asset_0001", via the ops helper
    assert(star6->category == "ornament");
    assert(star6->meshRef == "meshes/star6.blend"); // catalog-relative POSIX key

    // The manifest the realizer reads, built from the catalog. Byte-pin the header
    // (one curated record -> assets[1]{...}:) and the exact star6 row, so a drift
    // in the row shape, the meshRef key, or the curated count is a hard failure.
    AssetZoo zoo;
    zoo.assets = catalog;
    const std::string manifest = exportZooToToon(zoo);

    const std::string expectedHeader =
        "assets[1]{id,name,category,meshRef,proxyRef,textures,sockets}:\n";
    assert(manifest.find(expectedHeader) != std::string::npos);

    const std::string expectedStar6Row =
        "  asset_0001,star6,ornament,meshes/star6.blend,\"\",\"\",\"\"\n";
    assert(manifest.find(expectedStar6Row) != std::string::npos);

    return 0;
}
