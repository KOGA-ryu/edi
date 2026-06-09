#include "drafting/DraftingCalibration.h"

#include "drafting/DraftingGeometry.h"

#include <cstddef>
#include <cmath>
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

} // namespace

DraftingCalibrationPatternResult DraftingCalibrationPatternResult::accepted(std::vector<DraftingObject> objects)
{
    return {true, DraftingResultCode::None, {}, std::move(objects)};
}

DraftingCalibrationPatternResult DraftingCalibrationPatternResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message), {}};
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

} // namespace edi::drafting
