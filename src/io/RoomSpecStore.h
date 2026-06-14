#pragma once

#include "drafting/DraftingRoom.h"

#include <string>

namespace edi::io {

struct RoomSpecParseResult {
    bool ok = false;
    std::string message;
    edi::drafting::RoomSpec spec;
};

// Parse a room authored in edi's flat-TOML dialect (the SettingsStore / recipe
// convention: dotted `key = value` lines, lists as indexed keys) into a RoomSpec.
// Distances are written in the file's own units (e.g. feet); `canvasPerUnit`
// converts them to edi canvas units — the caller derives it from the document
// grid. This is the human/AI authoring front-end of Seam A; the parsed spec is
// neutral (geometry + tags), and createRoomFromSpec turns it into geometry.
//
// Schema (all distances in file units):
//   room.width, room.height                 (required, > 0)
//   room.origin_x, room.origin_y            (default 0)
//   room.wall_thickness                     (default 1)
//   room.wall_material                      (default "stone")
//   room.opening.<i>.edge   = N|E|S|W       (i = 0,1,2,...; stop at first gap)
//   room.opening.<i>.type   = door|secret|… (neutral tag; default "door")
//   room.opening.<i>.width                  (> 0)
//   room.opening.<i>.at     = center | <offset-from-edge-start>
//   room.plug.<i>.edge      = N|E|S|W       (i = 0,1,2,...; stop at first gap)
//   room.plug.<i>.name      = <label>       (required, unique; a connection refers to it)
//   room.plug.<i>.type      = door|portal|… (neutral tag; default "door")
//   room.plug.<i>.at        = center | <offset-from-edge-start>
//   map.connection.<i>.from = <plug name>   (i = 0,1,2,...; stop at first gap)
//   map.connection.<i>.to   = <plug name>   (both ends must name existing plugs)
//   map.connection.<i>.type = corridor|…    (neutral tag; default "")
RoomSpecParseResult parseRoomSpecToml(const std::string &text, double canvasPerUnit = 1.0);

} // namespace edi::io
