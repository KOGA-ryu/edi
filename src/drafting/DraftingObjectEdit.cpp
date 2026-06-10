#include "drafting/DraftingObjectEdit.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingPhysicalGeometry.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <type_traits>
#include <utility>

namespace edi::drafting {
namespace {

constexpr double pi = 3.14159265358979323846;

double normalizeDegrees(double degrees)
{
    return std::fmod(std::fmod(degrees, 360.0) + 360.0, 360.0);
}

Point2D rectangleCenter(const RectangleGeometry &rect)
{
    return {
        rect.origin.x + rect.width / 2.0,
        rect.origin.y + rect.height / 2.0,
    };
}

Point2D rotateAround(Point2D point, Point2D center, double degrees)
{
    const double angle = degrees * pi / 180.0;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    const double dx = point.x - center.x;
    const double dy = point.y - center.y;
    return {
        center.x + dx * cosA - dy * sinA,
        center.y + dx * sinA + dy * cosA,
    };
}

Point2D unrotateRectanglePoint(const RectangleGeometry &rect, Point2D point)
{
    return rotateAround(point, rectangleCenter(rect), -rect.rotationDeg);
}

std::vector<DraftingHandleDescriptor> rectangleHandles(const RectangleGeometry &rect)
{
    const Point2D center = rectangleCenter(rect);
    std::vector<DraftingHandleDescriptor> handles = {
        {"rect_nw", "corner", rect.origin},
        {"rect_ne", "corner", {rect.origin.x + rect.width, rect.origin.y}},
        {"rect_sw", "corner", {rect.origin.x, rect.origin.y + rect.height}},
        {"rect_se", "corner", {rect.origin.x + rect.width, rect.origin.y + rect.height}},
    };
    for (DraftingHandleDescriptor &handle : handles) {
        handle.point = rotateAround(handle.point, center, rect.rotationDeg);
    }

    const Point2D topMidpoint = rotateAround({rect.origin.x + rect.width / 2.0, rect.origin.y}, center, rect.rotationDeg);
    const double dx = topMidpoint.x - center.x;
    const double dy = topMidpoint.y - center.y;
    const double length = std::max(0.000001, std::sqrt(dx * dx + dy * dy));
    handles.push_back({
        "rect_rotate",
        "rotate",
        {topMidpoint.x + dx / length * 0.05, topMidpoint.y + dy / length * 0.05},
    });
    return handles;
}

DraftingObjectEditResult validatedEditResult(const DraftingObject &object, DraftingGeometry geometry)
{
    DraftingObject candidate = object;
    candidate.geometry = std::move(geometry);
    const auto validation = validateDraftingObjectShape(candidate);
    if (!validation.ok) {
        return DraftingObjectEditResult::rejected(validation.code, validation.message);
    }
    return DraftingObjectEditResult::accepted(candidate.geometry, computeBounds(candidate.geometry));
}

DraftingObjectEditResult applyRectangleEdit(const DraftingObject &object, const RectangleGeometry &rect, const DraftingObjectEdit &edit)
{
    RectangleGeometry next = rect;
    if (edit.kind == DraftingObjectEditKind::RotateRectangle) {
        if (!std::isfinite(edit.value)) {
            return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "rectangle rotation must be finite");
        }
        next.rotationDeg = normalizeDegrees(edit.value);
        return validatedEditResult(object, next);
    }

    if (edit.kind != DraftingObjectEditKind::MoveRectangleCorner) {
        return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "edit does not apply to rectangle geometry");
    }

    if (!isFinite(edit.point)) {
        return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "rectangle edit point must be finite");
    }

    const double left = rect.origin.x;
    const double top = rect.origin.y;
    const double right = rect.origin.x + rect.width;
    const double bottom = rect.origin.y + rect.height;
    const Point2D localPoint = unrotateRectanglePoint(rect, edit.point);

    double fixedX = left;
    double fixedY = top;
    if (edit.handleId == "rect_nw") {
        fixedX = right;
        fixedY = bottom;
    } else if (edit.handleId == "rect_ne") {
        fixedX = left;
        fixedY = bottom;
    } else if (edit.handleId == "rect_sw") {
        fixedX = right;
        fixedY = top;
    } else if (edit.handleId == "rect_se") {
        fixedX = left;
        fixedY = top;
    } else {
        return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "unknown rectangle handle");
    }

    next.origin = {std::min(fixedX, localPoint.x), std::min(fixedY, localPoint.y)};
    next.width = std::abs(fixedX - localPoint.x);
    next.height = std::abs(fixedY - localPoint.y);
    return validatedEditResult(object, next);
}

Point2D dimensionMidpoint(const DimensionGeometry &dimension)
{
    return {
        (dimension.a.x + dimension.b.x) / 2.0,
        (dimension.a.y + dimension.b.y) / 2.0,
    };
}

Point2D dimensionOffsetPoint(const DimensionGeometry &dimension)
{
    const Point2D midpoint = dimensionMidpoint(dimension);
    const Point2D offset = dimensionOffsetVector(dimension);
    return {
        midpoint.x + offset.x,
        midpoint.y + offset.y,
    };
}

