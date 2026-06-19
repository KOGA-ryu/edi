#include "zoo/ZooTesterCatalog.h"

#include "zoo/AssetZooOps.h"

namespace edi::zoo {

std::vector<AssetRecord> testerAssetCatalog()
{
    std::vector<AssetRecord> catalog;

    // star6 — the depth-first ornament. Forged from the nfold_star ScriptOp and
    // baked ONCE to samples/zoo/meshes/star6.blend (one mesh "girih.star_mesh").
    // The id is minted through the ops helper (assetIdForSerial) rather than the
    // literal "asset_0001", so the id FORMAT lives in one place — if it ever
    // changes, this catalog follows for free.
    AssetRecord star6;
    star6.id = assetIdForSerial(1);
    star6.name = "star6";
    star6.category = "ornament"; // neutral open vocab — a label, not a dimension
    star6.meshRef = "meshes/star6.blend"; // catalog-relative POSIX key (see header)
    star6.proxyRef = "";                  // no 2D proxy on the first pass
    star6.curated = true;                 // greenlit: the manifest emits curated-only
    // textureRefs / sockets left empty (optional on the first pass).
    catalog.push_back(std::move(star6));

    // R1d: append square/circle/hexagon/corner_l/ring here (the breadth slice).

    return catalog;
}

} // namespace edi::zoo
