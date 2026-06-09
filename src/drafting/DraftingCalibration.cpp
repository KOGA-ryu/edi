#include "drafting/DraftingCalibration.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace edi::drafting {
namespace {

bool calibrationRequestIsFinite(const DraftingCalibrationPatternRequest &request)
{
    return std::isfinite(request.origin.x)
        && std::isfinite(request.origin.y)
        && std::isfinite(request.size)
        && std::isfinite(request.spacing);
}

DraftingObjectBuildResult buildCalibrationObject(
    const DraftingCalibrationPatternRequest &request,
    const std::string &suffix,
    DraftingShapeKind kind,
    DraftingGeometry geometry)
{
    auto built = buildDraftingObject(request.idPrefix + "_" + suffix, kind, std::move(geometry));
    if (!built.ok) {
        return built;
    }
    built.object.layerId = request.layerId;
    built.object.bounds = computeBounds(built.object.geometry);
    built.object.metadata.toolProvenance = std::string("calibration_") + draftingCalibrationPatternKindName(request.kind);
    return built;
}

bool appendBuiltObject(
    std::vector<DraftingObject> &objects,
    const DraftingObjectBuildResult &built)
{
    if (!built.ok) {
        return false;
    }
    objects.push_back(built.object);
    return true;
}

bool startsWith(const std::string &value, const std::string &prefix)
{
    return value.rfind(prefix, 0) == 0;
}

std::string calibrationPatternId(const DraftingObject &object)
{
    if (!startsWith(object.metadata.toolProvenance, "calibration_")) {
        return {};
    }
    return object.metadata.toolProvenance;
}

double expectedSquareSize(const DraftingObject &object)
{
    const auto *geometry = std::get_if<RectangleGeometry>(&object.geometry);
    if (geometry == nullptr || geometry->width <= 0.0 || geometry->height <= 0.0) {
        return 0.0;
    }
    return (geometry->width + geometry->height) / 2.0;
}

double expectedCircleDiameter(const DraftingObject &object)
{
    const auto *geometry = std::get_if<CircleGeometry>(&object.geometry);
    if (geometry == nullptr || geometry->radius <= 0.0) {
        return 0.0;
    }
    return geometry->radius * 2.0;
}

double expectedLineSpacing(const std::vector<DraftingObject> &objects)
{
    if (objects.size() < 2) {
        return 0.0;
    }

    std::vector<double> yValues;
    yValues.reserve(objects.size());
    for (const DraftingObject &object : objects) {
        const auto *geometry = std::get_if<LineGeometry>(&object.geometry);
        if (geometry == nullptr) {
            return 0.0;
        }
        yValues.push_back((geometry->a.y + geometry->b.y) / 2.0);
    }
    std::sort(yValues.begin(), yValues.end());

    double totalSpacing = 0.0;
    for (std::size_t index = 1; index < yValues.size(); ++index) {
        const double spacing = yValues[index] - yValues[index - 1];
        if (spacing <= 0.0 || !std::isfinite(spacing)) {
            return 0.0;
        }
        totalSpacing += spacing;
    }
    return totalSpacing / static_cast<double>(yValues.size() - 1);
}

double expectedCalibrationValue(const std::string &patternId, const std::vector<DraftingObject> &objects)
{
    if (patternId == "calibration_square") {
        return objects.size() == 1 ? expectedSquareSize(objects.front()) : 0.0;
    }
    if (patternId == "calibration_circle") {
        return objects.size() == 1 ? expectedCircleDiameter(objects.front()) : 0.0;
    }
    if (patternId == "calibration_line_spacing") {
        return expectedLineSpacing(objects);
    }
    return 0.0;
}

std::string fixedNumber(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
}

} // namespace

DraftingCalibrationPatternResult DraftingCalibrationPatternResult::accepted(std::vector<DraftingObject> objects)
{
    return {true, DraftingResultCode::None, {}, std::move(objects)};
}

DraftingCalibrationPatternResult DraftingCalibrationPatternResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message), {}};
}

DraftingCalibrationMeasurementResult DraftingCalibrationMeasurementResult::accepted(DraftingCalibrationMeasurement measurement)
{
    return {true, DraftingResultCode::None, {}, std::move(measurement)};
}

DraftingCalibrationMeasurementResult DraftingCalibrationMeasurementResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message), {}};
}