double dimensionOffsetFromPoint(const DimensionGeometry &dimension, Point2D point)
{
    const double dx = dimension.b.x - dimension.a.x;
    const double dy = dimension.b.y - dimension.a.y;
    const double length = std::max(0.000001, std::sqrt(dx * dx + dy * dy));
    const Point2D midpoint = dimensionMidpoint(dimension);
    const double nx = -dy / length;
    const double ny = dx / length;
    return (point.x - midpoint.x) * nx + (point.y - midpoint.y) * ny;
}

DraftingObjectEditResult applyDimensionEdit(const DraftingObject &object, const DimensionGeometry &dimension, const DraftingObjectEdit &edit)
{
    if (!isFinite(edit.point)) {
        return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "dimension edit point must be finite");
    }

    DimensionGeometry next = dimension;
    if (edit.kind == DraftingObjectEditKind::MoveDimensionStart) {
        next.a = edit.point;
        if (next.kind == DimensionKind::Width) {
            next.b.y = next.a.y;
        } else if (next.kind == DimensionKind::Height) {
            next.b.x = next.a.x;
        }
    } else if (edit.kind == DraftingObjectEditKind::MoveDimensionEnd) {
        if (next.kind == DimensionKind::Width) {
            next.b = {edit.point.x, next.a.y};
        } else if (next.kind == DimensionKind::Height) {
            next.b = {next.a.x, edit.point.y};
        } else {
            next.b = edit.point;
        }
    } else if (edit.kind == DraftingObjectEditKind::SetDimensionOffset) {
        next.offset = dimensionOffsetFromPoint(next, edit.point);
    } else {
        return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "edit does not apply to dimension geometry");
    }
    return validatedEditResult(object, next);
}

} // namespace

DraftingObjectEditResult DraftingObjectEditResult::accepted(DraftingGeometry geometry, Bounds2D bounds)
{
    DraftingObjectEditResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = std::move(geometry);
    result.bounds = bounds;
    return result;
}

DraftingObjectEditResult DraftingObjectEditResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingObjectEditResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingHandleEditPlan DraftingHandleEditPlan::accepted(DraftingObjectEdit edit)
{
    DraftingHandleEditPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.edit = std::move(edit);
    return result;
}

DraftingHandleEditPlan DraftingHandleEditPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingHandleEditPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::vector<DraftingHandleDescriptor> draftingHandlesForObject(const DraftingObject &object)
{
    if (!kindMatchesGeometry(object.kind, object.geometry)) {
        return {};
    }

    return std::visit([](const auto &geometry) -> std::vector<DraftingHandleDescriptor> {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            return {{"point_position", "point", geometry.point}};
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            return {
                {"line_start", "endpoint", geometry.a},
                {"line_end", "endpoint", geometry.b},
            };
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            return rectangleHandles(geometry);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            return {
                {"circle_center", "center", geometry.center},
                {"circle_radius", "radius", {geometry.center.x + geometry.radius, geometry.center.y}},
            };
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            return {
                {"arc_center", "center", geometry.center},
                {"arc_radius", "radius", arcPointAtAngle(geometry.center, geometry.radius, arcMidAngleDeg(geometry))},
                {"arc_start", "endpoint", arcPointAtAngle(geometry.center, geometry.radius, geometry.startAngleDeg)},
                {"arc_end", "endpoint", arcPointAtAngle(geometry.center, geometry.radius, geometry.endAngleDeg)},
            };
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            DraftingHandleDescriptor offset {
                "dimension_offset",
                "offset",
                dimensionOffsetPoint(geometry),
            };
            offset.readOnly = false;
            offset.hasAnchor = true;
            offset.anchor = dimensionMidpoint(geometry);
            return {
                {"dimension_start", "endpoint", geometry.a},
                {"dimension_end", "endpoint", geometry.b},
                offset,
            };
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry> || std::is_same_v<Geometry, PolylineGeometry>) {
            std::vector<DraftingHandleDescriptor> handles;
            handles.reserve(geometry.vertices.size());
            for (std::size_t index = 0; index < geometry.vertices.size(); ++index) {
                DraftingHandleDescriptor handle {
                    "vertex_" + std::to_string(index),
                    "vertex",
                    geometry.vertices[index],
                };
                handle.readOnly = true;
                handles.push_back(handle);
            }
            return handles;
        } else {
            return {};
        }
    }, object.geometry);
}

DraftingHandleDescriptor draftingHandleById(const DraftingObject &object, const std::string &handleId)
{
    for (const DraftingHandleDescriptor &handle : draftingHandlesForObject(object)) {
        if (handle.id == handleId) {
            return handle;
        }
    }
    return {};
}

