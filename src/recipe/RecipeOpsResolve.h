#pragma once

#include "recipe/RecipeOps.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <string>
#include <vector>

namespace edi::recipe {

// One finding per FAILED binding: the op-field it targeted and why it could
// not resolve. The key composes the binding's address (op.<index>.<fieldKey>),
// the same coordinate the store names a binding by, so the UI can point at
// exactly which field went stale. The message is the shared measurement
// seam's own wording (RecipeMeasure / the B01 contract), or the writer's
// "not a bindable field" family when a hand-built stream binds a key the
// registry rejects.
struct OpResolveFinding {
    std::string key;     // "op.3.radius"
    std::string message; // seam wording
};

// The explicit resolve pass: turn a stream-with-bindings into a stream of
// literals. PURE and ALL-OR-NOTHING. The input is never mutated. On success
// every binding's measured number is written through the B02 registry
// (setOpFieldValue) and `bindings` is cleared. On ANY failure, ok is false
// with a finding for EVERY failed binding (per-binding isolation, mirroring
// pipeline A's per-parameter isolation — the UI points at all stale bindings
// at once, not one re-run at a time) and an EMPTY ops vector, so no
// partially-resolved stream escapes into a consumer that trusts the doubles.
struct OpResolveResult {
    bool ok = false;
    std::vector<OpResolveFinding> findings;
    RecipeOpStream stream; // resolved (bindings empty) on ok; empty ops otherwise
};

OpResolveResult resolveRecipeOps(const RecipeOpStream &stream,
                                 const edi::drafting::DraftingDocument &drafting,
                                 const edi::drafting::DraftingGridProjection &grid);

} // namespace edi::recipe
