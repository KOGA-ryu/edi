#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace edi::drafting {

struct DraftingArrayResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects;

    static DraftingArrayResult accepted(std::vector<DraftingObject> objects);
    static DraftingArrayResult rejected(DraftingResultCode code, std::string message);
};

struct DraftingArrayRepeatSettings {
    int copyCount = 0;
    double spacingX = 0.0;
    double spacingY = 0.0;
};

// Maps an axis id onto repeat settings: the caller supplies count and both
// axis spacings (tool-option state); this function decides which spacing the
// chosen axis keeps and zeroes the other, so the axis logic lives in exactly
// one place. Unknown axis -> nullopt.
std::optional<DraftingArrayRepeatSettings> draftingArrayRepeatSettingsFromAxisId(
    const std::string &axisId,
    int copyCount,
    double spacingX,
    double spacingY);

DraftingArrayResult repeatDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    double spacingX,
    double spacingY);

// Grid array: the source occupies cell (0,0); copies fill the remaining
// columns*rows - 1 cells (row-major), so newObjectIds.size() must equal
// columns*rows - 1. Spacing on an axis with more than one cell must be
// non-zero, or the copies would stack invisibly on each other.
DraftingArrayResult gridArrayDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    int columns,
    int rows,
    double spacingX,
    double spacingY);

// Radial array: copies fill the remaining slots of an evenly divided ring
// around `center` that includes the source, stepping 360/(copies+1) degrees
// from the source's bounds centre. Placement only — copies keep their
// orientation, because axis-aligned geometry kinds (rectangle, guide,
// construction line) cannot represent a rotated shape.
DraftingArrayResult radialArrayDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    Point2D center);

// Rotate-copies rosette: like radialArrayDraftingObject, copies fill the remaining
// slots of a fan/ring divided evenly among copies + source about `center` (step =
// totalAngleDeg / (copies + 1), matching the radial arm distribution). UNLIKE the
// placement-only radial array, each copy is actually ROTATED to its spoke via
// transformGeometry (rotation only, scale 1.0) — so rectangles/walls/arcs turn with
// the fan. Guides are rejected (a guide cannot rotate). The placement-only
// radialArrayDraftingObject is left intact as the non-rotating variant.
DraftingArrayResult rotateCopiesDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    Point2D center,
    double totalAngleDeg);

// Distribute copies of `source` along `path`, optionally rotated to the path tangent.
// Supported path kinds: Line, Polyline, Arc, Spline — others are rejected.
// `K = newObjectIds.size()` copies are placed at evenly arc-length-spaced stations
// that span the full path (both endpoints included for K ≥ 2; the single copy sits
// at the path start for K == 1). Each copy's BOUNDS CENTRE lands on its station.
// When `alignToTangent` is true, each copy is also rotated so its orientation matches
// the path tangent at that station.
// Controller wiring (the path-pick gesture) is deferred to edi-ui.
DraftingArrayResult arrayAlongCurve(const DraftingObject &source,
                                    const std::vector<DraftingObjectId> &newObjectIds,
                                    const DraftingGeometry &path,
                                    bool alignToTangent);

} // namespace edi::drafting
