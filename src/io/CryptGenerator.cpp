#include "io/CryptGenerator.h"

// No other dependencies — the builder is a pure MapSpec constructor.

namespace edi::io {

namespace {

// ---- layout data tables (standing rule 2026-06-18: every dimension is DATA) ----
//
// ALL crypt layout dimensions live in these four tables and nowhere else.
// buildCryptMapSpec() iterates them; it computes no dimension inline.
//
// Why constexpr tables, not a constructor / config file?
//   The builder is hardcoded-by-mandate (G1/G2 prove the seam, not an algorithm).
//   Constexpr tables are zero-cost (no heap, no I/O), surface the geometry in one
//   place for reviewers, and the compiler enforces structural completeness — a missing
//   field is a compile error, not a silent wrong result.  When the layout changes the
//   diff is exactly one table row, never a search through body logic.
//
// Why std::string_view rather than const char*?
//   string_view carries the length alongside the pointer so comparisons are O(N)
//   but never need NUL-scan; {std::string(sv)} is the idiomatic conversion to the
//   std::string the MapSpec structs own.

struct CryptRoomDim {
    std::string_view           name;
    edi::drafting::Point2D     origin;   // NW corner, authored feet at S=1
    double                     width;    // east extent, authored feet at S=1
    double                     height;   // south extent, authored feet at S=1
    std::string_view           material; // neutral wall tag
};

struct CryptPlugDim {
    std::string_view       room;     // which kCryptRooms entry owns this plug
    std::string_view       plugName;
    edi::drafting::RoomEdge edge;
    std::string_view       type;     // neutral door tag
    // NO `at` literal — derived as the edge MIDPOINT from the (scaled) room dims.
    // E/W edge length = room.height → midpoint at = height/2 from the start corner.
    // N/S edge length = room.width  → midpoint at = width/2  from the start corner.
    // Scaling room.height/width automatically scales `at` by the same factor.
};

struct CryptConnDim {
    std::string_view fromRoom, fromPlug;
    std::string_view toRoom,   toPlug;
    std::string_view type;
};

struct CryptBlockDim {
    std::string_view       asset;    // "<theme>.<piece>" asset ref for the realizer
    edi::drafting::Point2D position; // placement CENTRE, authored feet at S=1
    std::string_view       name;     // neutral label (carried as name:<name> tag)
};

// --- the single place the BASE (S=1) crypt geometry lives -------------------
// `scale` multiplies every dimension uniformly in buildCryptMapSpec(scale).

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

// G2: prop block instances (structure-only G1 had no blocks; G2 adds these).
// Identity rotation + scale (the per-block placement transform, NOT the dungeon S).
// Positions are BASE feet at S=1; buildCryptMapSpec() multiplies by S.
constexpr CryptBlockDim kCryptBlocks[] = {
    {"crypt.sarcophagus", {47.5, 12.5}, "sarcophagus"},
    {"crypt.brazier",     {41.0,  6.0}, "brazier"},
    {"crypt.stair",       {38.0,  4.0}, "stair"},
};

// Derive the plug offset `at` for a plug placed at its edge MIDPOINT.
// `at` is measured from the edge's START corner (the clockwise-first endpoint):
//   E edge NE→SE  length = height → at = height/2   (NE is the start)
//   W edge SW→NW  length = height → at = height/2   (SW is the start)
//   N edge NW→NE  length = width  → at = width/2
//   S edge SE→SW  length = width  → at = width/2
// Caller passes the SCALED height/width so `at` scales uniformly for free.
// Base S=1 examples:
//   entrance East  h=15 → at=7.5  → world anchor x=15,  y=5+7.5=12.5   ✓
//   crypt    West  h=25 → at=12.5 → world anchor x=35, y=25−12.5=12.5  ✓ (colinear)
constexpr double edgeMidpointAt(double height, double width,
                                edi::drafting::RoomEdge edge) noexcept
{
    using edi::drafting::RoomEdge;
    switch (edge) {
    case RoomEdge::East:
    case RoomEdge::West:
        return height / 2.0;
    case RoomEdge::North:
    case RoomEdge::South:
        return width / 2.0;
    }
    return height / 2.0; // unreachable — switch is exhaustive
}

} // namespace

// M0 crypt MapSpec in AUTHORED FEET, on the 5 ft grid (socket contract §1/§3):
// 2 rooms + 1 plug at each facing-edge MIDPOINT + 1 connection + 3 block props.
// All LAYOUT DIMENSIONS come from the file-scope tables above (standing rule:
// dimensions are data); this function only scales and assembles the MapSpec.
//
// WHY `scale` is a parameter, not a table literal:
//   The base layout is named data (kCryptRooms/kCryptBlocks); `scale` is the only
//   user-facing variable.  "Where a dimension varies (scale, theme) it is a spec
//   field or parameter, not a frozen constant" (SCALE-POLICY.md standing rule).
//   Baking S into the table would force re-briefing on every size change — the
//   knob's whole purpose is to avoid that.
//
// THE FENCE (socket contract §0/§5): the exported FEET are already scaled and
// authoritative — the `scale:` header meta is advisory for the realizer's greybox
// constants (CORRIDOR_W, WALL_H, …) and light range; the realizer must NOT
// re-scale the room footprints (they are literal scaled feet, not base feet).
edi::drafting::MapSpec buildCryptMapSpec(double scale)
{
    using namespace edi::drafting;

    MapSpec spec;

    // 1. Rooms — iterate the room table; multiply every dimension by scale.
    //    Origin and size scale uniformly so the whole dungeon expands from (0,0)
    //    without shear — missing one field would create a misaligned layout.
    for (const CryptRoomDim &rd : kCryptRooms) {
        NamedRoomSpec room;
        room.name              = std::string(rd.name);
        room.spec.origin       = {rd.origin.x * scale, rd.origin.y * scale};
        room.spec.width        = rd.width  * scale;
        room.spec.height       = rd.height * scale;
        room.spec.wallMaterial = std::string(rd.material);
        spec.rooms.push_back(std::move(room));
    }

    // 2. Plugs — iterate the plug table; derive `at` from the SCALED room dims.
    //    Why iterate kCryptPlugs separately after rooms are built?  The plug table
    //    references rooms by name, and we need the scaled height/width to compute
    //    `at`.  The two tables stay separate (room shape vs. connection topology)
    //    so each concern lives in one place.
    for (const CryptPlugDim &pd : kCryptPlugs) {
        // Find the BASE room entry to get width/height (scale applied below).
        const CryptRoomDim *roomDim = nullptr;
        for (const CryptRoomDim &rd : kCryptRooms) {
            if (rd.name == pd.room) { roomDim = &rd; break; }
        }
        if (!roomDim) { continue; } // defensive — tables are always consistent

        // Push the plug onto the matching NamedRoomSpec that was already built.
        for (NamedRoomSpec &namedRoom : spec.rooms) {
            if (namedRoom.name != pd.room) { continue; }
            RoomPlugSpec plug;
            plug.edge = pd.edge;
            // Derive `at` from the SCALED dims: base dim × scale, then / 2.
            // Equivalent to reading namedRoom.spec.height/width — same value.
            plug.at   = edgeMidpointAt(roomDim->height * scale,
                                       roomDim->width  * scale,
                                       pd.edge); // DERIVED — never a literal here
            plug.name  = std::string(pd.plugName);
            plug.type  = std::string(pd.type);
            plug.flags = {"crypt"}; // neutral theme tag — not a dimension
            namedRoom.spec.plugs.push_back(std::move(plug));
            break;
        }
    }

    // 3. Connections — iterate the connection table (connections are graph
    //    topology, not dimensions, so scale does not apply here).
    for (const CryptConnDim &cd : kCryptConns) {
        MapConnectionSpec conn;
        conn.from = {std::string(cd.fromRoom), std::string(cd.fromPlug)};
        conn.to   = {std::string(cd.toRoom),   std::string(cd.toPlug)};
        conn.type = std::string(cd.type);
        spec.connections.push_back(std::move(conn));
    }

    // 4. Blocks — iterate the block table; multiply positions by scale.
    //    The per-block placement transform (MapBlockSpec.scale / .rotationDeg)
    //    stays at identity (1 / 0) — that is NOT the dungeon S; it is the
    //    individual instance transform used by the block FLATTEN machinery.
    for (const CryptBlockDim &bd : kCryptBlocks) {
        MapBlockSpec block;
        block.assetRef  = std::string(bd.asset);
        block.position  = {bd.position.x * scale, bd.position.y * scale};
        block.rotationDeg = 0.0; // identity — individual placement transform
        block.scale       = 1.0; // identity — NOT the dungeon S
        block.name      = std::string(bd.name);
        spec.blocks.push_back(std::move(block));
    }

    return spec;
}

} // namespace edi::io
