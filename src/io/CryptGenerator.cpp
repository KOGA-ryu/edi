#include "io/CryptGenerator.h"

// No other dependencies — the builder is a pure MapSpec constructor; it has no
// knowledge of canvas scale, Qt, or the document model.  Those live downstream
// (createMapFromSpec / exportMapToToon).

namespace edi::io {

// All coordinates are in AUTHORED FEET on the 5 ft grid (1 tile = 5 ft).
// The layout is hardcoded by mandate (user directive 2026-06-18, "doubled crypt"):
// the goal is to PROVE the seam, not to run a generation algorithm.
//
// Placement (socket contract §3, doubled geometry):
//   entrance 30×30 @ (0,10)  — 6×6 tiles, origin = NW corner
//   crypt    50×50 @ (70,0)  — 10×10 tiles, origin = NW corner
//   gap between them: 70 − 30 = 40 ft (8 tiles) — room for the corridor.
//
// Plug-position rule (socket contract §1): the realizer derives every
// doorway centre as the MIDPOINT of the named room edge.  The generator
// must author each plug AT its edge midpoint so the realizer's anchor
// computation exactly recovers the intended world position.
//
// Edges run clockwise — N(NW→NE) · E(NE→SE) · S(SE→SW) · W(SW→NW) —
// and `at` is measured from each edge's first-named (start) corner.
//
//   entrance East edge: NE=(30,10) → SE=(30,40)  length=30  midpoint y=25
//     at = 25 − 10 = 15 from NE    world anchor = (30, 25)
//   crypt    West edge: SW=(70,50) → NW=(70,0)   length=50  midpoint y=25
//     at = 50 − 25 = 25 from SW    world anchor = (70, 25)
//
// Both plug midpoints share world y=25 — COLINEAR — so the realizer routes
// a straight corridor (corridor_straight piece type, no bend needed).
edi::drafting::MapSpec buildCryptMapSpec()
{
    using namespace edi::drafting;

    // ---- entrance chamber (6×6 tiles = 30×30 ft) ---------------------------
    NamedRoomSpec entrance;
    entrance.name = "entrance";
    entrance.spec.origin = {0.0, 10.0};
    entrance.spec.width  = 30.0;
    entrance.spec.height = 30.0;
    entrance.spec.wallMaterial = "stone";

    // East edge NE(30,10)→SE(30,40); midpoint at y=25; at = 25 − 10 = 15.
    RoomPlugSpec entrancePlug;
    entrancePlug.edge  = RoomEdge::East;
    entrancePlug.at    = 15.0;
    entrancePlug.name  = "to_crypt";
    entrancePlug.type  = "door";
    entrancePlug.flags = {"crypt"}; // neutral theme tag (carried verbatim to TOON)
    entrance.spec.plugs.push_back(entrancePlug);

    // ---- crypt chamber (10×10 tiles = 50×50 ft) ----------------------------
    NamedRoomSpec crypt;
    crypt.name = "crypt";
    crypt.spec.origin = {70.0, 0.0};
    crypt.spec.width  = 50.0;
    crypt.spec.height = 50.0;
    crypt.spec.wallMaterial = "stone";

    // West edge SW(70,50)→NW(70,0); midpoint at y=25; at = 50 − 25 = 25.
    RoomPlugSpec cryptPlug;
    cryptPlug.edge  = RoomEdge::West;
    cryptPlug.at    = 25.0;
    cryptPlug.name  = "to_entrance";
    cryptPlug.type  = "door";
    cryptPlug.flags = {"crypt"};
    crypt.spec.plugs.push_back(cryptPlug);

    // ---- connection (one straight corridor; plug midpoints colinear y=25) --
    // Colinear midpoints → realizer routes corridor_straight (no bend);
    // the L-bend variant (corridor_l) is exercised when midpoints differ.
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
