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

struct DraftingCalibrationMeasurement {
    std::string patternId;
    std::vector<DraftingObjectId> objectIds;
    double expectedValue = 0.0;
    double measuredValue = 0.0;
    double errorValue = 0.0;
    double percentError = 0.0;
    std::string source;
};

struct DraftingCalibrationMeasurementRequest {
    std::vector<DraftingObject> objects;
    double measuredValue = 0.0;
    std::string source;
};

struct DraftingCalibrationMeasurementResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingCalibrationMeasurement measurement;

    static DraftingCalibrationMeasurementResult accepted(DraftingCalibrationMeasurement measurement);
    static DraftingCalibrationMeasurementResult rejected(DraftingResultCode code, std::string message);
};

struct DraftingCalibrationCorrectionPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::string patternId;
    double expectedValue = 0.0;
    double measuredValue = 0.0;
    double scaleFactor = 1.0;
    double correctionPercent = 0.0;

    static DraftingCalibrationCorrectionPlan accepted(const DraftingCalibrationMeasurement &measurement);
    static DraftingCalibrationCorrectionPlan rejected(DraftingResultCode code, std::string message);
};

DraftingCalibrationPatternKind draftingCalibrationPatternKindFromId(const std::string &patternId);
const char *draftingCalibrationPatternKindName(DraftingCalibrationPatternKind kind);
DraftingCalibrationPatternResult buildDraftingCalibrationPattern(const DraftingCalibrationPatternRequest &request);
DraftingCalibrationMeasurementResult measureDraftingCalibrationPattern(const DraftingCalibrationMeasurementRequest &request);
DraftingCalibrationCorrectionPlan planDraftingCalibrationCorrection(const DraftingCalibrationMeasurement &measurement);
std::string formatDraftingCalibrationMeasurementNote(const DraftingCalibrationMeasurement &measurement);

} // namespace edi::drafting
