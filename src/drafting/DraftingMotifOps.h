#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingStore.h" // DraftingStoreResult

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace edi::drafting {

// M8-S1: pure ops over the document's `motifs` vector.
//
// A motif is the CORE twin of DraftingBlock — name-keyed, no id, no assetRef.
// The same FREE-FUNCTION + DraftingStoreResult shape as DraftingBlockOps lets the
// command layer (CreateMotifCommand) wrap these with one-line delegates and inherit
// DocumentSnapshot undo for free — no extra plumbing.
//
// NEUTRAL-ONLY, by VALUE: a motif stores COPIES of its objects, so there is
// deliberately no link back to the source document objects — the same referential
// independence the block library uses.

// Read helper — find a motif by name (the unique key).
std::optional<std::size_t> motifIndexByName(const DraftingDocument &document,
                                             const std::string &name);

// Build a normalized motif from a set of source objects.
//
// Mirrors buildBlockFromObjects with two differences:
//   1. Guide objects are EXCLUDED from the captured set — a guide has no
//      re-droppable local position so including it in a template is meaningless.
//      ConstructionLines and every other kind are included (they translate fine).
//   2. `locked` and `visible` are reset to false/true on each template object
//      because template state must not leak instance state to a future placement.
//
// The union bounds are computed from GEOMETRY (not the possibly-stale cached box),
// the entire set is translated so its lower-left lands at (0,0), each member's
// `bounds` is recomputed, and `motif.bounds` is set to the origin-based extent.
// An empty source list (or all-guides list) yields an empty-objects motif —
// addMotif is what rejects that.
DraftingMotif buildMotifFromObjects(std::string name,
                                    const std::vector<DraftingObject> &sources);

// Mutations.
DraftingStoreResult addMotif(DraftingDocument &document, DraftingMotif motif);
DraftingStoreResult removeMotif(DraftingDocument &document, const std::string &name);

} // namespace edi::drafting
