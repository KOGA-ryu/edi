#include "drafting/DraftingMirror.h"

#include "drafting/DraftingGeometry.h"

#include <utility>

namespace edi::drafting {
namespace {

Point2D mirrorPoint(Point2D point, Bounds2D bounds, DraftingMirrorAxis axis)
{
    if (axis == DraftingMirrorAxis::Horizontal) {
        return {point.x, bounds.y + bounds.height - (point.y - bounds.y)};
    }
    return {bounds.x + bounds.width - (point.x - bounds.x), point.y};
}

DraftingGeometry mirrorGeometry(const DraftingGeometry &geometry, Bounds2D bounds, DraftingMirrorAxis axis)
{
    return std::visit([&](auto typedGeometry) -> DraftingGeometry {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            typedGeometry.point = mirrorPoint(typedGeometry.point, bounds, axis);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const Point2D mirroredOrigin = mirrorPoint(typedGeometry.origin, bounds, axis);
            if (axis == DraftingMirrorAxis::Horizontal) {
                typedGeometry.origin.y = mirroredOrigin.y - typedGeometry.height;
            } else {
                typedGeometry.origin.x = mirroredOrigin.x - typedGeometry.width;
            }
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            typedGeometry.center = mirrorPoint(typedGeometry.center, bounds, axis);
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
            typedGeometry.offset = -typedGeometry.offset;
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry> || std::is_same_v<Geometry, PolylineGeometry>) {
            return DraftingGeometry{typedGeometry};
        } else {
            return DraftingGeometry{typedGeometry};
        }
        return typedGeometry;
    }, geometry);
}

bool supportsMirror(DraftingShapeKind kind)
{
    return kind == DraftingShapeKind::Point
        || kind == DraftingShapeKind::Line
        || kind == DraftingShapeKind::Rectangle
        || kind == DraftingShapeKind::Circle
        || kind == DraftingShapeKind::ConstructionLine
        || kind == DraftingShapeKind::Dimension;
}

} // namespace

DraftingMirrorResult DraftingMirrorResult::accepted(DraftingObject object)
{
    DraftingMirrorResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.object = std::move(object);
    return result;
}

DraftingMirrorResult DraftingMirrorResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingMirrorResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingMirrorAxis draftingMirrorAxisFromId(const std::string &axisId)
{
    return axisId == "vertical" ? DraftingMirrorAxis::Vertical : DraftingMirrorAxis::Horizontal;
}

DraftingMirrorResult mirrorDraftingObject(
    const DraftingObject &source,
    DraftingObjectId newObjectId,
    DraftingMirrorAxis axis)
{
    if (!kindMatchesGeometry(source.kind, source.geometry)) {
        return DraftingMirrorResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    if (!supportsMirror(source.kind)) {
        return DraftingMirrorResult::rejected(DraftingResultCode::InvalidSelectionTarget, "shape cannot be mirrored yet");
    }

    DraftingObject mirrored = source;
    mirrored.id = std::move(newObjectId);
    mirrored.geometry = mirrorGeometry(source.geometry, computeBounds(source.geometry), axis);
    mirrored.metadata.toolProvenance = "mirror";
    mirrored.metadata.source = source.id;

    const auto validation = validateDraftingObjectShape(mirrored);
    if (!validation.ok) {
        return DraftingMirrorResult::rejected(validation.code, validation.message);
    }
    mirrored.bounds = computeBounds(mirrored.geometry);
    return DraftingMirrorResult::accepted(std::move(mirrored));
}

} // namespace edi::drafting
