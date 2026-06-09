#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingCalibrationPatternKind {
    Square,
    Circle,
    LineSpacing,
};

struct DraftingCalibrationPatternRequest {
    DraftingCalibrationPatternKind kind = DraftingCalibrationPatternKind::Square;
    DraftingObjectId idPrefix;
    LayerId layerId = "default";
    Point2D origin{0.1, 0.1};
    double size = 0.2;
    double spacing = 0.03;
    int lineCount = 5;
};

struct DraftingCalibrationPatternResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects;

    static DraftingCalibrationPatternResult accepted(std::vector<DraftingObject> objects);
    static DraftingCalibrationPatternResult rejected(DraftingResultCode code, std::string message);
};

DraftingCalibrationPatternKind draftingCalibrationPatternKindFromId(const std::string &patternId);
const char *draftingCalibrationPatternKindName(DraftingCalibrationPatternKind kind);
DraftingCalibrationPatternResult buildDraftingCalibrationPattern(const DraftingCalibrationPatternRequest &request);

} // namespace edi::drafting
