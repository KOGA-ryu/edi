#include "widgets/BeltState.h"

#include <cstddef>

namespace edi::shell {

namespace {

// Euclidean modulo: C++'s % keeps the dividend's sign (-1 % 6 == -1), which
// would index off the grid when scrolling backwards past the edge. Adding
// one period before the second % maps any delta into [0, size).
int wrapIndex(int value, int size)
{
    return ((value % size) + size) % size;
}

} // namespace

bool beltStateValid(const BeltState &state)
{
    return state.rows > 0 && state.columns > 0
        && state.activeRow >= 0 && state.activeRow < state.rows
        && state.activeColumn >= 0 && state.activeColumn < state.columns;
}

int beltActiveIndex(const BeltState &state)
{
    return state.activeRow * state.columns + state.activeColumn;
}

BeltState beltStepRow(BeltState state, int delta)
{
    if (!beltStateValid(state)) {
        return state;
    }
    state.activeRow = wrapIndex(state.activeRow + delta, state.rows);
    return state;
}

BeltState beltStepColumn(BeltState state, int delta)
{
    if (!beltStateValid(state)) {
        return state;
    }
    state.activeColumn = wrapIndex(state.activeColumn + delta, state.columns);
    return state;
}

BeltState beltJumpTo(BeltState state, int row, int column)
{
    if (!beltStateValid(state)
        || row < 0 || row >= state.rows
        || column < 0 || column >= state.columns) {
        return state;
    }
    state.activeRow = row;
    state.activeColumn = column;
    return state;
}

bool beltCellOccupied(const BeltState &state, const std::vector<bool> &occupied, int row, int column)
{
    if (!beltStateValid(state) || row < 0 || row >= state.rows || column < 0 || column >= state.columns) {
        return false;
    }
    // A mask of the wrong length cannot be trusted row-major; treat it as
    // all-empty rather than reading some other row's bit.
    if (occupied.size() != static_cast<std::size_t>(state.rows) * static_cast<std::size_t>(state.columns)) {
        return false;
    }
    return occupied[static_cast<std::size_t>(row) * static_cast<std::size_t>(state.columns)
        + static_cast<std::size_t>(column)];
}

int beltRowLeadColumn(const BeltState &state, const std::vector<bool> &occupied, int row)
{
    for (int column = 0; column < state.columns; ++column) {
        if (beltCellOccupied(state, occupied, row, column)) {
            return column;
        }
    }
    return -1;
}

BeltState beltNormalizeToOccupied(BeltState state, const std::vector<bool> &occupied)
{
    if (!beltStateValid(state) || beltCellOccupied(state, occupied, state.activeRow, state.activeColumn)) {
        return state;
    }
    for (int row = 0; row < state.rows; ++row) {
        const int lead = beltRowLeadColumn(state, occupied, row);
        if (lead >= 0) {
            state.activeRow = row;
            state.activeColumn = lead;
            return state;
        }
    }
    return state; // all-empty belt: nothing to point at
}

BeltState beltStepRowOccupied(BeltState state, int delta, const std::vector<bool> &occupied)
{
    if (!beltStateValid(state) || delta == 0) {
        return state;
    }
    const int direction = delta > 0 ? 1 : -1;
    int remaining = delta * direction;
    int row = state.activeRow;
    // Walk one row at a time, counting only rows that hold an item; the hop
    // cap bounds each step at one full lap so an all-empty belt terminates.
    while (remaining > 0) {
        bool found = false;
        for (int hops = 0; hops < state.rows; ++hops) {
            row = wrapIndex(row + direction, state.rows);
            if (beltRowLeadColumn(state, occupied, row) >= 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return state;
        }
        --remaining;
    }
    state.activeRow = row;
    // Entering a row lands on its lead item — the row IS the tool, the lead
    // is its default sub-feature.
    state.activeColumn = beltRowLeadColumn(state, occupied, row);
    return state;
}

BeltState beltStepColumnOccupied(BeltState state, int delta, const std::vector<bool> &occupied)
{
    if (!beltStateValid(state) || delta == 0) {
        return state;
    }
    const int direction = delta > 0 ? 1 : -1;
    int remaining = delta * direction;
    int column = state.activeColumn;
    while (remaining > 0) {
        bool found = false;
        for (int hops = 0; hops < state.columns; ++hops) {
            column = wrapIndex(column + direction, state.columns);
            if (beltCellOccupied(state, occupied, state.activeRow, column)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return state;
        }
        --remaining;
    }
    state.activeColumn = column;
    return state;
}

BeltCrossView beltCrossView(const BeltState &state, const std::vector<bool> &occupied)
{
    BeltCrossView view;
    if (!beltStateValid(state)) {
        return view;
    }
    for (int row = 0; row < state.rows; ++row) {
        const int lead = beltRowLeadColumn(state, occupied, row);
        if (lead < 0) {
            continue; // empty rows do not exist on screen
        }
        if (row == state.activeRow) {
            view.activeRowPosition = static_cast<int>(view.rows.size());
        }
        view.rows.push_back({row, lead});
    }
    if (view.activeRowPosition < 0) {
        return view; // active row holds nothing: vertical strip only
    }
    for (int column = 0; column < state.columns; ++column) {
        if (beltCellOccupied(state, occupied, state.activeRow, column)) {
            view.items.push_back({column, column == state.activeColumn});
        }
    }
    return view;
}

std::vector<BeltItemEntry> beltRowItems(const BeltState &state, const std::vector<bool> &occupied, int row)
{
    std::vector<BeltItemEntry> items;
    if (!beltStateValid(state)) {
        return items;
    }
    for (int column = 0; column < state.columns; ++column) {
        if (beltCellOccupied(state, occupied, row, column)) {
            items.push_back({column, row == state.activeRow && column == state.activeColumn});
        }
    }
    return items;
}

std::vector<int> beltPinRow(std::vector<int> pins, int row)
{
    if (row < 0) {
        return pins;
    }
    for (const int pinned : pins) {
        if (pinned == row) {
            return pins; // already frozen; one quick bar per row
        }
    }
    pins.push_back(row);
    return pins;
}

std::vector<int> beltUnpinRow(std::vector<int> pins, int row)
{
    std::vector<int> kept;
    kept.reserve(pins.size());
    for (const int pinned : pins) {
        if (pinned != row) {
            kept.push_back(pinned);
        }
    }
    return kept;
}

std::vector<int> beltPrunePins(std::vector<int> pins, const BeltState &state, const std::vector<bool> &occupied)
{
    std::vector<int> kept;
    kept.reserve(pins.size());
    for (const int pinned : pins) {
        if (beltRowLeadColumn(state, occupied, pinned) >= 0) {
            kept.push_back(pinned);
        }
    }
    return kept;
}

BeltPeekView beltPeekView(const BeltState &state, const std::vector<bool> &occupied)
{
    BeltPeekView view;
    if (!beltStateValid(state)) {
        return view;
    }
    view.items = beltRowItems(state, occupied, state.activeRow);
    // The peeks ARE one vertical step: derive them from the step function so
    // what the half-cell shows and where the scroll lands cannot disagree.
    const BeltState previous = beltStepRowOccupied(state, -1, occupied);
    if (previous.activeRow != state.activeRow) {
        view.previousRow = previous.activeRow;
        view.previousLeadColumn = previous.activeColumn;
    }
    const BeltState next = beltStepRowOccupied(state, 1, occupied);
    if (next.activeRow != state.activeRow) {
        view.nextRow = next.activeRow;
        view.nextLeadColumn = next.activeColumn;
    }
    return view;
}

std::vector<BeltCrossCell> beltCrossCells(const BeltState &state)
{
    std::vector<BeltCrossCell> cells;
    if (!beltStateValid(state)) {
        return cells;
    }
    cells.reserve(static_cast<size_t>(state.columns + state.rows) - 1);
    for (int column = 0; column < state.columns; ++column) {
        cells.push_back({state.activeRow, column, true, column == state.activeColumn});
    }
    for (int row = 0; row < state.rows; ++row) {
        if (row == state.activeRow) {
            continue; // the intersection already sits in the row arm
        }
        cells.push_back({row, state.activeColumn, false, false});
    }
    return cells;
}

} // namespace edi::shell
