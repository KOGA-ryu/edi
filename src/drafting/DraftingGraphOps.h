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
// Map rooms are keyed by NAME (unique within a map — what plugs reference).
std::optional<std::size_t> mapRoomIndexByName(const DraftingDocument &document, const std::string &name);
// Interactive authoring (B2-2 connection tool): find the plug whose anchorObjectId
// matches the given document object id, or nullopt when no plug anchors to it.
// Used to resolve a canvas hit-test objectId back to the plug that rides on it:
// the user clicks a plug marker, hitTestDocument returns the marker's objectId,
// and this function maps it to the plug so the connection tool can refer to the
// plug record by id rather than carrying raw coordinates.
std::optional<DraftingPlugId> plugAtAnchorObject(const DraftingDocument &document,
                                                  const DraftingObjectId &objectId);

// Mutations.
DraftingStoreResult addPlug(DraftingDocument &document, DraftingPlug plug);
DraftingStoreResult removePlug(DraftingDocument &document, const DraftingPlugId &id);
// B2-3: update ONLY the neutral type field of an existing plug. The anchor,
// anchorObjectId, id, name, and flags are never touched (field isolation). The
// caller (setPlugType) re-mints the door leaf to match the new type; the graph
// op owns only the record mutation. Returns rejected when the id is unknown.
DraftingStoreResult updatePlug(DraftingDocument &document, const DraftingPlugId &id,
                                const std::string &type);
DraftingStoreResult declareConnection(DraftingDocument &document, DraftingDeclaredConnection connection);
DraftingStoreResult undeclareConnection(DraftingDocument &document, const DraftingConnectionId &id);
// Record a named map room (Seam C). Validates a non-empty, unique name; the
// footprint is authored data taken as-is. No cascade on object delete — a room is
// authored neutral metadata independent of the walls (like a block).
DraftingStoreResult addMapRoom(DraftingDocument &document, DraftingMapRoom room);

// Referential-integrity sweep. When an object is removed from the document, any
// plug anchored to it is now dangling — drop those plugs and (cascading) every
// connection that named them. This is the graph analogue of normalizeSelection:
// a prune the document owes AFTER a delete, called from removeObject. It does NOT
// bump revision (the delete that calls it owns the single bump). Returns true if
// anything was pruned. A document with no graph is a no-op (returns false).
bool pruneGraphForRemovedObject(DraftingDocument &document, const DraftingObjectId &removedObjectId);

// MOVE counterpart of pruneGraphForRemovedObject: when `movedObjectId`'s geometry
// has changed (translate, handle-drag, numeric edit), recompute the cached
// `DraftingPlug::anchor` of every plug riding on it, so `deriveEdge` and Seam C
// export read the live position instead of the stale creation-time snapshot.
// Retires the anchor-drift TODO that lived in DraftingDocument.h / DraftingGraphOps
// (ratified as Phase-1 decision 7 / brief 052). Pure; no Qt; does NOT bump
// document.revision (the geometry command that triggered the sync owns the bump).
// Returns true if any plug anchor was updated; false when the object is unknown
// or nothing is anchored to it (the common case for ordinary non-anchor objects).
bool syncGraphForMovedObject(DraftingDocument &document, const DraftingObjectId &movedObjectId);

} // namespace edi::drafting
