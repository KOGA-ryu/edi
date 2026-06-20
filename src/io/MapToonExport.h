#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"

#include <string>

namespace edi::io {

// Seam B export (Phase D): project an authored MapSpec — the typed product of a
// .map.toml — to a TOON map document the game engine reads. The inverse of
// parseMapSpecToml; pure (builds a string, no file I/O).
//
// Flat, uniform TOON arrays in the compact tabular form
// (`name[count]{fields}:` + indented CSV rows):
//   rooms[N]{name,origin,size,material}     footprint in the AUTHORED units
//   plugs[M]{room,name,edge,type,connected} doors/portals; `connected` is derived
//   connections[K]{from,to,type[,locked,key_id]}  the adjacency graph; the lock
//                                           columns are CONDITIONAL — appended only
//                                           when some connection carries a lock tag
//   markers[P]{room,id,role,x,y,meta}       the neutral entity layer (spawn/pickup/
//                                           npc/goal/chest/…); CONDITIONAL — emitted
//                                           only when some room has a feature. `role`
//                                           is the feature type; `meta` is the
//                                           metadata as a `·`-joined key=value run
//   patrols[Q]{id,closed,points}            neutral patrol paths; CONDITIONAL —
//                                           emitted only when patrols exist. `points`
//                                           is a `·`-joined run of `x,y` pairs
// Rooms are keyed by NAME (what plugs reference — "room.plug"), so the .map.toml's
// positional index is intentionally dropped. A plug with no authored type defaults
// to "door"; a plug referenced by no connection reads connected=false (e.g. a
// secret dead-end). `units` only LABELS the numbers — parse with canvasPerUnit=1.0
// so the footprints stay in the authored unit, not canvas units. The conditional
// sections/columns keep every legacy (entity-less, lock-less) map BYTE-IDENTICAL.
std::string exportMapToToon(const edi::drafting::MapSpec &spec,
                            const std::string &title = {},
                            const std::string &units = "feet");

// Seam C export: project the live DOCUMENT (not the authoring .map.toml) — so it
// carries the placed BLOCK INSTANCES the .map.toml never had. Same three arrays as
// above plus a fourth:
//   blocks[B]{room,asset,origin,scale,rotation}  placed asset instances
// The document stores coordinates in CANVAS units; this divides by the document's
// canvasPerAuthoredUnit to recover the authored units (`units`). Plug edges are
// DERIVED from each plug's anchor vs its room footprint (the document plug carries
// no edge), and connections resolve plug ids back to "room.plug" names. Block
// instances are re-formed by grouping placed objects on their BlockPlacementMetadata
// instanceId; `origin` is the placement centre, `scale`/`rotation` are 1/0 today
// (placement is translate-only).
// Engine-reader contract: a block placed outside every room footprint emits an
// EMPTY `room` cell, and a block with no linked asset emits an EMPTY `asset` cell —
// the reader should tolerate empty cells in the blocks[] rows.
//
// `sceneScale` — emit ONE advisory `scale: <S>` header line only when sceneScale !=
// 1.0 (OMIT at S=1 so all existing exports stay byte-identical; engine treats missing
// => 1.0).  THE FENCE (socket contract §0/§5): the exported FEET are already scaled
// and authoritative; the `scale:` line is advisory for the realizer's greybox
// constants (CORRIDOR_W, WALL_H, ...) only.  Do NOT re-scale anything based on it.
std::string exportMapToToon(const edi::drafting::DraftingDocument &document,
                            const std::string &title = {},
                            const std::string &units = "feet",
                            double sceneScale = 1.0);

} // namespace edi::io
