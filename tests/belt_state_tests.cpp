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

    // ---- Occupancy-aware navigation (rows = tools, cells = sub-features) ----
    // A 4x4 belt shaped like the drafting arrangement: row 0 one tool,
    // row 1 empty, row 2 three items with a gap, row 3 two items.
    {
        const BeltState state{4, 4, 0, 0};
        std::vector<bool> occupied(16, false);
        occupied[0] = true;                            // row 0: lead only
        occupied[8] = occupied[9] = occupied[11] = true; // row 2: cols 0,1,3
        occupied[12] = occupied[13] = true;            // row 3: cols 0,1

        assert(beltCellOccupied(state, occupied, 2, 3));
        assert(!beltCellOccupied(state, occupied, 1, 0));
        assert(beltRowLeadColumn(state, occupied, 2) == 0);
        assert(beltRowLeadColumn(state, occupied, 1) == -1);

        // Row steps skip the empty row entirely and land on the lead.
        BeltState cursor = state;
        cursor = beltStepRowOccupied(cursor, 1, occupied);
        assert(cursor.activeRow == 2 && cursor.activeColumn == 0); // row 1 skipped
        cursor = beltStepRowOccupied(cursor, 1, occupied);
        assert(cursor.activeRow == 3);
        cursor = beltStepRowOccupied(cursor, 1, occupied);
        assert(cursor.activeRow == 0); // wrapped, still only non-empty rows
        cursor = beltStepRowOccupied(cursor, -1, occupied);
        assert(cursor.activeRow == 3);

        // Column steps skip gaps inside the row and wrap over the ragged end.
        cursor = beltJumpTo(cursor, 2, 1);
        cursor = beltStepColumnOccupied(cursor, 1, occupied);
        assert(cursor.activeColumn == 3); // col 2 is a gap
        cursor = beltStepColumnOccupied(cursor, 1, occupied);
        assert(cursor.activeColumn == 0); // wrapped past the ragged end
        cursor = beltStepColumnOccupied(cursor, -1, occupied);
        assert(cursor.activeColumn == 3);

        // A single-item row makes column steps a no-op instead of a spin.
        cursor = beltJumpTo(cursor, 0, 0);
        assert(beltStepColumnOccupied(cursor, 1, occupied).activeColumn == 0);

        // Normalize: an unoccupied cursor snaps to the first occupied cell.
        assert(beltNormalizeToOccupied({4, 4, 1, 2}, occupied).activeRow == 0);
        assert(beltNormalizeToOccupied({4, 4, 2, 3}, occupied).activeColumn == 3); // already occupied: kept

        // The view: vertical strip lists only non-empty rows (with leads);
        // horizontal strip lists the active row's items, gaps compacted away.
        const BeltCrossView view = beltCrossView({4, 4, 2, 3}, occupied);
        assert(view.rows.size() == 3);
        assert(view.rows[0].row == 0 && view.rows[1].row == 2 && view.rows[2].row == 3);
        assert(view.rows[1].leadColumn == 0);
        assert(view.activeRowPosition == 1);
        assert(view.items.size() == 3);
        assert(view.items[0].column == 0 && !view.items[0].active);
        assert(view.items[2].column == 3 && view.items[2].active);

        // All-empty belt: steps stay put, the view is empty, normalize keeps.
        const std::vector<bool> none(16, false);
        assert(beltStepRowOccupied(state, 1, none).activeRow == 0);
        assert(beltCrossView(state, none).rows.empty());
        assert(beltNormalizeToOccupied(state, none).activeRow == 0);

        // A wrong-length mask reads as all-empty, never as shifted rows.
        const std::vector<bool> ragged(15, true);
        assert(!beltCellOccupied(state, ragged, 0, 0));

        // The peek view: active row items plus the wrapping neighbours'
        // leads — exactly where one vertical step lands, by construction.
        {
            const BeltPeekView peek = beltPeekView({4, 4, 2, 3}, occupied);
            assert(peek.items.size() == 3);
            assert(peek.items[2].column == 3 && peek.items[2].active);
            assert(peek.previousRow == 0 && peek.previousLeadColumn == 0);
            assert(peek.nextRow == 3 && peek.nextLeadColumn == 0);
        }
        // From the top row the previous peek wraps to the bottom.
        {
            const BeltPeekView peek = beltPeekView({4, 4, 0, 0}, occupied);
            assert(peek.previousRow == 3);
            assert(peek.nextRow == 2);
        }
        // Two non-empty rows: the same row peeks on both sides (both scroll
        // directions reach it). One non-empty row: no peeks at all.
        {
            std::vector<bool> twoRows(16, false);
            twoRows[0] = true;
            twoRows[12] = true;
            const BeltPeekView peek = beltPeekView({4, 4, 0, 0}, twoRows);
            assert(peek.previousRow == 3 && peek.nextRow == 3);

            std::vector<bool> oneRow(16, false);
            oneRow[0] = oneRow[1] = true;
            const BeltPeekView solo = beltPeekView({4, 4, 0, 0}, oneRow);
            assert(solo.previousRow == -1 && solo.nextRow == -1);
            assert(solo.items.size() == 2);
        }

        // Row items: compacted, active flag only on the cursor's own cell.
        {
            const BeltState at23{4, 4, 2, 3};
            const std::vector<BeltItemEntry> row2 = beltRowItems(at23, occupied, 2);
            assert(row2.size() == 3);
            assert(row2[2].column == 3 && row2[2].active);
            const std::vector<BeltItemEntry> row3 = beltRowItems(at23, occupied, 3);
            assert(row3.size() == 2);
            assert(!row3[0].active && !row3[1].active); // cursor is on row 2
            assert(beltRowItems(at23, occupied, 1).empty());
        }

        // Pins: append-once, unpin removes, prune drops rows gone empty.
        {
            std::vector<int> pins;
            pins = beltPinRow(pins, 2);
            pins = beltPinRow(pins, 0);
            pins = beltPinRow(pins, 2);  // duplicate: no-op
            pins = beltPinRow(pins, -1); // invalid: no-op
            assert((pins == std::vector<int>{2, 0}));

            pins = beltUnpinRow(pins, 2);
            assert((pins == std::vector<int>{0}));
            pins = beltUnpinRow(pins, 5); // unknown: no-op
            assert((pins == std::vector<int>{0}));

            std::vector<int> stale = {0, 1, 3}; // row 1 is empty in this fixture
            stale = beltPrunePins(stale, state, occupied);
            assert((stale == std::vector<int>{0, 3}));
        }
    }

    return 0;
}
