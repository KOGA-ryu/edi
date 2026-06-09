#include "drafting/DraftingPhysicalEdit.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingNumericEdit.h"

#include <cmath>
#include <utility>

namespace edi::drafting {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMinLength = 0.000001;

bool validPhysicalGrid(const DraftingGridProjection &grid)
{
    return std::isfinite(grid.settings.width)
        && std::isfinite(grid.settings.height)
        && grid.settings.width > 0.0
        && grid.settings.height > 0.0;
}

DraftingPhysicalGeometryEditPlan commandFromNumericEdit(
    const DraftingObject &object,
    const std::string &fieldId,
    double value)
{
    const DraftingNumericEditResult edit = applyNumericGeometryEdit(object, fieldId, value);
    if (!edit.ok) {
        return DraftingPhysicalGeometryEditPlan::rejected(edit.code, edit.message);
    }
    return DraftingPhysicalGeometryEditPlan::accepted(NumericGeometryEditCommand{object.id, fieldId, value});
}

DraftingPhysicalGeometryEditPlan commandFromGeometry(
    const DraftingObject &object,
    DraftingGeometry geometry)
{
    DraftingObject candidate = object;
    candidate.geometry = geometry;
    const auto validation = validateDraftingObjectShape(candidate);
    if (!validation.ok) {
        return DraftingPhysicalGeometryEditPlan::rejected(validation.code, validation.message);
    }
    return DraftingPhysicalGeometryEditPlan::accepted(UpdateGeometryCommand{object.id, std::move(geometry)});
}

DraftingPhysicalGeometryEditPlan planPhysicalLineEdit(
    const DraftingObject &object,
    const LineGeometry &line,
    const std::string &fieldId,
    double value,
    double width,
    double height)
{
    const double ax = line.a.x * width;
    const double ay = line.a.y * height;
    const double bx = line.b.x * width;
    const double by = line.b.y * height;
    const double dx = bx - ax;
    const double dy = by - ay;
    const double currentLength = std::sqrt(dx * dx + dy * dy);
    const double angle = fieldId == "line_angle_deg"
        ? value * kPi / 180.0
        : std::atan2(dy, dx);
    const double length = fieldId == "line_length" ? value : currentLength;
    if (!std::isfinite(angle) || !std::isfinite(length) || length < 0.0) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "line physical edit is invalid");
    }

    LineGeometry next = line;
    next.b = {
        (ax + std::cos(angle) * length) / width,
        (ay + std::sin(angle) * length) / height,
    };
    return commandFromGeometry(object, DraftingGeometry{next});
}

DraftingPhysicalGeometryEditPlan planPhysicalDimensionEdit(
    const DraftingObject &object,
    const DimensionGeometry &dimension,
    const std::string &fieldId,
    double value,
    double width,
    double height)
{
    if (fieldId == "dimension_angle_deg"
        && (dimension.kind == DimensionKind::Width || dimension.kind == DimensionKind::Height)) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension angle does not apply to width or height dimensions");
    }

    const double ax = dimension.a.x * width;
    const double ay = dimension.a.y * height;
    const double bx = dimension.b.x * width;
    const double by = dimension.b.y * height;
    const double dx = bx - ax;
    const double dy = by - ay;
    const double currentLength = std::sqrt(dx * dx + dy * dy);
    const double displayedLength = dimension.kind == DimensionKind::Diameter ? currentLength * 2.0 : currentLength;
    const double nextDisplayedLength = fieldId == "dimension_length" ? value : displayedLength;
    const double nextStoredLength = dimension.kind == DimensionKind::Diameter ? nextDisplayedLength / 2.0 : nextDisplayedLength;
    if (!std::isfinite(nextStoredLength) || nextStoredLength < 0.0) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension physical length is invalid");
    }

    DimensionGeometry next = dimension;
    if (dimension.kind == DimensionKind::Width) {
        const double sign = dimension.b.x < dimension.a.x ? -1.0 : 1.0;
        next.b = {(ax + sign * nextStoredLength) / width, dimension.a.y};
    } else if (dimension.kind == DimensionKind::Height) {
        const double sign = dimension.b.y < dimension.a.y ? -1.0 : 1.0;
        next.b = {dimension.a.x, (ay + sign * nextStoredLength) / height};
    } else {
        const double angle = fieldId == "dimension_angle_deg"
            ? value * kPi / 180.0
            : std::atan2(dy, dx);
        if (!std::isfinite(angle)) {
            return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension physical angle is invalid");
        }
        next.b = {
            (ax + std::cos(angle) * nextStoredLength) / width,
            (ay + std::sin(angle) * nextStoredLength) / height,
        };
    }
    return commandFromGeometry(object, DraftingGeometry{next});
}

