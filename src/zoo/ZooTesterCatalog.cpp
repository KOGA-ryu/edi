#include "zoo/ZooTesterCatalog.h"

#include "zoo/AssetZooOps.h"

namespace edi::zoo {

std::vector<AssetRecord> testerAssetCatalog()
{
    std::vector<AssetRecord> catalog;

    // The six tester shapes (R1d-1, BREADTH) plus three TREE variants (L5b, the
    // forest payoff). Each one repeats the IDENTICAL proven path star6 walked
    // depth-first: a compiled recipe under
    // samples/zoo/recipes/<shape>_ops_compiled.toml is baked ONCE to
    // samples/zoo/meshes/<shape>.blend (one mesh per asset), then curated here.
    // The catalog is pure DATA — a flat table of records, one row per shape —
    // because that is the contract the persist/manifest seam consumes; no logic,
    // no subclassing. The id is minted through the ops helper (assetIdForSerial)
    // rather than the literal "asset_000N", so the id FORMAT lives in one place —
    // if it ever changes, this catalog follows for free. meshRef is a
    // catalog-relative POSIX key (see header). proxyRef / sockets stay empty;
    // textureRefs is empty on the shapes and carries the two slot-colour
    // appearance names on the trees (see the tree rows). Every record is curated
    // (the manifest emits curated-only).
    //
    // A tiny local emitter so the rows read as one table, not many copy-paste
    // blocks. serial -> assetIdForSerial keeps the id format single-sourced. The
    // optional texture columns carry the realizer's per-slot APPEARANCE override:
    // a tree's two baked material slots (slot0 bark / slot1 leaf, from the L4 face
    // split) are coloured IN ORDER by the placement's textures, so a tree row
    // ships {"bark_brown","leaf_green"} to drive a brown trunk + green canopy at
    // render time. The shapes carry none (their bronze/grey default reads fine).
    struct ShapeRow {
        const char *name;
        const char *category;
        const char *meshRef;
        // Empty -> no texture override (the shape rows). A tree row lists its two
        // slot colours in baked-slot order. nullptr terminates so the table can
        // mix 0- and 2-texture rows without a length column.
        const char *texture0;
        const char *texture1;
    };
    // Forge route per shape (see the recipes for the exact ops):
    //   star6    Script nfold_star, points=6 skip=5 (genuinely six-pointed)
    //   square   AddPrism, 4-vert square footprint
    //   circle   AddCylinder
    //   hexagon  AddPrism, 6-vert regular-hexagon footprint
    //   corner_l AddPrism, 6-vert L footprint
    //   ring     AddBoolean subtract: outer AddCylinder - inner AddCylinder
    // Forge route per tree (see the recipes for the exact ops):
    //   tree_a   Script tree, seed 1 — full broadleaf (the manifest defaults)
    //   tree_b   Script tree, seed 2 — sparser + taller (fewer levels/clumps)
    //   tree_c   Script tree, seed 3 — fuller + rounder (more/bigger clumps)
    // The three are ONE craftsman with three seeds + a fuller-vs-sparser spread:
    // build-once-place-many, a forest from three datablocks + seed variation.
    // Each carries {"bark_brown","leaf_green"} so the realizer's _instance_material
    // overrides the two baked slots IN ORDER (slot0 structure->brown, slot1
    // canopy->green) and the forest reads green. These are the realizer's OWN
    // appearance palette names (GREY_MATERIALS), not the edi_craft MATERIALS table.
    const ShapeRow rows[] = {
        {"star6", "ornament", "meshes/star6.blend", nullptr, nullptr},
        {"square", "floor", "meshes/square.blend", nullptr, nullptr},
        {"circle", "column", "meshes/circle.blend", nullptr, nullptr},
        {"hexagon", "column", "meshes/hexagon.blend", nullptr, nullptr},
        {"corner_l", "wall", "meshes/corner_l.blend", nullptr, nullptr},
        {"ring", "archway", "meshes/ring.blend", nullptr, nullptr},
        {"tree_a", "tree", "meshes/tree_a.blend", "bark_brown", "leaf_green"},
        {"tree_b", "tree", "meshes/tree_b.blend", "bark_brown", "leaf_green"},
        {"tree_c", "tree", "meshes/tree_c.blend", "bark_brown", "leaf_green"},
        // CONTAINER BATCH (game-asset library batch 1) — six pre-authored
        // container craftsmen integrated through the SAME forge->bake->curate path:
        // each is a single Script op invoking the craftsman at its MANIFEST
        // defaults, baked once to meshes/<name>.blend, dual-tier parity confirmed
        // (proof_mesh vert/face counts == baked mesh counts). They share one
        // neutral category="container" and carry no texture override (the
        // realizer's grey default reads fine — they are not slot-split like trees),
        // so these rows ship 0 textures, keeping zooManifestFieldProblem nullopt.
        {"barrel", "container", "meshes/barrel.blend", nullptr, nullptr},
        {"crate", "container", "meshes/crate.blend", nullptr, nullptr},
        {"chest", "container", "meshes/chest.blend", nullptr, nullptr},
        {"sack", "container", "meshes/sack.blend", nullptr, nullptr},
        {"pot", "container", "meshes/pot.blend", nullptr, nullptr},
        {"bucket", "container", "meshes/bucket.blend", nullptr, nullptr},
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
        // A tree row ships its two slot-colour appearance names (in baked-slot
        // order); a shape row ships none. The names are reserved-char-clean, so
        // zooManifestFieldProblem stays nullopt (asserted in the catalog test).
        if (row.texture0 != nullptr) {
            record.textureRefs.emplace_back(row.texture0);
        }
        if (row.texture1 != nullptr) {
            record.textureRefs.emplace_back(row.texture1);
        }
        catalog.push_back(std::move(record));
    }

    return catalog;
}

} // namespace edi::zoo
