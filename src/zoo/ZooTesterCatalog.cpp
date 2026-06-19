#include "zoo/ZooTesterCatalog.h"

#include "zoo/AssetZooOps.h"

namespace edi::zoo {

std::vector<AssetRecord> testerAssetCatalog()
{
    std::vector<AssetRecord> catalog;

    // The six tester shapes (R1d-1, BREADTH). Each one repeats the IDENTICAL
    // proven path star6 walked depth-first: a compiled recipe under
    // samples/zoo/recipes/<shape>_ops_compiled.toml is baked ONCE to
    // samples/zoo/meshes/<shape>.blend (one mesh per asset), then curated here.
    // The catalog is pure DATA — a flat table of records, one row per shape —
    // because that is the contract the persist/manifest seam consumes; no logic,
    // no subclassing. The id is minted through the ops helper (assetIdForSerial)
    // rather than the literal "asset_000N", so the id FORMAT lives in one place —
    // if it ever changes, this catalog follows for free. meshRef is a
    // catalog-relative POSIX key (see header). proxyRef / textureRefs / sockets
    // stay empty on this pass; every record is curated (the manifest emits
    // curated-only).
    //
    // A tiny local emitter so the six rows read as one table, not six copy-paste
    // blocks. serial -> assetIdForSerial keeps the id format single-sourced.
    struct ShapeRow {
        const char *name;
        const char *category;
        const char *meshRef;
    };
    // Forge route per shape (see the recipes for the exact ops):
    //   star6    Script nfold_star, points=6 skip=5 (genuinely six-pointed)
    //   square   AddPrism, 4-vert square footprint
    //   circle   AddCylinder
    //   hexagon  AddPrism, 6-vert regular-hexagon footprint
    //   corner_l AddPrism, 6-vert L footprint
    //   ring     AddBoolean subtract: outer AddCylinder - inner AddCylinder
    const ShapeRow rows[] = {
        {"star6", "ornament", "meshes/star6.blend"},
        {"square", "floor", "meshes/square.blend"},
        {"circle", "column", "meshes/circle.blend"},
        {"hexagon", "column", "meshes/hexagon.blend"},
        {"corner_l", "wall", "meshes/corner_l.blend"},
        {"ring", "archway", "meshes/ring.blend"},
    };

    int serial = 1; // serials are 1-based, dense, in catalog order
    for (const ShapeRow &row : rows) {
        AssetRecord record;
        record.id = assetIdForSerial(serial++);
        record.name = row.name;
        record.category = row.category; // neutral open vocab — a label, not a dimension
        record.meshRef = row.meshRef;
        record.proxyRef = "";  // no 2D proxy on the first pass
        record.curated = true; // greenlit
        catalog.push_back(std::move(record));
    }

    return catalog;
}

} // namespace edi::zoo
