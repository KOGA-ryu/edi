// footprintsOverlap unit tests (Phase-1 slice 3d / brief 062).
//
// WHY these tests live here (pure drafting, not in map_spec_store_tests)?
//   footprintsOverlap is in edi_drafting_core: a pure geometric primitive with no
//   IO dependency. Testing it here isolates the AABB logic from the TOML parser
//   path and lets edi_drafting_core itself own the test, consistent with every
//   other DraftingXxx module.  The parser behavior-preservation test is covered by
//   the existing map_spec_store_tests overlap rejects (which stays green).
#include "drafting/DraftingOverlap.h"

#include <cassert>

using namespace edi::drafting;

int main()
{
    // Clearly overlapping: 10×10 at (0,0) vs 10×10 at (5,5).
    assert(footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                             {5.0, 5.0}, 10.0, 10.0));

    // Clearly disjoint: 10×10 at (0,0) vs 10×10 at (20,0).
    assert(!footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                              {20.0, 0.0}, 10.0, 10.0));

    // Disjoint in Y, overlapping in X.
    assert(!footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                              {5.0, 20.0}, 10.0, 10.0));

    // Edge-touching in X (right edge of A == left edge of B, within eps).
    // Touching edges are NOT overlap — corridor-separated rooms share a boundary.
    assert(!footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                              {10.0, 0.0}, 10.0, 10.0));

    // Edge-touching in Y (bottom edge of A == top edge of B).
    assert(!footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                              {0.0, 10.0}, 10.0, 10.0));

    // A is strictly CONTAINED inside B — overlap in both axes, true.
    assert(footprintsOverlap({2.0, 2.0}, 3.0, 3.0,
                             {0.0, 0.0}, 10.0, 10.0));

    // Identical rectangles — maximum overlap.
    assert(footprintsOverlap({1.0, 1.0}, 5.0, 5.0,
                             {1.0, 1.0}, 5.0, 5.0));

    // A penetrates B by just over eps in X but is disjoint in Y → false overall.
    const double eps = kFootprintOverlapEps;
    assert(!footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                              {10.0 - eps * 0.5, 20.0}, 10.0, 10.0));

    // A penetrates B by 2×eps in X AND overlaps in Y → true (just over threshold).
    assert(footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                             {10.0 - eps * 2.0, 5.0}, 10.0, 10.0));

    // Custom eps: use a large eps so that 0.5-unit overlap is NOT considered overlap.
    assert(!footprintsOverlap({0.0, 0.0}, 10.0, 10.0,
                              {9.6, 0.0}, 10.0, 10.0,
                              1.0 /* eps = 1.0 unit */));

    return 0;
}
