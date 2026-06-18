#include "io/CryptGenerator.h"

// No other dependencies — the builder is a pure MapSpec constructor; it has no
// knowledge of canvas scale, Qt, or the document model.  Those live downstream
// (createMapFromSpec / exportMapToToon).

namespace edi::io {

// All coordinates are in AUTHORED FEET on the 5 ft grid (1 tile = 5 ft).
// The layout is hardcoded by mandate: the goal is to PROVE the seam, not to
// run a generation algorithm.  A real algorithm is a future M1+ milestone.
//
// Placement rationale (socket contract §3):
//   entrance 15×15 @ (0,0)  — 3×3 tiles, origin = NW corner
//   crypt    20×20 @ (35,25) — 4×4 tiles, origin = NW corner
//   gap between them: 35 - 15 = 20 ft (4 tiles) — enough for the corridor.
//
// Plug-position rule (socket contract §1): the realizer derives every
// doorway centre as the MIDPOINT of the named room edge.  The generator
// must author each plug AT its edge midpoint so the realizer's anchor
// computation exactly recovers the intended world position.
//
// Edges run clockwise — N(NW→NE) · E(NE→SE) · S(SE→SW) · W(SW→NW) —
// and `at` is measured from each edge's first-named (start) corner.
//
//   entrance East edge: NE=(15,0) → SE=(15,15)  length=15  midpoint y=7.5
//     at = 7.5 − 0 = 7.5 from NE    world anchor = (15, 7.5)
//   crypt    West edge: SW=(35,45) → NW=(35,25)  length=20  midpoint y=35
//     at = 45 − 35 = 10 from SW     world anchor = (35, 35)
//
// The two plug midpoints have world y=7.5 and y=35 — non-colinear — so
// the realizer routes an L-shaped corridor (exercises corridor_l + the full
// set of 10 piece types that the 5090 gate needed).
edi::drafting::MapSpec buildCryptMapSpec()
{
    using namespace edi::drafting;

    // ---- entrance chamber (3×3 tiles = 15×15 ft) ---------------------------
    NamedRoomSpec entrance;
    entrance.name = "entrance";
    entrance.spec.origin = {0.0, 0.0};
    entrance.spec.width  = 15.0;
    entrance.spec.height = 15.0;
    entrance.spec.wallMaterial = "stone";

    // East edge NE(15,0)→SE(15,15); midpoint at y=7.5; at = 7.5 − 0 = 7.5.
    RoomPlugSpec entrancePlug;
    entrancePlug.edge  = RoomEdge::East;
    entrancePlug.at    = 7.5;
    entrancePlug.name  = "to_crypt";
    entrancePlug.type  = "door";
    entrancePlug.flags = {"crypt"}; // neutral theme tag (carried verbatim to TOON)
    entrance.spec.plugs.push_back(entrancePlug);

    // ---- crypt chamber (4×4 tiles = 20×20 ft) ------------------------------
    NamedRoomSpec crypt;
    crypt.name = "crypt";
    crypt.spec.origin = {35.0, 25.0};
    crypt.spec.width  = 20.0;
    crypt.spec.height = 20.0;
    crypt.spec.wallMaterial = "stone";

    // West edge SW(35,45)→NW(35,25); midpoint at y=35; at = 45 − 35 = 10.
    RoomPlugSpec cryptPlug;
    cryptPlug.edge  = RoomEdge::West;
    cryptPlug.at    = 10.0;
    cryptPlug.name  = "to_entrance";
    cryptPlug.type  = "door";
    cryptPlug.flags = {"crypt"};
    crypt.spec.plugs.push_back(cryptPlug);

    // ---- connection (one L-corridor; plug midpoints non-colinear) ----------
    // The realizer handles the bend: corridor_l piece type + two CORRIDOR_END
    // sockets that snap to each doorway_frame (piece type table, contract §4).
    MapConnectionSpec connection;
    connection.from = {"entrance", "to_crypt"};
    connection.to   = {"crypt",    "to_entrance"};
    connection.type = "corridor";

    MapSpec spec;
    spec.rooms.push_back(entrance);
    spec.rooms.push_back(crypt);
    spec.connections.push_back(connection);
    // spec.blocks is empty: props (sarcophagus, brazier, stair) are G2.
    return spec;
}

} // namespace edi::io
