#include "widgets/BeltState.h"

#include <cassert>

using namespace edi::shell;

namespace {

bool crossHasCell(const std::vector<BeltCrossCell> &cells, int row, int column)
{
    for (const BeltCrossCell &cell : cells) {
        if (cell.row == row && cell.column == column) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    // Default state: 6x6, active at the origin, valid, index 0.
    {
        const BeltState state;
        assert(beltStateValid(state));
        assert(beltActiveIndex(state) == 0);
    }

    // Validity rejects empty grids and out-of-range active cells.
    assert(!beltStateValid({0, 6, 0, 0}));
    assert(!beltStateValid({6, 0, 0, 0}));
    assert(!beltStateValid({6, 6, 6, 0}));
    assert(!beltStateValid({6, 6, 0, -1}));

    // Row-major index: the contract with the caller's flat item list.
    assert(beltActiveIndex({6, 6, 2, 3}) == 15);
    assert(beltActiveIndex({6, 6, 5, 5}) == 35);

    // Vertical scroll changes the row and wraps both directions.
    {
        BeltState state{6, 6, 0, 2};
        state = beltStepRow(state, 1);
        assert(state.activeRow == 1 && state.activeColumn == 2);
        state = beltStepRow(state, -2);
        assert(state.activeRow == 5); // wrapped past the top
        state = beltStepRow(state, 1);
        assert(state.activeRow == 0); // wrapped past the bottom
        state = beltStepRow(state, 13);
        assert(state.activeRow == 1); // multi-period delta still lands on-grid
    }

    // Horizontal scroll slides along the row and wraps.
    {
        BeltState state{6, 6, 3, 5};
        state = beltStepColumn(state, 1);
        assert(state.activeColumn == 0 && state.activeRow == 3);
        state = beltStepColumn(state, -1);
        assert(state.activeColumn == 5);
        state = beltStepColumn(state, -7);
        assert(state.activeColumn == 4);
    }

    // Jump goes straight to a cell; out-of-range clicks are no-ops.
    {
        BeltState state{6, 6, 0, 0};
        state = beltJumpTo(state, 4, 2);
        assert(state.activeRow == 4 && state.activeColumn == 2);
        state = beltJumpTo(state, 6, 0);
        assert(state.activeRow == 4 && state.activeColumn == 2);
        state = beltJumpTo(state, 0, -1);
        assert(state.activeRow == 4 && state.activeColumn == 2);
    }

    // The cross: full active row plus active column, intersection only once.
    {
        const BeltState state{6, 6, 2, 4};
        const std::vector<BeltCrossCell> cells = beltCrossCells(state);
        assert(cells.size() == 11); // 6 + 6 - 1 shared intersection

        int activeCount = 0;
        int horizontalCount = 0;
        for (const BeltCrossCell &cell : cells) {
            assert(cell.row == 2 || cell.column == 4); // on an arm
            if (cell.active) {
                ++activeCount;
                assert(cell.row == 2 && cell.column == 4);
                assert(cell.horizontal);
            }
            if (cell.horizontal) {
                ++horizontalCount;
                assert(cell.row == 2);
            }
        }
        assert(activeCount == 1);
        assert(horizontalCount == 6);
        for (int column = 0; column < 6; ++column) {
            assert(crossHasCell(cells, 2, column));
        }
        for (int row = 0; row < 6; ++row) {
            assert(crossHasCell(cells, row, 4));
        }
    }

    // Non-square grids keep the same math (the component is size-generic).
    {
        const BeltState state{2, 4, 1, 0};
        assert(beltStateValid(state));
        assert(beltActiveIndex(state) == 4);
        assert(beltCrossCells(state).size() == 5); // 4 + 2 - 1
        assert(beltStepRow(state, 1).activeRow == 0);
    }

    // Invalid states are inert: steps return them unchanged, the cross is empty.
    {
        const BeltState broken{0, 0, 0, 0};
        assert(beltStepRow(broken, 1).activeRow == 0);
        assert(beltCrossCells(broken).empty());
    }

    return 0;
}
