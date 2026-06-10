#pragma once

#include <vector>

namespace edi::shell {

// State of a grid "weapon belt": a rows x columns table of slots with one
// active cell. The UI renders only the cross through the active cell (its
// row and its column); scrolling moves along the arms and wraps at edges.
// Pure data + free functions, no Qt and no drafting knowledge — the belt is
// a reusable game-UI component; what lives in the slots is the caller's
// business (the widget gets items as data, this module only does the math).
struct BeltState {
    int rows = 6;
    int columns = 6;
    int activeRow = 0;
    int activeColumn = 0;
};

bool beltStateValid(const BeltState &state);

// Row-major index of the active cell — the contract between the state and
// the caller's flat item list (slot i holds item i).
int beltActiveIndex(const BeltState &state);

// Vertical scroll: move the active cell delta rows, wrapping at the edges.
BeltState beltStepRow(BeltState state, int delta);

// Horizontal scroll: slide along the active row, wrapping at the edges.
BeltState beltStepColumn(BeltState state, int delta);

// Click a visible cell: jump straight to it. Out-of-range coordinates leave
// the state unchanged (a click outside the cross is a no-op, not a clamp).
BeltState beltJumpTo(BeltState state, int row, int column);

// One renderable cell of the cross.
struct BeltCrossCell {
    int row = 0;
    int column = 0;
    bool horizontal = false; // cell sits on the active-row arm
    bool active = false;     // the intersection (always on the horizontal arm)
};

// The cells the widget paints: the active row left-to-right, then the active
// column top-to-bottom with the intersection skipped (it is already in the
// row arm). Everything else on the belt is invisible by design.
std::vector<BeltCrossCell> beltCrossCells(const BeltState &state);

} // namespace edi::shell
