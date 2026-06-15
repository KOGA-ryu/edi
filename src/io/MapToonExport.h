#pragma once

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

} // namespace edi::io