DraftingHandleEditPlan handleEditPlan(const DraftingObject &object, const std::string &handleId, Point2D point)
{
    const DraftingHandleDescriptor handle = draftingHandleById(object, handleId);
    if (handle.id.empty()) {
        return DraftingHandleEditPlan::rejected(DraftingResultCode::InvalidSelectionTarget, "handle does not exist for object");
    }
    if (handle.readOnly) {
        return DraftingHandleEditPlan::rejected(DraftingResultCode::InvalidSelectionTarget, "handle is read-only");
    }
    if (!isFinite(point)) {
        return DraftingHandleEditPlan::rejected(DraftingResultCode::InvalidGeometry, "handle edit point must be finite");
    }

    if (object.kind == DraftingShapeKind::Point && handleId == "point_position") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MovePoint, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Line && handleId == "line_start") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveLineStart, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Line && handleId == "line_end") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveLineEnd, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Circle && handleId == "circle_center") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveCircleCenter, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Circle && handleId == "circle_radius") {
        const auto *circle = std::get_if<CircleGeometry>(&object.geometry);
        if (circle == nullptr) {
            return DraftingHandleEditPlan::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
        }
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::SetCircleRadius, handleId, point, distance(circle->center, point)});
    }
    if (object.kind == DraftingShapeKind::Arc) {
        const auto *arc = std::get_if<ArcGeometry>(&object.geometry);
        if (arc == nullptr) {
            return DraftingHandleEditPlan::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
        }
        if (handleId == "arc_center") {
            return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveArcCenter, handleId, point});
        }
        if (handleId == "arc_radius") {
            return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::SetArcRadius, handleId, point, distance(arc->center, point)});
        }
        const double angle = std::atan2(point.y - arc->center.y, point.x - arc->center.x) * 180.0 / pi;
        if (handleId == "arc_start") {
            return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::SetArcStartAngle, handleId, point, angle});
        }
        if (handleId == "arc_end") {
            return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::SetArcEndAngle, handleId, point, angle});
        }
    }
    if (object.kind == DraftingShapeKind::Rectangle && handleId == "rect_rotate") {
        const auto *rect = std::get_if<RectangleGeometry>(&object.geometry);
        if (rect == nullptr) {
            return DraftingHandleEditPlan::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
        }
        const Point2D center = rectangleCenter(*rect);
        const double degrees = std::atan2(point.y - center.y, point.x - center.x) * 180.0 / pi + 90.0;
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::RotateRectangle, handleId, point, normalizeDegrees(degrees)});
    }
    if (object.kind == DraftingShapeKind::Rectangle && handle.role == "corner") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveRectangleCorner, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Dimension && handleId == "dimension_start") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveDimensionStart, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Dimension && handleId == "dimension_end") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::MoveDimensionEnd, handleId, point});
    }
    if (object.kind == DraftingShapeKind::Dimension && handleId == "dimension_offset") {
        return DraftingHandleEditPlan::accepted({DraftingObjectEditKind::SetDimensionOffset, handleId, point});
    }

    return DraftingHandleEditPlan::rejected(DraftingResultCode::InvalidSelectionTarget, "handle cannot produce an edit for object");
}

DraftingObjectEditResult applyObjectEdit(const DraftingObject &object, const DraftingObjectEdit &edit)
{
    if (!kindMatchesGeometry(object.kind, object.geometry)) {
        return DraftingObjectEditResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }

    return std::visit([&](auto geometry) -> DraftingObjectEditResult {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            if (edit.kind != DraftingObjectEditKind::MovePoint) {
                return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "edit does not apply to point geometry");
            }
            geometry.point = edit.point;
            return validatedEditResult(object, geometry);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            if (edit.kind == DraftingObjectEditKind::MoveLineStart) {
                geometry.a = edit.point;
            } else if (edit.kind == DraftingObjectEditKind::MoveLineEnd) {
                geometry.b = edit.point;
            } else {
                return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "edit does not apply to line geometry");
            }
            return validatedEditResult(object, geometry);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            return applyRectangleEdit(object, geometry, edit);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            if (edit.kind == DraftingObjectEditKind::MoveCircleCenter) {
                geometry.center = edit.point;
            } else if (edit.kind == DraftingObjectEditKind::SetCircleRadius) {
                geometry.radius = edit.value;
            } else {
                return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "edit does not apply to circle geometry");
            }
            return validatedEditResult(object, geometry);
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            if (edit.kind == DraftingObjectEditKind::MoveArcCenter) {
                geometry.center = edit.point;
            } else if (edit.kind == DraftingObjectEditKind::SetArcRadius) {
                geometry.radius = edit.value;
            } else if (edit.kind == DraftingObjectEditKind::SetArcStartAngle) {
                geometry.startAngleDeg = edit.value;
            } else if (edit.kind == DraftingObjectEditKind::SetArcEndAngle) {
                geometry.endAngleDeg = edit.value;
            } else {
                return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidGeometry, "edit does not apply to arc geometry");
            }
            return validatedEditResult(object, geometry);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            return applyDimensionEdit(object, geometry, edit);
        } else {
            return DraftingObjectEditResult::rejected(DraftingResultCode::InvalidSelectionTarget, "shape has no editable handles yet");
        }
    }, object.geometry);
}

} // namespace edi::drafting
