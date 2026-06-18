#include "io/CryptGenerator.h"

// No other dependencies — the builder is a pure MapSpec constructor.

namespace edi::io {

namespace {

// ---- layout data tables (standing rule 2026-06-18: every dimension is DATA) ----
//
// ALL crypt layout dimensions live in these three tables and nowhere else.
// buildCryptMapSpec() iterates them; it computes no dimension inline.
//
// Why constexpr tables, not a constructor / config file?
//   The builder is hardcoded-by-mandate (G1 proves the seam, not an algorithm).
//   Constexpr tables are zero-cost (no heap, no I/O), they surface the geometry
//   in one place for reviewers, and the compiler enforces completeness — a missing
//   field is a compile error, not a silent wrong result.  When the layout changes
//   the diff is exactly one table row, never a search through body logic.
//
// Why std::string_view rather than const char*?
//   string_view carries the length alongside the pointer, so comparisons are O(N)
//   but never need NUL-scan; and {std::string(sv)} is the idiomatic conversion to
//   the std::string the MapSpec structs own.

struct CryptRoomDim {
    std::string_view           name;
    edi::drafting::Point2D     origin;   // NW corner, authored feet
    double                     width;    // east extent, authored feet
    double                     height;   // south extent, authored feet
    std::string_view           material; // neutral wall tag
};

struct CryptPlugDim {
    std::string_view       room;     // which kCryptRooms entry owns this plug
    std::string_view       plugName;
    edi::drafting::RoomEdge edge;
    std::string_view       type;     // neutral door tag
    // NO `at` literal — derived as the edge MIDPOINT from the room's own dims.
    // E/W edge length = room.height → midpoint at = height/2 from the start corner.
    // N/S edge length = room.width  → midpoint at = width/2  from the start corner.
    // This keeps the plug centred automatically if a dimension changes.
};

struct CryptConnDim {
    std::string_view fromRoom, fromPlug;
    std::string_view toRoom,   toPlug;
    std::string_view type;
};

// --- the single place the BASE (S=1) crypt geometry lives ---------------------
// (user directive 2026-06-18; scale knob S is a separate reviewer-gated slice,
// brief 046+, pending gate 045 — do NOT add the scale parameter here yet)

constexpr CryptRoomDim kCryptRooms[] = {
    {"entrance", { 0.0, 5.0}, 15.0, 15.0, "stone"},
    {"crypt",    {35.0, 0.0}, 25.0, 25.0, "stone"},
};

constexpr CryptPlugDim kCryptPlugs[] = {
    {"entrance", "to_crypt",    edi::drafting::RoomEdge::East, "door"},
    {"crypt",    "to_entrance", edi::drafting::RoomEdge::West, "door"},
};

constexpr CryptConnDim kCryptConns[] = {
    {"entrance", "to_crypt", "crypt", "to_entrance", "corridor"},
};

// Derive the plug offset `at` for a plug placed at its edge MIDPOINT.
// `at` is measured from the edge's START corner (the clockwise-first endpoint):
//   E edge NE→SE  length = height → at = height/2   (East of a room = NE at top)
//   W edge SW→NW  length = height → at = height/2   (West = SW at bottom)
//   N edge NW→NE  length = width  → at = width/2
//   S edge SE→SW  length = width  → at = width/2
// Result for the base (S=1) crypt:
//   entrance East  h=15 → at=7.5  → world anchor x=15, y=5+7.5=12.5   ✓
//   crypt    West  h=25 → at=12.5 → world anchor x=35, y=25−12.5=12.5 ✓  (colinear)
constexpr double edgeMidpointAt(const CryptRoomDim &room,
                                edi::drafting::RoomEdge edge) noexcept
{
    using edi::drafting::RoomEdge;
    switch (edge) {
    case RoomEdge::East:
    case RoomEdge::West:
        return room.height / 2.0;
    case RoomEdge::North:
    case RoomEdge::South:
        return room.width / 2.0;
    }
    return room.height / 2.0; // unreachable — switch is exhaustive
}

} // namespace

// M0 crypt MapSpec in AUTHORED FEET, on the 5 ft grid (socket contract §1/§3):
// 2 rooms + 1 plug at each facing-edge MIDPOINT + 1 connection.  All LAYOUT
// DIMENSIONS come from the file-scope data tables above (standing rule: dimensions
// are data); this function only translates the tables into a MapSpec.
// Props (sarcophagus, brazier, stair) are G2.
edi::drafting::MapSpec buildCryptMapSpec()
{
    using namespace edi::drafting;

    MapSpec spec;

    // 1. Rooms — iterate the room table.
    for (const CryptRoomDim &rd : kCryptRooms) {
        NamedRoomSpec room;
        room.name              = std::string(rd.name);
        room.spec.origin       = rd.origin;
        room.spec.width        = rd.width;
        room.spec.height       = rd.height;
        room.spec.wallMaterial = std::string(rd.material);
        spec.rooms.push_back(std::move(room));
    }

    // 2. Plugs — iterate the plug table; derive `at` from the room's dimensions.
    //    Why iterate kCryptPlugs separately after rooms are built?  The plug table
    //    references rooms by name, and we need the room's width/height to compute
    //    `at`.  The two tables are kept separate (room shape vs. connection graph)
    //    so each concern lives in one place — adding a room doesn't force a plug
    //    edit, and vice versa.
    for (const CryptPlugDim &pd : kCryptPlugs) {
        // Find the room dims used to derive the midpoint `at`.
        const CryptRoomDim *roomDim = nullptr;
        for (const CryptRoomDim &rd : kCryptRooms) {
            if (rd.name == pd.room) { roomDim = &rd; break; }
        }
        if (!roomDim) { continue; } // defensive — tables are always consistent

        // Push the plug onto the matching NamedRoomSpec that was already built.
        for (NamedRoomSpec &namedRoom : spec.rooms) {
            if (namedRoom.name != pd.room) { continue; }
            RoomPlugSpec plug;
            plug.edge  = pd.edge;
            plug.at    = edgeMidpointAt(*roomDim, pd.edge); // DERIVED — never a literal here
            plug.name  = std::string(pd.plugName);
            plug.type  = std::string(pd.type);
            plug.flags = {"crypt"}; // neutral theme tag — not a dimension
            namedRoom.spec.plugs.push_back(std::move(plug));
            break;
        }
    }

    // 3. Connections — iterate the connection table.
    for (const CryptConnDim &cd : kCryptConns) {
        MapConnectionSpec conn;
        conn.from = {std::string(cd.fromRoom), std::string(cd.fromPlug)};
        conn.to   = {std::string(cd.toRoom),   std::string(cd.toPlug)};
        conn.type = std::string(cd.type);
        spec.connections.push_back(std::move(conn));
    }

    // spec.blocks is empty: props (sarcophagus, brazier, stair) are G2.
    return spec;
}

} // namespace edi::io
