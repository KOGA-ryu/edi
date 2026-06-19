#pragma once

#include <string>
#include <vector>

namespace edi::zoo {

// The asset zoo is pillar A's backbone: a CURATED, cross-document library of
// reusable assets. Each AssetRecord is a thin INDEX — an id plus neutral
// references — NOT the asset's geometry. The 3D mesh is built ONCE by the geometry
// forge and lives as a Blender asset; the 2D proxy is a symbol the maker places;
// the record only points at them by opaque ref. "Build once, reference many" — the
// realizer INSTANCES the same `meshRef` at every placement instead of regenerating
// it each render.
//
// Why this is its own module, not a DraftingDocument vector (contrast DraftingBlock,
// which is per-document and stores its geometry BY VALUE): the zoo is a catalog
// that outlives any single drawing and persists to its own file. A drawing's
// `DraftingBlock.assetRef` / `BlockPlacementMetadata.assetRef` are today dangling
// free-form strings — this catalog is what those keys resolve against. Wiring that
// validation is a later slice; this slice establishes the catalog.
//
// NEUTRAL, by the three-tier law (edi records · Asset-Dex expands · engine moves):
// `category`, `meshRef`, and `proxyRef` are free-form strings in an OPEN vocabulary
// — edi records what an asset is called and what it points at without interpreting
// it (the same discipline as a plug's `type`). A closed enum would be a category
// list the user must keep extending, and would assert meaning edi has no business
// owning.

using AssetId = std::string; // opaque, minted "asset_0001" like object/block ids

// A LOCAL-asset-space 2D point — the socket's attach position relative to the
// asset's own origin, NOT a canvas/document coordinate. Deliberately a zoo-local
// type (not the drafting core's Point2D) so edi_zoo_core stays Qt/drafting-free.
struct Anchor2D { double x = 0.0; double y = 0.0; };

// A named NEUTRAL attach point — the generator/placement connection point.
// `type` is OPEN VOCAB (free-form string, e.g. "door"/"edge"), same neutrality
// law as category/meshRef — edi records the label without interpreting it.
struct AssetSocket { std::string name; std::string type; Anchor2D anchor; };

struct AssetRecord {
    AssetId id;                           // catalog key; a drawing's assetRef resolves to this
    std::string name;                     // authored label, e.g. "crypt_wall"
    std::string category;                 // neutral open vocab: "wall"/"floor"/"door"/...
    std::string meshRef;                  // the built 3D asset (Blender mesh/recipe) the realizer INSTANCES
    std::string proxyRef;                 // the 2D proxy symbol it places as (e.g. a block id); may be empty
    bool curated = false;                 // greenlight: only curated assets are "in" the zoo
    std::vector<std::string> textureRefs; // art-forge textures/materials applied to the asset
    std::vector<AssetSocket> sockets;     // named attach points (local-space); additive, no version bump
    // DEFERRED (additive — added with NO version bump, the wall_visual pattern):
    // the user-defined metadata "juice" grid.
};

struct AssetZoo {
    std::vector<AssetRecord> assets;
};

} // namespace edi::zoo
