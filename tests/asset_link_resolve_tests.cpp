// Asset-link bridge (slice A4) — the assetRef -> catalog RESOLVER.
//
// Asserts the bridge READS both cores correctly without changing either: a neutral
// assetRef resolves (hit/miss/empty) against the zoo, the document's USED refs are
// collected sorted-unique from blocks + placements (excluding ordinary objects and
// empty refs), dangling refs surface as unresolved, and the curated flag rides
// through. Pure int main()+assert — this test links only edi_asset_link, which pulls
// both cores + format_core transitively; no Qt.
#include "assetlink/AssetLinkResolve.h"

#include "drafting/DraftingDocument.h"
#include "zoo/AssetZoo.h"
#include "zoo/AssetZooOps.h"

#include "EdiAssert.h"
#include <set>
#include <string>
#include <vector>

using namespace edi::assetlink;
using edi::drafting::DraftingBlock;
using edi::drafting::DraftingDocument;
using edi::drafting::DraftingObject;
using edi::drafting::makeDraftingDocument;
using edi::zoo::addAsset;
using edi::zoo::AssetId;
using edi::zoo::AssetZoo;
using edi::zoo::curateAsset;
using edi::zoo::makeAssetRecord;
using edi::zoo::mintAssetId;

namespace {

// Helper: does a status list contain a given ref?
bool hasRef(const std::vector<std::string> &list, const std::string &ref)
{
    for (const std::string &r : list) {
        if (r == ref) {
            return true;
        }
    }
    return false;
}

// Helper: find a per-ref status by assetRef (nullptr if absent).
const AssetRefStatus *statusFor(const DocumentAssetRefValidation &v, const std::string &ref)
{
    for (const AssetRefStatus &s : v.refs) {
        if (s.assetRef == ref) {
            return &s;
        }
    }
    return nullptr;
}

} // namespace

int main()
{
    // Build a small zoo: two assets, one curated (greenlit), one not.
    AssetZoo zoo;
    const AssetId curatedId = mintAssetId(zoo);
    EDI_CHECK(addAsset(zoo, makeAssetRecord(curatedId, "crypt_wall", "wall")));
    const AssetId uncuratedId = mintAssetId(zoo);
    EDI_CHECK(addAsset(zoo, makeAssetRecord(uncuratedId, "stone_floor", "floor")));
    EDI_CHECK(curateAsset(zoo, curatedId, true)); // only the wall is greenlit

    // resolveAssetRef: hit returns the right record; unknown and EMPTY both nullptr.
    {
        const edi::zoo::AssetRecord *hit = resolveAssetRef(zoo, curatedId);
        EDI_CHECK(hit != nullptr);
        EDI_CHECK(hit->id == curatedId);
        EDI_CHECK(hit->name == "crypt_wall");
        EDI_CHECK(resolveAssetRef(zoo, "asset_9999") == nullptr); // dangling
        EDI_CHECK(resolveAssetRef(zoo, "") == nullptr);           // empty sentinel
    }

    // collect from blocks + placements + dedup. The doc uses:
    //  - a block depicting the curated asset
    //  - a block depicting the uncurated asset
    //  - a hand-drawn block (empty assetRef) -> EXCLUDED
    //  - a placement (non-empty instanceId) re-using the curated ref -> DEDUPED
    //  - a placement depicting a dangling ref
    //  - an ordinary object (EMPTY instanceId) with an assetRef -> EXCLUDED
    DraftingDocument doc = makeDraftingDocument("doc_0001", "asset-link");

    DraftingBlock blockCurated;
    blockCurated.id = "block_0001";
    blockCurated.assetRef = curatedId;
    doc.blocks.push_back(blockCurated);

    DraftingBlock blockUncurated;
    blockUncurated.id = "block_0002";
    blockUncurated.assetRef = uncuratedId;
    doc.blocks.push_back(blockUncurated);

    DraftingBlock blockHandDrawn;
    blockHandDrawn.id = "block_0003";
    blockHandDrawn.assetRef = ""; // hand-drawn — excluded
    doc.blocks.push_back(blockHandDrawn);

    // Placement re-using the curated ref (dedup target).
    DraftingObject placementDup;
    placementDup.id = "obj_0001";
    placementDup.metadata.blockPlacement.assetRef = curatedId;
    placementDup.metadata.blockPlacement.instanceId = "inst_0001";
    doc.objects.push_back(placementDup);

    // Placement depicting a ref absent from the zoo (dangling).
    const std::string danglingRef = "asset_4242";
    DraftingObject placementDangling;
    placementDangling.id = "obj_0002";
    placementDangling.metadata.blockPlacement.assetRef = danglingRef;
    placementDangling.metadata.blockPlacement.instanceId = "inst_0002";
    doc.objects.push_back(placementDangling);

    // Ordinary object: EMPTY instanceId but a non-empty assetRef -> NOT a placement.
    DraftingObject ordinary;
    ordinary.id = "obj_0003";
    ordinary.metadata.blockPlacement.assetRef = "asset_should_be_ignored";
    ordinary.metadata.blockPlacement.instanceId = ""; // empty -> excluded
    doc.objects.push_back(ordinary);

    {
        const std::set<std::string> refs = collectDocumentAssetRefs(doc);
        // curatedId (block + placement, deduped), uncuratedId (block), danglingRef
        // (placement). Hand-drawn block and ordinary object excluded.
        const std::set<std::string> expected{curatedId, uncuratedId, danglingRef};
        EDI_CHECK(refs == expected);
        EDI_CHECK(refs.size() == 3);
        EDI_CHECK(refs.count("asset_should_be_ignored") == 0);
        EDI_CHECK(refs.count("") == 0);
    }

    // validateDocumentAssetRefs: status per distinct ref, sorted by assetRef.
    {
        const DocumentAssetRefValidation v = validateDocumentAssetRefs(doc, zoo);
        EDI_CHECK(v.refs.size() == 3);
        // refs are sorted by assetRef (std::set order): asset_0001, asset_0002, asset_4242.
        EDI_CHECK(v.refs[0].assetRef == curatedId);
        EDI_CHECK(v.refs[1].assetRef == uncuratedId);
        EDI_CHECK(v.refs[2].assetRef == danglingRef);

        // Curated asset: resolved && curated, in neither warning list.
        const AssetRefStatus *cur = statusFor(v, curatedId);
        EDI_CHECK(cur != nullptr);
        EDI_CHECK(cur->resolved && cur->curated);
        EDI_CHECK(!hasRef(v.unresolved, curatedId));
        EDI_CHECK(!hasRef(v.uncurated, curatedId));

        // Uncurated (resolved) asset: resolved && !curated, appears in uncurated only.
        const AssetRefStatus *unc = statusFor(v, uncuratedId);
        EDI_CHECK(unc != nullptr);
        EDI_CHECK(unc->resolved && !unc->curated);
        EDI_CHECK(!hasRef(v.unresolved, uncuratedId));
        EDI_CHECK(hasRef(v.uncurated, uncuratedId));

        // Dangling ref: resolved == false (curated false too), appears in unresolved only.
        const AssetRefStatus *dang = statusFor(v, danglingRef);
        EDI_CHECK(dang != nullptr);
        EDI_CHECK(!dang->resolved && !dang->curated);
        EDI_CHECK(hasRef(v.unresolved, danglingRef));
        EDI_CHECK(!hasRef(v.uncurated, danglingRef));

        // Exactly one of each warning kind.
        EDI_CHECK(v.unresolved.size() == 1);
        EDI_CHECK(v.uncurated.size() == 1);
    }

    return 0;
}