DraftingCalibrationCorrectionPlan DraftingCalibrationCorrectionPlan::accepted(const DraftingCalibrationMeasurement &measurement)
{
    const double scaleFactor = measurement.expectedValue / measurement.measuredValue;
    return {
        true,
        DraftingResultCode::None,
        {},
        measurement.patternId,
        measurement.expectedValue,
        measurement.measuredValue,
        scaleFactor,
        (scaleFactor - 1.0) * 100.0,
    };
}

DraftingCalibrationCorrectionPlan DraftingCalibrationCorrectionPlan::rejected(DraftingResultCode code, std::string message)
{
    return {
        false,
        code,
        std::move(message),
        {},
        0.0,
        0.0,
        1.0,
        0.0,
    };
}

DraftingCalibrationPatternKind draftingCalibrationPatternKindFromId(const std::string &patternId)
{
    if (patternId == "test_circle") {
        return DraftingCalibrationPatternKind::Circle;
    }
    if (patternId == "line_spacing") {
        return DraftingCalibrationPatternKind::LineSpacing;
    }
    return DraftingCalibrationPatternKind::Square;
}

const char *draftingCalibrationPatternKindName(DraftingCalibrationPatternKind kind)
{
    switch (kind) {
    case DraftingCalibrationPatternKind::Circle:
        return "circle";
    case DraftingCalibrationPatternKind::LineSpacing:
        return "line_spacing";
    case DraftingCalibrationPatternKind::Square:
        return "square";
    }
    return "square";
}

DraftingCalibrationPatternRequest defaultDraftingCalibrationPatternRequest(
    DraftingCalibrationPatternKind kind,
    DraftingObjectId idPrefix,
    LayerId layerId)
{
    DraftingCalibrationPatternRequest request;
    request.kind = kind;
    request.idPrefix = std::move(idPrefix);
    request.layerId = std::move(layerId);
    request.origin = {0.15, 0.15};
    request.size = 0.24;
    request.spacing = 0.04;
    request.lineCount = 5;
    return request;
}

DraftingCalibrationPatternResult buildDraftingCalibrationPattern(const DraftingCalibrationPatternRequest &request)
{
    if (!isValidDraftingObjectId(request.idPrefix)) {
        return DraftingCalibrationPatternResult::rejected(DraftingResultCode::EmptyObjectId, "calibration id prefix is invalid");
    }
    if (!isValidLayerId(request.layerId)) {
        return DraftingCalibrationPatternResult::rejected(DraftingResultCode::LayerNotFound, "calibration layer id is invalid");
    }
    if (!calibrationRequestIsFinite(request) || request.size <= 0.0 || request.spacing <= 0.0 || request.lineCount <= 0) {
        return DraftingCalibrationPatternResult::rejected(DraftingResultCode::InvalidGeometry, "calibration dimensions must be positive and finite");
    }

    std::vector<DraftingObject> objects;
    if (request.kind == DraftingCalibrationPatternKind::Square) {
        const auto built = buildCalibrationObject(
            request,
            "square",
            DraftingShapeKind::Rectangle,
            RectangleGeometry{request.origin, request.size, request.size});
        if (!appendBuiltObject(objects, built)) {
            return DraftingCalibrationPatternResult::rejected(built.code, built.message);
        }
    } else if (request.kind == DraftingCalibrationPatternKind::Circle) {
        const Point2D center{request.origin.x + request.size * 0.5, request.origin.y + request.size * 0.5};
        const auto built = buildCalibrationObject(
            request,
            "circle",
            DraftingShapeKind::Circle,
            CircleGeometry{center, request.size * 0.5});
        if (!appendBuiltObject(objects, built)) {
            return DraftingCalibrationPatternResult::rejected(built.code, built.message);
        }
    } else if (request.kind == DraftingCalibrationPatternKind::LineSpacing) {
        objects.reserve(static_cast<std::size_t>(request.lineCount));
        for (int index = 0; index < request.lineCount; ++index) {
            const double y = request.origin.y + static_cast<double>(index) * request.spacing;
            const auto built = buildCalibrationObject(
                request,
                "line_" + std::to_string(index + 1),
                DraftingShapeKind::Line,
                LineGeometry{{request.origin.x, y}, {request.origin.x + request.size, y}});
            if (!appendBuiltObject(objects, built)) {
                return DraftingCalibrationPatternResult::rejected(built.code, built.message);
            }
        }
    }

    return DraftingCalibrationPatternResult::accepted(std::move(objects));
}

