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

// Maps an axis id onto repeat settings: the caller supplies count and spacing
// (tool-option state), this function only decides which axis the spacing
// rides on. Unknown axis -> nullopt.
std::optional<DraftingArrayRepeatSettings> draftingArrayRepeatSettingsFromAxisId(
    const std::string &axisId,
    int copyCount,
    double spacing);

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

} // namespace edi::drafting
