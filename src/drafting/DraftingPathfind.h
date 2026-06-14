#pragma once

#include <vector>

namespace edi::drafting {

struct GridCell {
    int col = 0;
    int row = 0;
};

// A weighted occupancy grid for corridor routing. cellCost[row*cols + col] is the
// cost to ENTER that cell — free space is cheap, a room is expensive (so a path
// routes AROUND rooms but may cut through one if it must), and a cost at or above
// kImpassableCost is a hard wall the path may never cross.
inline constexpr double kImpassableCost = 1e9;

struct DraftingPathGrid {
    int cols = 0;
    int rows = 0;
    std::vector<double> cellCost;

    bool inBounds(int col, int row) const;
    double at(int col, int row) const; // kImpassableCost when out of bounds
};

// 4-connected A* from start to goal. g = sum of entered-cell costs plus turnPenalty
// on each direction change (so the path keeps few corners — clean orthogonal halls);
// h = Manhattan distance (admissible as long as every free cell costs >= 1). Returns
// the inclusive cell path start..goal, or empty if the goal is unreachable or either
// endpoint is out of bounds / impassable.
std::vector<GridCell> findGridPath(const DraftingPathGrid &grid, GridCell start, GridCell goal,
                                   double turnPenalty);

} // namespace edi::drafting
