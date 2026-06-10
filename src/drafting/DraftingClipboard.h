#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

// The pure half of copy/paste. The clipboard itself is just a vector of
// DraftingObject snapshots (the controller owns the live one); the logic
// worth isolating and testing is PASTING: minting fresh, non-colliding ids
// and offsetting the geometry. Kept a free function over plain structs so a
// test can assert id sequence and offset math with no Qt and no controller.
struct DraftingPasteResult {
    std::vector<DraftingObject> objects; // fresh-id, offset clones, source order
    int nextSerial = 0;                  // serial past the last id minted
};

// Clone each source: a fresh id (prefix_serial, serial incrementing), geometry
// translated by (dx,dy), bounds recomputed; layer, style, metadata, locked and
// visible flags carried over verbatim — a paste is the same object somewhere
// else, not a new kind of thing. Deterministic given startSerial, so the
// controller threads its live serial counter through and gets it back advanced.
DraftingPasteResult planDraftingPaste(
    const std::vector<DraftingObject> &sources,
    const std::string &prefix, int startSerial, double dx, double dy);

} // namespace edi::drafting
