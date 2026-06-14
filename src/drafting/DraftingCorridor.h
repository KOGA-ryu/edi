#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h" // RoomEdge

#include <functional>
#include <vector>

namespace edi::drafting {

// Two door points to be joined by a corridor, each on a room wall whose edge gives
// the outward normal (so the corridor can leave each door PERPENDICULAR to its
// wall — the door-stub the research said other generators lack, free here because
// a plug already knows its edge). `width` is the walkable gap between the two side
// walls; `wallThickness` is the thin side-wall band itself.
struct CorridorSpec {
    Point2D doorA;
    RoomEdge edgeA = RoomEdge::North;
    Point2D doorB;
    RoomEdge edgeB = RoomEdge::North;
    double width = 0.06;
    double wallThickness = 0.02;
};

// Plan an orthogonal corridor (v1: straight / L / Z centerline, NO obstacle
// avoidance) between the two doors, returned as its two side-wall bands
// (WallGeometry objects, provenance "corridor"). The centerline leaves and enters
// each door along its wall normal; the side walls are the centerline offset by
// half the width, with mitered corners that SHARE endpoints so the existing
// wall-join pass cleans them. Emits nothing if the doors coincide. mintId supplies
// fresh object ids (the caller owns minting, keeping this Qt-free).
std::vector<DraftingObject> planCorridor(const CorridorSpec &spec,
                                         const std::function<DraftingObjectId()> &mintId);

} // namespace edi::drafting