DraftingPhysicalGeometryEditPlan planPhysicalDimensionOffsetEdit(
    const DraftingObject &object,
    const DimensionGeometry &dimension,
    double value,
    double width,
    double height)
{
    const double dx = dimension.b.x - dimension.a.x;
    const double dy = dimension.b.y - dimension.a.y;
    const double normalizedLength = std::sqrt(dx * dx + dy * dy);
    if (!std::isfinite(normalizedLength) || normalizedLength <= kMinLength) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension offset requires a non-zero dimension");
    }
    const double nx = -dy / normalizedLength;
    const double ny = dx / normalizedLength;
    const double physicalPerNormalizedOffset = std::sqrt((nx * width) * (nx * width) + (ny * height) * (ny * height));
    if (!std::isfinite(physicalPerNormalizedOffset) || physicalPerNormalizedOffset <= kMinLength) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension offset scale is invalid");
    }
    return commandFromNumericEdit(object, "offset", value / physicalPerNormalizedOffset);
}

} // namespace

DraftingPhysicalGeometryEditPlan DraftingPhysicalGeometryEditPlan::accepted(DraftingCommand command)
{
    DraftingPhysicalGeometryEditPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.command = std::move(command);
    return result;
}

DraftingPhysicalGeometryEditPlan DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingPhysicalGeometryEditPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingPhysicalGeometryEditPlan planPhysicalGeometryEdit(
    const DraftingObject &object,
    const DraftingGridProjection &grid,
    const std::string &fieldId,
    double value)
{
    if (fieldId.empty()) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical edit field id is empty");
    }
    if (!std::isfinite(value)) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical edit value must be finite");
    }
    if (!validPhysicalGrid(grid)) {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical edit grid dimensions are invalid");
    }

    const double width = grid.settings.width;
    const double height = grid.settings.height;

    if (fieldId == "line_length" || fieldId == "line_angle_deg") {
        const auto *line = std::get_if<LineGeometry>(&object.geometry);
        if (object.kind != DraftingShapeKind::Line || line == nullptr) {
            return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical line edit requires a line object");
        }
        return planPhysicalLineEdit(object, *line, fieldId, value, width, height);
    }

    if (fieldId == "dimension_length" || fieldId == "dimension_angle_deg") {
        const auto *dimension = std::get_if<DimensionGeometry>(&object.geometry);
        if (object.kind != DraftingShapeKind::Dimension || dimension == nullptr) {
            return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical dimension edit requires a dimension object");
        }
        return planPhysicalDimensionEdit(object, *dimension, fieldId, value, width, height);
    }

    if (fieldId == "offset") {
        const auto *dimension = std::get_if<DimensionGeometry>(&object.geometry);
        if (object.kind != DraftingShapeKind::Dimension || dimension == nullptr) {
            return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical offset edit requires a dimension object");
        }
        return planPhysicalDimensionOffsetEdit(object, *dimension, value, width, height);
    }

    double normalizedValue = value;
    if (fieldId == "position") {
        const auto *guide = std::get_if<GuideGeometry>(&object.geometry);
        if (object.kind != DraftingShapeKind::Guide || guide == nullptr) {
            return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical guide edit requires a guide object");
        }
        normalizedValue = guide->orientation == GuideOrientation::Horizontal ? value / height : value / width;
    } else if (fieldId == "x"
        || fieldId == "cx"
        || fieldId == "x1"
        || fieldId == "x2"
        || fieldId == "width"
        || fieldId == "radius"
        || fieldId == "diameter") {
        normalizedValue = value / width;
    } else if (fieldId == "y"
        || fieldId == "cy"
        || fieldId == "y1"
        || fieldId == "y2"
        || fieldId == "height") {
        normalizedValue = value / height;
    } else if (fieldId == "rotation_deg") {
        normalizedValue = value;
    } else {
        return DraftingPhysicalGeometryEditPlan::rejected(DraftingResultCode::InvalidGeometry, "physical field does not apply to object geometry");
    }

    return commandFromNumericEdit(object, fieldId, normalizedValue);
}

} // namespace edi::drafting
