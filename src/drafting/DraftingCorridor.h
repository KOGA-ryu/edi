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

// A room rectangle a corridor should route around (canvas units).
struct CorridorObstacle {
    Point2D origin;
    double width = 0.0;
    double height = 0.0;
};

// The orthogonal centerline between the two doors (straight / L / Z), leaving and
// entering each door along its wall normal. Duplicate/colinear points are dropped.
std::vector<Point2D> corridorCenterline(const CorridorSpec &spec);

// Like corridorCenterline, but if the direct centerline would cross any obstacle
// rectangle it is re-routed with A* (rooms as expensive cells), so the corridor
// goes AROUND unrelated rooms. Falls back to the direct centerline if A* finds no
// improvement. (v2 — the obstacle-aware upgrade.)
std::vector<Point2D> routeCorridorCenterline(const CorridorSpec &spec,
                                             const std::vector<CorridorObstacle> &obstacles);

// Build a corridor's two side-wall bands (WallGeometry, provenance "corridor") from
// a centerline: each segment offset by half the width to both sides, with mitered
// corners that SHARE endpoints so the existing wall-join pass cleans them.
std::vector<DraftingObject> corridorWalls(const std::vector<Point2D> &centerline,
                                          double width, double wallThickness,
                                          const std::function<DraftingObjectId()> &mintId);

// Plan a corridor between two doors (the direct centerline + its walls). mintId
// supplies fresh object ids (the caller owns minting, keeping this Qt-free).
std::vector<DraftingObject> planCorridor(const CorridorSpec &spec,
                                         const std::function<DraftingObjectId()> &mintId);

} // namespace edi::drafting
