#pragma once

#include "drafting/DraftingArray.h" // DraftingArrayResult (kaleidoscope returns N copies)
#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingMirrorAxis {
    Horizontal,
    Vertical
};

struct DraftingMirrorResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingObject object;

    static DraftingMirrorResult accepted(DraftingObject object);
    static DraftingMirrorResult rejected(DraftingResultCode code, std::string message);
};

DraftingMirrorAxis draftingMirrorAxisFromId(const std::string &axisId);
DraftingMirrorResult mirrorDraftingObject(
    const DraftingObject &source,
    DraftingObjectId newObjectId,
    DraftingMirrorAxis axis);

// Kaleidoscope / dihedral mirror: reflect `source` across `axisCount` evenly spaced
// axes through `center` (axis i at i * 180/axisCount degrees), emitting one copy per
// axis. Reflection across an arbitrary axis is composed from the keystone +
// canonical mirror — rotate the axis to horizontal, apply the canonical per-kind
// reflection through `center`, rotate back — so it REUSES mirrorGeometry's existing
// per-kind orientation flips rather than re-deriving angle math. Gates on
// supportsMirror exactly as mirrorDraftingObject (so the unsupported kinds — Arc,
// Ellipse, Polygon, Polyline, Guide, Text, Spline — reject); also rejects
// axisCount < 1 and an id-count mismatch. The canonical single-axis
// mirrorDraftingObject path is untouched.
DraftingArrayResult kaleidoscopeMirror(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    Point2D center,
    int axisCount);

} // namespace edi::drafting
