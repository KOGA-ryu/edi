#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"

#include <string>

namespace edi::io {

// Seam B export (Phase D): project an authored MapSpec — the typed product of a
// .map.toml — to a TOON map document the game engine reads. The inverse of
// parseMapSpecToml; pure (builds a string, no file I/O).
//
// Three flat, uniform TOON arrays in the compact tabular form
// (`name[count]{fields}:` + indented CSV rows):
//   rooms[N]{name,origin,size,material}     footprint in the AUTHORED units
//   plugs[M]{room,name,edge,type,connected} doors/portals; `connected` is derived
//   connections[K]{from,to,type}            the adjacency graph (from/to = room.plug)
// Rooms are keyed by NAME (what plugs reference — "room.plug"), so the .map.toml's
// positional index is intentionally dropped. A plug with no authored type defaults
// to "door"; a plug referenced by no connection reads connected=false (e.g. a
// secret dead-end). `units` only LABELS the numbers — parse with canvasPerUnit=1.0
// so the footprints stay in the authored unit, not canvas units.
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
std::string exportMapToToon(const edi::drafting::DraftingDocument &document,
                            const std::string &title = {},
                            const std::string &units = "feet");

} // namespace edi::io
