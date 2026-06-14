// A* corridor pathfinder: 4-connected shortest path on a weighted grid, with a turn
// penalty (clean low-corner halls) and room cells as expensive quasi-obstacles
// (paths route around rooms but may cut through if forced).
#include "drafting/DraftingPathfind.h"

#include <cassert>
#include <cstddef>
#include <vector>

using namespace edi::drafting;

namespace {

// All-free grid: every cell costs 1 to enter.
DraftingPathGrid freeGrid(int cols, int rows)
{
    DraftingPathGrid g;
    g.cols = cols;
    g.rows = rows;
    g.cellCost.assign(static_cast<std::size_t>(cols) * rows, 1.0);
    return g;
}

void block(DraftingPathGrid &g, int col, int row)
{
    g.cellCost[static_cast<std::size_t>(row) * g.cols + col] = kImpassableCost;
}

int turns(const std::vector<GridCell> &path)
{
    int count = 0;
    for (std::size_t i = 2; i < path.size(); ++i) {
        const int dx1 = path[i - 1].col - path[i - 2].col;
        const int dy1 = path[i - 1].row - path[i - 2].row;
        const int dx2 = path[i].col - path[i - 1].col;
        const int dy2 = path[i].row - path[i - 1].row;
        if (dx1 != dx2 || dy1 != dy2) {
            ++count;
        }
    }
    return count;
}

} // namespace

int main()
{
    // Straight clear run: 5 cells, no turns.
    {
        const DraftingPathGrid g = freeGrid(5, 5);
        const std::vector<GridCell> path = findGridPath(g, {0, 0}, {4, 0}, 1.0);
        assert(path.size() == 5);
        assert(path.front().col == 0 && path.front().row == 0);
        assert(path.back().col == 4 && path.back().row == 0);
        for (const GridCell &c : path) {
            assert(c.row == 0);
        }
        assert(turns(path) == 0);
    }

    // A wall across the direct line forces a detour around it.
    {
        DraftingPathGrid g = freeGrid(5, 5);
        block(g, 2, 0); // a vertical wall on col 2, rows 0..2
        block(g, 2, 1);
        block(g, 2, 2);
        const std::vector<GridCell> path = findGridPath(g, {0, 0}, {4, 0}, 1.0);
        assert(!path.empty());
        assert(path.front().col == 0 && path.front().row == 0);
        assert(path.back().col == 4 && path.back().row == 0);
        for (const GridCell &c : path) {
            const bool onWall = c.col == 2 && c.row <= 2;
            assert(!onWall); // never steps on the wall
        }
        assert(path.size() > 5); // longer than the blocked straight line
    }

    // Turn penalty yields a clean L (one corner), not a staircase.
    {
        const DraftingPathGrid g = freeGrid(5, 5);
        const std::vector<GridCell> path = findGridPath(g, {0, 0}, {4, 4}, 5.0);
        assert(!path.empty());
        assert(path.back().col == 4 && path.back().row == 4);
        assert(turns(path) <= 1);
    }

    // Goal walled off on every side is unreachable.
    {
        DraftingPathGrid g = freeGrid(5, 5);
        block(g, 3, 4);
        block(g, 4, 3);
        // goal (4,4) is enclosed by walls + the grid corner.
        assert(findGridPath(g, {0, 0}, {4, 4}, 1.0).empty());
    }

    // A room cell is expensive but passable: with no alternative the path uses it.
    {
        DraftingPathGrid g = freeGrid(3, 1);
        g.cellCost[1] = 10.0; // middle cell is a "room"
        const std::vector<GridCell> path = findGridPath(g, {0, 0}, {2, 0}, 1.0);
        assert(path.size() == 3); // forced straight through the expensive cell
    }

    return 0;
}
