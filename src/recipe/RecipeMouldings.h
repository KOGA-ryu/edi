#pragma once

#include <optional>
#include <string>
#include <vector>

namespace edi::recipe {

// The classical moulding vocabulary, ported numerically from the user's
// working prototype (ascii_blender_dryrun_v0/profile_mouldings.py — "the
// proto-type tools"). A moulding is a SEQUENCE of named terms a mason
// would recognize — fillet, torus, scotia, cavetto, echinus, cyma — each
// one band of the profile: a height, a start/end radius, and the term's
// characteristic curve between them. The compiler expands the sequence
// into deterministic (z, radius) points for the mesh builders.
//
// PORT-FIDELITY CONTRACT: same easing curves, same default step counts,
// same 4-decimal rounding, same error wording as v0 — the prototype's
// pytest cases and its generated doric output are this module's goldens.

struct MouldingSegment {
    std::string term;
    double height = 0.0;
    // First segment must carry startRadius; later segments default to the
    // previous segment's end (the bands CHAIN — a mason cuts continuously).
    std::optional<double> startRadius;
    std::optional<double> endRadius;    // absolute target...
    std::optional<double> radiusDelta;  // ...or relative; absolute wins
    std::optional<int> steps;           // default depends on the term
};

struct MouldingPoint {
    std::string term; // "<term>_start" / "<term>_01" ... — pointable labels
    double z = 0.0;      // relative to the moulding's base
    double radius = 0.0;
};

struct MouldingCompileResult {
    bool ok = false;
    std::string message;
    std::vector<MouldingPoint> points;
};

// True for terms the vocabulary knows (data table in the .cpp).
bool mouldingTermSupported(const std::string &term);

// Expands one named sequence into profile points. `name` is only for the
// error messages — the same pointable wording the prototype uses.
MouldingCompileResult compileMouldingSequence(const std::string &name,
                                              const std::vector<MouldingSegment> &sequence);

} // namespace edi::recipe
