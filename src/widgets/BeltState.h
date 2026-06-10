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

// ---- Occupancy-aware navigation and view (user feedback 2026-06-10) -------
//
// The belt's mental model: each ROW is one tool/feature, its sub-features
// fill the row's cells. Rows have different lengths, so navigation must skip
// empty cells and the view must never show a blank box. Occupancy is passed
// in as data (row-major bools, the caller derives it from its item list) —
// the state stays a dumb cursor, and the same functions serve any host.

// occupied.size() must be rows*columns; anything else reads as all-empty.

// True when the cell at (row, column) is occupied.
bool beltCellOccupied(const BeltState &state, const std::vector<bool> &occupied, int row, int column);

// First occupied column of `row` (its "lead" item), or -1 for an empty row.
int beltRowLeadColumn(const BeltState &state, const std::vector<bool> &occupied, int row);

// Snap an arbitrary cursor onto occupancy: keep the active cell if occupied,
// else the first occupied cell on the belt, else unchanged (an all-empty
// belt has nothing to point at).
BeltState beltNormalizeToOccupied(BeltState state, const std::vector<bool> &occupied);

// Vertical scroll: move to the next/previous row that has any item, wrapping;
// the column snaps to that row's lead. Lands nowhere new on an all-empty belt.
BeltState beltStepRowOccupied(BeltState state, int delta, const std::vector<bool> &occupied);

// Horizontal scroll: next/previous occupied cell within the active row,
// wrapping. No-op when the row has fewer than two items.
BeltState beltStepColumnOccupied(BeltState state, int delta, const std::vector<bool> &occupied);

// The compacted view the widget renders. Visual positions are list indexes,
// NOT grid columns: the vertical strip stacks non-empty rows top-to-bottom,
// and the horizontal strip lays the active row's items left-to-right, so the
// screen never shows the gaps the grid may contain.
struct BeltRowEntry {
    int row = 0;        // grid row
    int leadColumn = 0; // grid column of the row's lead item
};
struct BeltItemEntry {
    int column = 0;     // grid column
    bool active = false;
};
struct BeltCrossView {
    std::vector<BeltRowEntry> rows;     // vertical strip, one entry per non-empty row
    int activeRowPosition = -1;         // index into rows of the active row
    std::vector<BeltItemEntry> items;   // horizontal strip: the active row's items
};
BeltCrossView beltCrossView(const BeltState &state, const std::vector<bool> &occupied);

// The carousel presentation (user feedback 2026-06-10): only the active
// row's items render full-size; the previous/next non-empty rows peek in as
// half-cells above and below, hinting what a vertical scroll reaches. The
// peeks wrap like the scroll does; with two rows the same row peeks on both
// sides (both scroll directions reach it), and with one row there is no
// peek at all (rows < 0).
struct BeltPeekView {
    std::vector<BeltItemEntry> items;   // active row, compacted, one active
    int previousRow = -1;               // -1 = nothing to peek
    int previousLeadColumn = -1;
    int nextRow = -1;
    int nextLeadColumn = -1;
};
BeltPeekView beltPeekView(const BeltState &state, const std::vector<bool> &occupied);

} // namespace edi::shell
