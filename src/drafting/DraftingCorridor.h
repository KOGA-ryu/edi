#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h" // RoomEdge

#include <functional>
#include <vector>

namespace edi::drafting {

// CorridorSpec fallback defaults (canvas units).
// DORMANT — every live caller (createMapFromSpec, tests) supplies its own width;
// these exist only so a default-constructed CorridorSpec is well-formed.
// Named per the no-hardcoded-dimensions rule: a dimension is DATA, never a bare
// literal in a struct definition.
//
// NOTE: these are distinct from kDefaultCorridorWidth in DraftingCanvasDims.h
// (that constant = 0.045 and governs the INTERACTIVE corridor tool / canvas
// fallback). The CorridorSpec struct predates the canvas-dims unification and
// carries its own historical defaults (0.06 / 0.02) — same behavior, separate
// named data. If the values are ever unified, update this constant, not callers.
// `kCorridorSpecDefaultWallThickness` is a SEPARATE dimension from the walkable
// width even though the values happen to differ; they scale independently.
inline constexpr double kCorridorSpecDefaultWidth         = 0.06;
inline constexpr double kCorridorSpecDefaultWallThickness = 0.02;

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
    double width         = kCorridorSpecDefaultWidth;
    double wallThickness = kCorridorSpecDefaultWallThickness;
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
