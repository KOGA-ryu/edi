#include "widgets/BeltState.h"

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
