#pragma once

#include "drafting/DraftingDocument.h"

#include <functional>
#include <string>
#include <vector>

namespace edi::drafting {

// The four edges of a rectangular room, named by compass corner. The edges run
// CLOCKWISE — N(NW->NE) · E(NE->SE) · S(SE->SW) · W(SW->NW) — so an opening's
// `center` (below) is always measured from the edge's first-named corner.
enum class RoomEdge {
    North,
    East,
    South,
    West
};

// An opening hosted in one edge, given as an INTERVAL along that edge: `center`
// is the distance from the edge's start corner and `width` the gap size (canvas
// units). The wall owns the gap (cleaner than two free endpoints — see the
// Seam-A discussion). `type` is a NEUTRAL tag the drafting layer does not
// interpret — door / corridor / secret / window — leaving the rule (does a
// secret door block sight?) to the game engine downstream. v1 realises an
// opening as a literal GAP in the band; the door leaf / secret marker is M2.2.
struct RoomOpening {
    RoomEdge edge = RoomEdge::North;
    double center = 0.0;
    double width = 0.0;
    std::string type;
};

// A plug authored on an edge: a NEUTRAL named attachment point at a single offset
// along the edge (like an opening, but a point — no width). `at` is the distance
// from the edge's start corner. The drafting layer realises it as a Point marker;
// the map graph's plug record (DraftingPlug) then anchors to that marker. `type`
// is a neutral tag (door/portal/threshold/…) edi does not interpret.
struct RoomPlugSpec {
    RoomEdge edge = RoomEdge::North;
    double at = 0.0;
    std::string name;
    std::string type;
};

// A declared connection authored between two plugs, named by their plug `name`s
// (the controller resolves names to the plug ids it minted). NEUTRAL — it records
// that two plugs are joined, nothing about whether the join is passable. `type` is
// a free tag (corridor/portal/…). Authored at map level because a connection can
// span rooms; in a single-room file both ends are that room's plugs.
struct RoomConnectionSpec {
    std::string from;
    std::string to;
    std::string type;
};

// A rectangular room in CANVAS units (the grid projection maps these to physical
// feet/squares). origin is the NW corner. The room is pure spatial data plus a
// neutral material tag — no game semantics, by design.
struct RoomSpec {
    Point2D origin;
    double width = 0.0;
    double height = 0.0;
    double wallThickness = 0.1;
    std::string wallMaterial = "stone"; // neutral tag carried on every wall
    std::vector<RoomOpening> openings;
    std::vector<RoomPlugSpec> plugs;
    std::vector<RoomConnectionSpec> connections; // edges between plugs (by name)
};

// --- multi-room map authoring (a whole dungeon from one file) ----------------
// A reference to a plug in a multi-room map, by (room name, plug name). Plug names
// are unique only WITHIN a room, so a cross-room connection needs both halves.
struct MapPlugRef {
    std::string roomName;
    std::string plugName;
};

// A connection authored at MAP level: it can span rooms, so each endpoint is a
// (room, plug) pair, not a bare plug name. Neutral like RoomConnectionSpec. A hub
// of degree N (e.g. a 3-way junction) is an N-plug room with N of these edges —
// a single connection joins exactly two plugs, never fans to three.
struct MapConnectionSpec {
    MapPlugRef from;
    MapPlugRef to;
    std::string type;
};

// One room in a multi-room map, named so connections can find its plugs. The
// geometry is just a RoomSpec — the multi-room file reuses the single-room shape.
struct NamedRoomSpec {
    std::string name;
    RoomSpec spec;
};

// A whole map: many named rooms in one coordinate space + the cross-room
// connections between their plugs. The neutral authoring product of a .map.toml.
struct MapSpec {
    std::vector<NamedRoomSpec> rooms;
    std::vector<MapConnectionSpec> connections;
};

// A plug the room emitted, paired with the marker object it rides on. The marker
// is in DraftingRoomPlan::objects; this records WHICH plug it is (neutral name +
// type) and the marker's id as the anchor, so the caller mints a plug id and
// issues a CreatePlugCommand. planDraftingRoom mints the marker's OBJECT id; the
// caller mints the PLUG id (work-order decision #4: caller mints, op validates).
struct RoomPlugPlacement {
    DraftingObjectId anchorObjectId;
    std::string name;
    std::string type;
    Point2D anchor;
    RoomEdge edge = RoomEdge::North; // which wall the plug sits on (its outward normal)
};

struct DraftingRoomPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects; // the perimeter wall segments + plug markers
    std::vector<RoomPlugPlacement> plugs; // one per authored plug, anchored to a marker

    static DraftingRoomPlan accepted(std::vector<DraftingObject> objects);
    static DraftingRoomPlan rejected(DraftingResultCode code, std::string message);
};

// Build a room's perimeter as WallGeometry segments: each edge MINUS its
// openings, so a solid edge is one segment and an edge with a centred door is
// two. Adjacent segments share corner endpoints, so the existing wall-join pass
// miters them for free; an opening at a corner simply leaves that corner open.
// `mintId` supplies a fresh object id per emitted segment — the caller owns id
// minting so this stays Qt-free and the segment count (opening-dependent) need
// not be known in advance.
DraftingRoomPlan planDraftingRoom(const RoomSpec &spec,
                                  const std::function<DraftingObjectId()> &mintId);

} // namespace edi::drafting
