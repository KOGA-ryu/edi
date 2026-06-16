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
    // Every one of the 14 kinds gets an EXPLICIT arm: the seven that transform
    // (which must match supportsMirror's set below by hand — see the comment
    // there) and the seven that pass through unchanged. The terminal
    // always_false_v guard then forces a future 15th kind to choose, at COMPILE
    // time, whether it mirrors — the old `else { return unchanged; }` would have
    // silently passed a new kind through. Behavior is unchanged: the transforming
    // arms are identical to before, and the pass-through arms reproduce the old
    // `else` (and the old Polygon/Polyline arm) exactly.
    return std::visit([&](auto typedGeometry) -> DraftingGeometry {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            typedGeometry.point = mirrorPoint(typedGeometry.point, bounds, axis);
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const Point2D mirroredOrigin = mirrorPoint(typedGeometry.origin, bounds, axis);
            if (axis == DraftingMirrorAxis::Horizontal) {
                typedGeometry.origin.y = mirroredOrigin.y - typedGeometry.height;
            } else {
                typedGeometry.origin.x = mirroredOrigin.x - typedGeometry.width;
            }
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            typedGeometry.center = mirrorPoint(typedGeometry.center, bounds, axis);
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // A wall is a thick line: mirror both centerline endpoints; the band
            // thickness is a scalar width, unaffected by the reflection.
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            typedGeometry.a = mirrorPoint(typedGeometry.a, bounds, axis);
            typedGeometry.b = mirrorPoint(typedGeometry.b, bounds, axis);
            typedGeometry.offset = -typedGeometry.offset;
            return typedGeometry;
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>
                || std::is_same_v<Geometry, EllipseGeometry>
                || std::is_same_v<Geometry, PolygonGeometry>
                || std::is_same_v<Geometry, PolylineGeometry>
                || std::is_same_v<Geometry, GuideGeometry>
                || std::is_same_v<Geometry, TextAnnotationGeometry>
                || std::is_same_v<Geometry, SplineGeometry>) {
            // The seven kinds ABSENT from supportsMirror: mirrorDraftingObject
            // rejects them before this visit runs, so returning them unchanged is
            // the explicit form of the old catch-all `else`. Named individually so
            // they stay in lockstep with supportsMirror by inspection.
            return DraftingGeometry{typedGeometry};
        } else {
            static_assert(always_false_v<Geometry>, "mirrorGeometry: unhandled geometry kind — add an arm");
        }
    }, geometry);
}

// The mirrorable kinds. This MUST match the transforming arms of mirrorGeometry
// above (Point/Line/Wall/Rectangle/Circle/ConstructionLine/Dimension) — the two
// are kept in sync by hand. They are not unified into one source because the gate
// keys on DraftingShapeKind while the visit keys on the geometry type; the
// mirrorGeometry guard ensures a new kind can't be silently forgotten on the
// visit side, and this list must then be revisited to opt the kind in or out.
bool supportsMirror(DraftingShapeKind kind)
{
    return kind == DraftingShapeKind::Point
        || kind == DraftingShapeKind::Line
        || kind == DraftingShapeKind::Wall
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
