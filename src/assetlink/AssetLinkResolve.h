#pragma once

// The asset-link bridge (slice A4): resolve a drawing's NEUTRAL assetRefs against
// the asset zoo catalog. This is the single named meeting-point of two deliberately
// isolated cores — edi_drafting_core and edi_zoo_core — neither of which may depend
// on the other. The resolver needs BOTH, so it lives in this higher layer that links
// both; the cross-link lives HERE, not inside either core.
//
// Resolution is a read-side query, not a type change: the document keeps assetRef as
// an opaque std::string and the zoo is untouched. We only LOOK UP what those neutral
// strings point at — equality by id, never interpreting the ref's content.

#include "drafting/DraftingDocument.h"
#include "zoo/AssetZoo.h"
#include "zoo/AssetZooOps.h"

#include <set>
#include <string>
#include <vector>

namespace edi::assetlink {

// Per-ref resolution status. curated is false whenever unresolved.
struct AssetRefStatus {
    std::string assetRef;
    bool resolved = false;
    bool curated = false;
};

// Whole-document validation: one status per DISTINCT used ref (sorted by assetRef),
// plus convenience lists. unresolved = dangling (points at nothing in the zoo);
// uncurated = resolved but not greenlit (usable-but-ungreenlit) — a softer warning.
struct DocumentAssetRefValidation {
    std::vector<AssetRefStatus> refs;
    std::vector<std::string> unresolved;
    std::vector<std::string> uncurated;
};

// Thin over findAsset; empty assetRef => nullptr. The returned pointer ALIASES the
// zoo and DANGLES if zoo.assets is later mutated (parity with findAsset/findSocket).
const edi::zoo::AssetRecord *resolveAssetRef(const edi::zoo::AssetZoo &zoo,
                                             const std::string &assetRef);

// Sorted-unique set of the NON-EMPTY assetRefs a document uses: every block's
// assetRef, plus each placement's assetRef where instanceId is non-empty.
std::set<std::string> collectDocumentAssetRefs(const edi::drafting::DraftingDocument &document);

// Per-ref status + convenience unresolved/uncurated lists.
DocumentAssetRefValidation validateDocumentAssetRefs(const edi::drafting::DraftingDocument &document,
                                                     const edi::zoo::AssetZoo &zoo);

} // namespace edi::assetlink
