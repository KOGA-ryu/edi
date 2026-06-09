#include "drafting/DraftingQuickMeasure.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingPhysicalGeometry.h"
#include "drafting/DraftingSnap.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <utility>
#include <variant>

namespace edi::drafting {

namespace {

double angleDegrees(Point2D a, Point2D b, double widthScale = 1.0, double heightScale = 1.0)
{
    constexpr double pi = 3.14159265358979323846;
    return std::atan2((b.y - a.y) * heightScale, (b.x - a.x) * widthScale) * 180.0 / pi;
}

double physicalDimensionOffset(const DimensionGeometry &dimension, const DraftingGridProjection &grid)
{
    const double normalizedLength = distance(dimension.a, dimension.b);
    if (normalizedLength <= 0.000001) {
        return 0.0;
    }
    const double nx = -(dimension.b.y - dimension.a.y) / normalizedLength * dimension.offset;
    const double ny = (dimension.b.x - dimension.a.x) / normalizedLength * dimension.offset;
    const double dx = physicalWidth(nx, grid);
    const double dy = physicalHeight(ny, grid);
    const double magnitude = std::sqrt(dx * dx + dy * dy);
    return dimension.offset < 0.0 ? -magnitude : magnitude;
}

std::string compactNumber(double value)
{
    std::ostringstream stream;
    stream << std::setprecision(6) << std::defaultfloat << value;
    return stream.str();
}

void attachGridUnit(DraftingQuickMeasureResult &result, const DraftingGridProjection &grid)
{
    result.unit = grid.settings.unit;
    result.unitName = draftingGridUnitName(grid.settings.unit);
    result.unitLabel = draftingGridUnitLabel(grid.settings.unit);
}

DraftingQuickMeasureResult acceptedBase(
    const DraftingObject &object,
    const DraftingHitTestResult &hit,
    const DraftingGridProjection &grid)
{
    DraftingQuickMeasureResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.objectId = object.id;
    result.objectKind = object.kind;
    result.hitDistance = hit.distance;
    attachGridUnit(result, grid);
    return result;
}

} // namespace

DraftingQuickMeasureResult DraftingQuickMeasureResult::rejected(
    DraftingQuickMeasureKind kind,
    DraftingResultCode code,
    std::string message,
    const DraftingGridProjection &grid)
{
    DraftingQuickMeasureResult result;
    result.ok = false;
    result.kind = kind;
    result.code = code;
    result.message = std::move(message);
    attachGridUnit(result, grid);
    return result;
}

const char *draftingQuickMeasureKindName(DraftingQuickMeasureKind kind)
{
    switch (kind) {
    case DraftingQuickMeasureKind::None:
        return "none";
    case DraftingQuickMeasureKind::Point:
        return "point";
    case DraftingQuickMeasureKind::Line:
        return "line";
    case DraftingQuickMeasureKind::Rectangle:
        return "rectangle";
    case DraftingQuickMeasureKind::Circle:
        return "circle";
    case DraftingQuickMeasureKind::Dimension:
        return "dimension";
    case DraftingQuickMeasureKind::Unsupported:
        return "unsupported";
    }
    return "none";
}

DraftingQuickMeasureResult quickMeasureAt(
    const DraftingDocument &document,
    Point2D rawPoint,
    const DraftingGridProjection &grid)
{
    const Point2D raw = normalizeDraftingPoint(rawPoint);
    const DraftingHitTestResult hit = hitTestDocument(document, raw);
    if (!hit.ok) {
        return DraftingQuickMeasureResult::rejected(
            DraftingQuickMeasureKind::None,
            DraftingResultCode::ObjectNotFound,
            "no measurable target",
            grid);
    }

    const DraftingObject *object = findObject(document, hit.objectId);
    if (object == nullptr || !kindMatchesGeometry(object->kind, object->geometry)) {
        return DraftingQuickMeasureResult::rejected(
            DraftingQuickMeasureKind::None,
            DraftingResultCode::ObjectNotFound,
            "measurable target is unavailable",
            grid);
    }

    DraftingQuickMeasureResult result = acceptedBase(*object, hit, grid);
    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            result.kind = DraftingQuickMeasureKind::Point;
            result.x = geometry.point.x;
            result.y = geometry.point.y;
            result.physicalX = physicalX(geometry.point, grid);
            result.physicalY = physicalY(geometry.point, grid);
            result.label = "point " + compactNumber(result.physicalX) + "," + compactNumber(result.physicalY) + " "
                + result.unitLabel;
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            result.kind = DraftingQuickMeasureKind::Line;
            result.length = distance(geometry.a, geometry.b);
            result.angleDeg = angleDegrees(geometry.a, geometry.b);
            result.physicalLength = physicalDistance(geometry.a, geometry.b, grid);
            result.physicalAngleDeg = physicalAngleDegrees(geometry.a, geometry.b, grid);
            result.label = "line " + compactNumber(result.physicalLength) + " " + result.unitLabel + " @ "
                + compactNumber(result.physicalAngleDeg) + " deg";
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            result.kind = DraftingQuickMeasureKind::Rectangle;
            result.width = geometry.width;
            result.height = geometry.height;
            result.area = geometry.width * geometry.height;
            result.physicalWidth = physicalWidth(geometry.width, grid);
            result.physicalHeight = physicalHeight(geometry.height, grid);
            result.physicalArea = result.physicalWidth * result.physicalHeight;
            result.label = "rect " + compactNumber(result.physicalWidth) + " x "
                + compactNumber(result.physicalHeight) + " " + result.unitLabel + ", area "
                + compactNumber(result.physicalArea);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            result.kind = DraftingQuickMeasureKind::Circle;
            result.radius = geometry.radius;
            result.diameter = geometry.radius * 2.0;
            result.physicalRadius = physicalWidth(geometry.radius, grid);
            result.physicalDiameter = result.physicalRadius * 2.0;
            result.physicalRadiusY = physicalHeight(geometry.radius, grid);
            result.physicalDiameterY = result.physicalRadiusY * 2.0;
            result.label = "circle r " + compactNumber(result.physicalRadius) + " " + result.unitLabel + ", d "
                + compactNumber(result.physicalDiameter);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            const double displayedMultiplier = geometry.kind == DimensionKind::Diameter ? 2.0 : 1.0;
            result.kind = DraftingQuickMeasureKind::Dimension;
            result.dimensionKind = geometry.kind;
            result.length = distance(geometry.a, geometry.b);
            result.displayedLength = result.length * displayedMultiplier;
            result.angleDeg = angleDegrees(geometry.a, geometry.b);
            result.physicalLength = physicalDistance(geometry.a, geometry.b, grid);
            result.physicalDisplayedLength = result.physicalLength * displayedMultiplier;
            result.physicalAngleDeg = physicalAngleDegrees(geometry.a, geometry.b, grid);
            result.offset = geometry.offset;
            result.physicalOffset = physicalDimensionOffset(geometry, grid);
            result.label = std::string("dimension ") + dimensionKindName(geometry.kind) + " "
                + compactNumber(result.physicalDisplayedLength) + " " + result.unitLabel + " @ "
                + compactNumber(result.physicalAngleDeg) + " deg";
        } else {
            result = DraftingQuickMeasureResult::rejected(
                DraftingQuickMeasureKind::Unsupported,
                DraftingResultCode::InvalidGeometry,
                "target has no quick measurement yet",
                grid);
            result.objectId = object->id;
            result.objectKind = object->kind;
            result.hitDistance = hit.distance;
        }
    }, object->geometry);

    return result;
}

} // namespace edi::drafting
