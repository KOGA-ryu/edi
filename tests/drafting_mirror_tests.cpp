#include "drafting/DraftingMirror.h"

#include <cassert>
#include <cmath>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    assert(draftingMirrorAxisFromId("vertical") == DraftingMirrorAxis::Vertical);
    assert(draftingMirrorAxisFromId("horizontal") == DraftingMirrorAxis::Horizontal);
    assert(draftingMirrorAxisFromId("missing") == DraftingMirrorAxis::Horizontal);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.5}});
    line.stroke.color = "#22ccff";
    auto verticalLine = mirrorDraftingObject(line, "line_mirror_v", DraftingMirrorAxis::Vertical);
    assert(verticalLine.ok);
    assert(verticalLine.object.id == "line_mirror_v");
    assert(verticalLine.object.kind == DraftingShapeKind::Line);
    assert(verticalLine.object.metadata.toolProvenance == "mirror");
    assert(verticalLine.object.metadata.source == "line_1");
    assert(verticalLine.object.stroke.color == "#22ccff");
    const auto *verticalLineGeometry = std::get_if<LineGeometry>(&verticalLine.object.geometry);
    assert(verticalLineGeometry != nullptr);
    assert(nearlyEqual(verticalLineGeometry->a.x, 1.0));
    assert(nearlyEqual(verticalLineGeometry->a.y, 0.0));
    assert(nearlyEqual(verticalLineGeometry->b.x, 0.0));
    assert(nearlyEqual(verticalLineGeometry->b.y, 0.5));

    auto horizontalLine = mirrorDraftingObject(line, "line_mirror_h", DraftingMirrorAxis::Horizontal);
    assert(horizontalLine.ok);
    const auto *horizontalLineGeometry = std::get_if<LineGeometry>(&horizontalLine.object.geometry);
    assert(horizontalLineGeometry != nullptr);
    assert(nearlyEqual(horizontalLineGeometry->a.x, 0.0));
    assert(nearlyEqual(horizontalLineGeometry->a.y, 0.5));
    assert(nearlyEqual(horizontalLineGeometry->b.x, 1.0));
    assert(nearlyEqual(horizontalLineGeometry->b.y, 0.0));

    // A wall mirrors like a line: both centerline endpoints flip across the
    // object's bounds (which include the half-thickness pad); thickness is a
    // scalar width and is preserved.
    DraftingObject wall = object("wall_1", DraftingShapeKind::Wall, WallGeometry{{0.2, 0.5}, {0.8, 0.5}, 0.1});
    auto mirroredWall = mirrorDraftingObject(wall, "wall_mirror", DraftingMirrorAxis::Vertical);
    assert(mirroredWall.ok);
    assert(mirroredWall.object.kind == DraftingShapeKind::Wall);
    const auto *wallGeometry = std::get_if<WallGeometry>(&mirroredWall.object.geometry);
    assert(wallGeometry != nullptr);
    assert(nearlyEqual(wallGeometry->a.x, 0.8) && nearlyEqual(wallGeometry->a.y, 0.5));
    assert(nearlyEqual(wallGeometry->b.x, 0.2) && nearlyEqual(wallGeometry->b.y, 0.5));
    assert(nearlyEqual(wallGeometry->thickness, 0.1));

    DraftingObject construction = object(
        "construction_1",
        DraftingShapeKind::ConstructionLine,
        ConstructionLineGeometry{{0.2, 0.0}, {0.8, 1.0}});
    auto mirroredConstruction = mirrorDraftingObject(construction, "construction_mirror", DraftingMirrorAxis::Vertical);
    assert(mirroredConstruction.ok);
    const auto *constructionGeometry = std::get_if<ConstructionLineGeometry>(&mirroredConstruction.object.geometry);
    assert(constructionGeometry != nullptr);
    assert(nearlyEqual(constructionGeometry->a.x, 0.8));
    assert(nearlyEqual(constructionGeometry->a.y, 0.0));
    assert(nearlyEqual(constructionGeometry->b.x, 0.2));
    assert(nearlyEqual(constructionGeometry->b.y, 1.0));

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.2, 0.3}, {0.8, 0.5}, 0.04});
    auto mirroredDimension = mirrorDraftingObject(dimension, "dimension_mirror", DraftingMirrorAxis::Horizontal);
    assert(mirroredDimension.ok);
    const auto *dimensionGeometry = std::get_if<DimensionGeometry>(&mirroredDimension.object.geometry);
    assert(dimensionGeometry != nullptr);
    assert(nearlyEqual(dimensionGeometry->a.x, 0.2));
    assert(nearlyEqual(dimensionGeometry->a.y, 0.5379473319));
    assert(nearlyEqual(dimensionGeometry->b.x, 0.8));
    assert(nearlyEqual(dimensionGeometry->b.y, 0.3379473319));
    assert(nearlyEqual(dimensionGeometry->offset, -0.04));

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.5, 0.5}, 0.2});
    auto mirroredCircle = mirrorDraftingObject(circle, "circle_mirror", DraftingMirrorAxis::Vertical);
    assert(mirroredCircle.ok);
    const auto *circleGeometry = std::get_if<CircleGeometry>(&mirroredCircle.object.geometry);
    assert(circleGeometry != nullptr);
    assert(nearlyEqual(circleGeometry->center.x, 0.5));
    assert(nearlyEqual(circleGeometry->center.y, 0.5));
    assert(nearlyEqual(circleGeometry->radius, 0.2));

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.2, 0.3}, 0.4, 0.2});
    auto mirroredRect = mirrorDraftingObject(rect, "rect_mirror", DraftingMirrorAxis::Horizontal);
    assert(mirroredRect.ok);
    const auto *rectGeometry = std::get_if<RectangleGeometry>(&mirroredRect.object.geometry);
    assert(rectGeometry != nullptr);
    assert(nearlyEqual(rectGeometry->origin.x, 0.2));
    assert(nearlyEqual(rectGeometry->origin.y, 0.3));
    assert(nearlyEqual(rectGeometry->rotationDeg, 0.0));

    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{0.25, 0.75}});
    auto mirroredPoint = mirrorDraftingObject(point, "point_mirror", DraftingMirrorAxis::Vertical);
    assert(mirroredPoint.ok);
    const auto *pointGeometry = std::get_if<PointGeometry>(&mirroredPoint.object.geometry);
    assert(pointGeometry != nullptr);
    assert(nearlyEqual(pointGeometry->point.x, 0.25));
    assert(nearlyEqual(pointGeometry->point.y, 0.75));

    DraftingObject polygon = object("polygon_1", DraftingShapeKind::Polygon, PolygonGeometry{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}});
    auto unsupported = mirrorDraftingObject(polygon, "polygon_mirror", DraftingMirrorAxis::Vertical);
    assert(!unsupported.ok);
    assert(unsupported.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Point;
    auto mismatch = mirrorDraftingObject(mismatched, "mismatch_mirror", DraftingMirrorAxis::Vertical);
    assert(!mismatch.ok);
    assert(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    // --- DR-11 kaleidoscope (multi-axis radial mirror) -------------------
    // A POINT reflected across 4 axes (45° spacing) through (0.5,0.5). Relative to
    // the centre the offset is (0.4,0.1); reflection across the axis at angle α is
    // the standard reflection Refl(2α), so the four copies land at the points below.
    {
        DraftingObject kPoint = object("k_point", DraftingShapeKind::Point, PointGeometry{{0.9, 0.6}});
        auto rosette = kaleidoscopeMirror(kPoint, {"kp_0", "kp_1", "kp_2", "kp_3"}, {0.5, 0.5}, 4);
        assert(rosette.ok);
        assert(rosette.objects.size() == 4);
        assert(rosette.objects[0].metadata.toolProvenance == "kaleidoscope");
        const double expected[4][2] = {
            {0.9, 0.4}, // axis 0°   (reflect across horizontal through centre)
            {0.6, 0.9}, // axis 45°  (Refl 90° swaps the relative offset)
            {0.1, 0.6}, // axis 90°  (reflect across vertical)
            {0.4, 0.1}, // axis 135° (Refl 270°)
        };
        for (std::size_t i = 0; i < 4; ++i) {
            const auto *p = std::get_if<PointGeometry>(&rosette.objects[i].geometry);
            assert(p != nullptr);
            assert(nearlyEqual(p->point.x, expected[i][0]));
            assert(nearlyEqual(p->point.y, expected[i][1]));
        }
    }
    // ORIENTATION FLIP (the correctness risk). Arc is NOT in supportsMirror, so the
    // brief's "reflect an Arc" is not reachable; the flip is pinned instead on a
    // Dimension, whose offset the canonical mirrorGeometry NEGATES. One axis at 0°
    // through the dimension's own line: endpoints stay, the offset sign flips.
    {
        DraftingObject dim = object("k_dim", DraftingShapeKind::Dimension,
                                    DimensionGeometry{DimensionKind::Distance, {0.4, 0.5}, {0.6, 0.5}, 0.04});
        auto flipped = kaleidoscopeMirror(dim, {"kd_0"}, {0.5, 0.5}, 1);
        assert(flipped.ok);
        const auto *d = std::get_if<DimensionGeometry>(&flipped.objects[0].geometry);
        assert(d != nullptr);
        assert(nearlyEqual(d->offset, -0.04)); // orientation flip: side negates
        assert(nearlyEqual(d->a.x, 0.4) && nearlyEqual(d->a.y, 0.5));
        assert(nearlyEqual(d->b.x, 0.6) && nearlyEqual(d->b.y, 0.5));
    }
    // Unsupported kind (Arc, absent from supportsMirror) rejects — exactly as the
    // canonical single-axis mirror does.
    {
        DraftingObject arc = object("k_arc", DraftingShapeKind::Arc, ArcGeometry{{0.5, 0.5}, 0.2, 0.0, 90.0});
        auto rejected = kaleidoscopeMirror(arc, {"ka_0"}, {0.5, 0.5}, 1);
        assert(!rejected.ok);
        assert(rejected.code == DraftingResultCode::InvalidSelectionTarget);
    }
    // axisCount < 1 and an id-count mismatch reject.
    {
        DraftingObject kPoint = object("k_point2", DraftingShapeKind::Point, PointGeometry{{0.7, 0.5}});
        assert(!kaleidoscopeMirror(kPoint, {}, {0.5, 0.5}, 0).ok);
        assert(!kaleidoscopeMirror(kPoint, {"only_one"}, {0.5, 0.5}, 2).ok); // size 1 != axisCount 2
    }

    return 0;
}
