#include "drafting/DraftingPathfind.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>

namespace edi::drafting {

bool DraftingPathGrid::inBounds(int col, int row) const
{
    return col >= 0 && row >= 0 && col < cols && row < rows;
}

double DraftingPathGrid::at(int col, int row) const
{
    if (!inBounds(col, row)) {
        return kImpassableCost;
    }
    return cellCost[static_cast<std::size_t>(row) * cols + col];
}

namespace {

// The four 4-connected steps; index doubles as the "direction" used for the turn
// penalty (a change of index between consecutive steps is a corner).
constexpr int kDx[4] = {1, -1, 0, 0};
constexpr int kDy[4] = {0, 0, 1, -1};

// Search state = (cell, direction entered by). Direction 4 is the start sentinel
// (no incoming direction, so the first move is never a turn). 5 states per cell.
int stateIndex(int cellIndex, int dir)
{
    return cellIndex * 5 + dir;
}

struct Node {
    double f;
    double g;
    int cell;
    int dir;
};

struct NodeGreater {
    bool operator()(const Node &a, const Node &b) const { return a.f > b.f; }
};

double manhattan(GridCell a, GridCell b)
{
    return static_cast<double>(std::abs(a.col - b.col) + std::abs(a.row - b.row));
}

} // namespace

std::vector<GridCell> findGridPath(const DraftingPathGrid &grid, GridCell start, GridCell goal,
                                   double turnPenalty)
{
    if (!grid.inBounds(start.col, start.row) || !grid.inBounds(goal.col, goal.row)) {
        return {};
    }
    if (grid.at(start.col, start.row) >= kImpassableCost || grid.at(goal.col, goal.row) >= kImpassableCost) {
        return {};
    }

    const int n = grid.cols * grid.rows;
    const auto cellOf = [&](int col, int row) { return row * grid.cols + col; };
    const int startCell = cellOf(start.col, start.row);
    const int goalCell = cellOf(goal.col, goal.row);

    const double inf = std::numeric_limits<double>::infinity();
    std::vector<double> gScore(static_cast<std::size_t>(n) * 5, inf);
    std::vector<int> parent(static_cast<std::size_t>(n) * 5, -1);

    std::priority_queue<Node, std::vector<Node>, NodeGreater> open;
    gScore[stateIndex(startCell, 4)] = 0.0;
    open.push({manhattan(start, goal), 0.0, startCell, 4});

    while (!open.empty()) {
        const Node cur = open.top();
        open.pop();
        if (cur.g > gScore[stateIndex(cur.cell, cur.dir)]) {
            continue; // stale entry
        }
        if (cur.cell == goalCell) {
            // Reconstruct: walk parents back to the start sentinel.
            std::vector<GridCell> path;
            int state = stateIndex(cur.cell, cur.dir);
            while (state >= 0) {
                const int cell = state / 5;
                path.push_back({cell % grid.cols, cell / grid.cols});
                state = parent[state];
            }
            return {path.rbegin(), path.rend()};
        }

        const int col = cur.cell % grid.cols;
        const int row = cur.cell / grid.cols;
        for (int d = 0; d < 4; ++d) {
            const int ncol = col + kDx[d];
            const int nrow = row + kDy[d];
            const double enter = grid.at(ncol, nrow);
            if (enter >= kImpassableCost) {
                continue;
            }
            const double turn = (cur.dir != 4 && d != cur.dir) ? turnPenalty : 0.0;
            const double g = cur.g + enter + turn;
            const int nstate = stateIndex(cellOf(ncol, nrow), d);
            if (g < gScore[nstate]) {
                gScore[nstate] = g;
                parent[nstate] = stateIndex(cur.cell, cur.dir);
                open.push({g + manhattan({ncol, nrow}, goal), g, cellOf(ncol, nrow), d});
            }
        }
    }
    return {};
}

} // namespace edi::drafting
