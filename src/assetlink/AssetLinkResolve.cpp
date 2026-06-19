#include "assetlink/AssetLinkResolve.h"

namespace edi::assetlink {

const edi::zoo::AssetRecord *resolveAssetRef(const edi::zoo::AssetZoo &zoo,
                                             const std::string &assetRef)
{
    // An empty ref is the "hand-drawn / no asset" sentinel — never a catalog miss to
    // report; short-circuit so it can't accidentally match an (also-empty) id.
    if (assetRef.empty()) {
        return nullptr;
    }
    return edi::zoo::findAsset(zoo, assetRef);
}

std::set<std::string> collectDocumentAssetRefs(const edi::drafting::DraftingDocument &document)
{
    // std::set gives sorted-unique for free, which is exactly the contract: dedups a
    // ref shared by a block and a placement, and yields a deterministic order for the
    // per-ref status list. The strings are COPIED in — the set outlives no document
    // alias, so it owns its keys.
    std::set<std::string> refs;

    // Every block-library definition carries the asset it depicts (empty = hand-drawn).
    for (const auto &block : document.blocks) {
        if (!block.assetRef.empty()) {
            refs.insert(block.assetRef);
        }
    }

    // A placement is an object whose blockPlacement has a non-empty instanceId; an
    // empty instanceId means an ordinary object (NOT a placement), so it is excluded.
    for (const auto &obj : document.objects) {
        const auto &placement = obj.metadata.blockPlacement;
        if (!placement.instanceId.empty() && !placement.assetRef.empty()) {
            refs.insert(placement.assetRef);
        }
    }

    return refs;
}

DocumentAssetRefValidation validateDocumentAssetRefs(const edi::drafting::DraftingDocument &document,
                                                     const edi::zoo::AssetZoo &zoo)
{
    DocumentAssetRefValidation validation;

    for (const std::string &ref : collectDocumentAssetRefs(document)) {
        const edi::zoo::AssetRecord *rec = resolveAssetRef(zoo, ref);
        // Read the aliasing pointer's curated flag into a bool IMMEDIATELY; we hold no
        // zoo pointer past this point, so a later zoo mutation can't dangle through us.
        const bool resolved = rec != nullptr;
        const bool curated = resolved && rec->curated;
        validation.refs.push_back(AssetRefStatus{ref, resolved, curated});

        if (!resolved) {
            validation.unresolved.push_back(ref); // dangling: points at nothing
        } else if (!curated) {
            validation.uncurated.push_back(ref);  // resolved but not greenlit
        }
    }

    return validation;
}

} // namespace edi::assetlink
