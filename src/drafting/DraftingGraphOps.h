#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingStore.h" // DraftingStoreResult

#include <cstddef>
#include <optional>

namespace edi::drafting {

// S1 of the map graph: the mutation ops over the document's plug/connection
// vectors. They are FREE FUNCTIONS over the document returning DraftingStoreResult
// — the exact shape addObject/removeObject use (same accepted()/rejected(), same
// ++revision-on-success) — so the command layer (S2) wraps them identically and
// they inherit DocumentSnapshot undo with no extra plumbing.
//
// NEUTRAL-ONLY: these record and validate the graph as plain references. They
// check that ids are well-formed and that the objects/plugs an edge names EXIST;
// they never interpret a plug's type or a connection's meaning.

// Read helpers — the plug/connection analogues of objectIndexById.
std::optional<std::size_t> plugIndexById(const DraftingDocument &document, const DraftingPlugId &id);
std::optional<std::size_t> connectionIndexById(const DraftingDocument &document, const DraftingConnectionId &id);

// Mutations.
DraftingStoreResult addPlug(DraftingDocument &document, DraftingPlug plug);
DraftingStoreResult removePlug(DraftingDocument &document, const DraftingPlugId &id);
DraftingStoreResult declareConnection(DraftingDocument &document, DraftingDeclaredConnection connection);
DraftingStoreResult undeclareConnection(DraftingDocument &document, const DraftingConnectionId &id);

// Referential-integrity sweep. When an object is removed from the document, any
// plug anchored to it is now dangling — drop those plugs and (cascading) every
// connection that named them. This is the graph analogue of normalizeSelection:
// a prune the document owes AFTER a delete, called from removeObject. It does NOT
// bump revision (the delete that calls it owns the single bump). Returns true if
// anything was pruned. A document with no graph is a no-op (returns false).
bool pruneGraphForRemovedObject(DraftingDocument &document, const DraftingObjectId &removedObjectId);

} // namespace edi::drafting
