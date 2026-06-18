#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingTypes.h"

#include <vector>

namespace edi::drafting {

// --- M2-S1: Materialize intersections as Point geometry ----------------------
//
// "Straight segment" for intersection purposes: the a→b endpoint pair of a
// Line or ConstructionLine.  Guides are EXCLUDED (they are infinite lines, not
// bounded segments).  Polyline edges, polygon edges, and wall centerlines are
// a documented FUTURE extension — do not add here without a brief.
//
// Only EFFECTIVELY VISIBLE objects participate: an object whose own `visible`
// flag is false, or whose layer is hidden, contributes nothing.
//
// Pairwise segmentIntersection is used (true crossing within both segments only).
// Near-coincident results — e.g. three segments through one point produce three
// pair-wise hits — are merged into one: two hits within 1e-6 of each other
// collapse to a single representative.

// All distinct crossing points among the document's visible straight segments.
// Returns an empty vector when there are fewer than two participating segments.
std::vector<Point2D> documentIntersectionPoints(const DraftingDocument &document);

// Same logic restricted to the identified subset of objects.
// Objects not in `ids`, or not in the document, are ignored.
// Used by the selection-scoped controller verb when a selection is active.
std::vector<Point2D> subsetIntersectionPoints(const DraftingDocument &document,
                                               const std::vector<DraftingObjectId> &ids);

// Idempotency filter: removes from `candidates` every point that is already
// within 1e-6 of an existing Point-kind object in the document.  The controller
// verb calls this before building DraftingObjects so a repeated "drop" is a
// no-op rather than a duplicate.  The tolerance matches the dedup pass above so
// the predicate is consistent end-to-end.
std::vector<Point2D> filterExistingIntersections(const DraftingDocument &document,
                                                  const std::vector<Point2D> &candidates);

} // namespace edi::drafting