DraftingCalibrationMeasurementResult measureDraftingCalibrationPattern(const DraftingCalibrationMeasurementRequest &request)
{
    if (request.objects.empty()) {
        return DraftingCalibrationMeasurementResult::rejected(DraftingResultCode::InvalidSelectionTarget, "calibration measurement requires selected pattern objects");
    }
    if (!std::isfinite(request.measuredValue) || request.measuredValue <= 0.0) {
        return DraftingCalibrationMeasurementResult::rejected(DraftingResultCode::InvalidGeometry, "measured value must be positive and finite");
    }

    const std::string patternId = calibrationPatternId(request.objects.front());
    if (patternId.empty()) {
        return DraftingCalibrationMeasurementResult::rejected(DraftingResultCode::InvalidMetadata, "selected object is not a calibration pattern");
    }

    DraftingCalibrationMeasurement measurement;
    measurement.patternId = patternId;
    measurement.measuredValue = request.measuredValue;
    measurement.source = request.source;
    measurement.objectIds.reserve(request.objects.size());
    for (const DraftingObject &object : request.objects) {
        if (calibrationPatternId(object) != patternId) {
            return DraftingCalibrationMeasurementResult::rejected(DraftingResultCode::InvalidSelectionTarget, "calibration measurement requires one pattern type");
        }
        measurement.objectIds.push_back(object.id);
    }

    measurement.expectedValue = expectedCalibrationValue(patternId, request.objects);
    if (!std::isfinite(measurement.expectedValue) || measurement.expectedValue <= 0.0) {
        return DraftingCalibrationMeasurementResult::rejected(DraftingResultCode::InvalidGeometry, "calibration pattern cannot provide an expected measurement");
    }

    measurement.errorValue = measurement.measuredValue - measurement.expectedValue;
    measurement.percentError = (measurement.errorValue / measurement.expectedValue) * 100.0;
    return DraftingCalibrationMeasurementResult::accepted(std::move(measurement));
}

DraftingCalibrationMeasurementResult measureSelectedDraftingCalibrationPattern(
    const DraftingDocument &document,
    const std::vector<DraftingObjectId> &objectIds,
    double measuredValue,
    std::string source)
{
    std::vector<DraftingObject> selectedObjects;
    selectedObjects.reserve(objectIds.size());
    for (const DraftingObjectId &objectId : objectIds) {
        const DraftingObject *object = findObject(document, objectId);
        if (object == nullptr) {
            return DraftingCalibrationMeasurementResult::rejected(
                DraftingResultCode::InvalidSelectionTarget,
                "calibration selection target does not exist");
        }
        selectedObjects.push_back(*object);
    }

    return measureDraftingCalibrationPattern({std::move(selectedObjects), measuredValue, std::move(source)});
}

DraftingCalibrationCorrectionPlan planDraftingCalibrationCorrection(const DraftingCalibrationMeasurement &measurement)
{
    if (measurement.patternId.empty()) {
        return DraftingCalibrationCorrectionPlan::rejected(DraftingResultCode::InvalidMetadata, "calibration correction requires a pattern id");
    }
    if (!std::isfinite(measurement.expectedValue) || measurement.expectedValue <= 0.0) {
        return DraftingCalibrationCorrectionPlan::rejected(DraftingResultCode::InvalidGeometry, "calibration correction requires a positive expected value");
    }
    if (!std::isfinite(measurement.measuredValue) || measurement.measuredValue <= 0.0) {
        return DraftingCalibrationCorrectionPlan::rejected(DraftingResultCode::InvalidGeometry, "calibration correction requires a positive measured value");
    }
    return DraftingCalibrationCorrectionPlan::accepted(measurement);
}

std::string formatDraftingCalibrationMeasurementNote(const DraftingCalibrationMeasurement &measurement)
{
    return "calibration "
        + measurement.patternId
        + " expected="
        + fixedNumber(measurement.expectedValue)
        + " measured="
        + fixedNumber(measurement.measuredValue)
        + " error="
        + fixedNumber(measurement.errorValue)
        + " percent="
        + fixedNumber(measurement.percentError);
}

} // namespace edi::drafting
