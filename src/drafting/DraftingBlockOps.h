#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingStore.h" // DraftingStoreResult

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace edi::drafting {

// Phase C (block library) S1: the pure ops over the document's `blocks` vector.
// FREE FUNCTIONS over the document returning DraftingStoreResult — the exact
// shape addObject/addPlug use (same accepted()/rejected(), same
// ++revision-on-success) — so the command layer (S2) wraps them identically and
// they inherit DocumentSnapshot undo with no extra plumbing.
//
// NEUTRAL-ONLY, by VALUE: a block stores independent COPIES of its objects, so
// there is deliberately NO link back to the document objects it was built from
// and NO cascade on object delete — that referential independence is the whole
// point of the FLATTEN fork. (Contrast pruneGraphForRemovedObject, which a
// linked relation like a plug must carry.)

// Read helper — the block analogue of objectIndexById / plugIndexById.
std::optional<std::size_t> blockIndexById(const DraftingDocument &document, const DraftingBlockId &id);

// Build a normalized block definition from a set of source objects: union their
// bounds, translate every object so the union's lower-left lands at the origin
// (so a placement's offset is computed against a definition-local frame), recompute
// each member's bounds, and cache the origin-based union extent. The sources are
// COPIED (the block owns its geometry); the originals are untouched. An empty
// source list yields an empty block — addBlock is what rejects that.
// `assetRef` (Seam B) is the neutral asset/recipe the block depicts; empty for a
// purely hand-drawn block.
DraftingBlock buildBlockFromObjects(DraftingBlockId id, std::string name,
                                    const std::vector<DraftingObject> &sources,
                                    std::string assetRef = {});

// Mutations.
DraftingStoreResult addBlock(DraftingDocument &document, DraftingBlock block);
DraftingStoreResult removeBlock(DraftingDocument &document, const DraftingBlockId &id);

} // namespace edi::drafting
