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
};

struct DraftingRoomPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects; // the perimeter wall segments

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
