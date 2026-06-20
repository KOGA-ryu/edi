#include "drafting/DraftingMirror.h"

#include "EdiAssert.h"
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
    EDI_CHECK(built.ok);
    return built.object;
}

} // namespace

int main()
{
    EDI_CHECK(draftingMirrorAxisFromId("vertical") == DraftingMirrorAxis::Vertical);
    EDI_CHECK(draftingMirrorAxisFromId("horizontal") == DraftingMirrorAxis::Horizontal);
    EDI_CHECK(draftingMirrorAxisFromId("missing") == DraftingMirrorAxis::Horizontal);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 0.5}});
    line.stroke.color = "#22ccff";
    auto verticalLine = mirrorDraftingObject(line, "line_mirror_v", DraftingMirrorAxis::Vertical);
    EDI_CHECK(verticalLine.ok);
    EDI_CHECK(verticalLine.object.id == "line_mirror_v");
    EDI_CHECK(verticalLine.object.kind == DraftingShapeKind::Line);
    EDI_CHECK(verticalLine.object.metadata.toolProvenance == "mirror");
    EDI_CHECK(verticalLine.object.metadata.source == "line_1");
    EDI_CHECK(verticalLine.object.stroke.color == "#22ccff");
    const auto *verticalLineGeometry = std::get_if<LineGeometry>(&verticalLine.object.geometry);
    EDI_CHECK(verticalLineGeometry != nullptr);
    EDI_CHECK(nearlyEqual(verticalLineGeometry->a.x, 1.0));
    EDI_CHECK(nearlyEqual(verticalLineGeometry->a.y, 0.0));
    EDI_CHECK(nearlyEqual(verticalLineGeometry->b.x, 0.0));
    EDI_CHECK(nearlyEqual(verticalLineGeometry->b.y, 0.5));

    auto horizontalLine = mirrorDraftingObject(line, "line_mirror_h", DraftingMirrorAxis::Horizontal);
    EDI_CHECK(horizontalLine.ok);
    const auto *horizontalLineGeometry = std::get_if<LineGeometry>(&horizontalLine.object.geometry);
    EDI_CHECK(horizontalLineGeometry != nullptr);
    EDI_CHECK(nearlyEqual(horizontalLineGeometry->a.x, 0.0));
    EDI_CHECK(nearlyEqual(horizontalLineGeometry->a.y, 0.5));
    EDI_CHECK(nearlyEqual(horizontalLineGeometry->b.x, 1.0));
    EDI_CHECK(nearlyEqual(horizontalLineGeometry->b.y, 0.0));

    // A wall mirrors like a line: both centerline endpoints flip across the
    // object's bounds (which include the half-thickness pad); thickness is a
    // scalar width and is preserved.
    DraftingObject wall = object("wall_1", DraftingShapeKind::Wall, WallGeometry{{0.2, 0.5}, {0.8, 0.5}, 0.1});
    auto mirroredWall = mirrorDraftingObject(wall, "wall_mirror", DraftingMirrorAxis::Vertical);
    EDI_CHECK(mirroredWall.ok);
    EDI_CHECK(mirroredWall.object.kind == DraftingShapeKind::Wall);
    const auto *wallGeometry = std::get_if<WallGeometry>(&mirroredWall.object.geometry);
    EDI_CHECK(wallGeometry != nullptr);
    EDI_CHECK(nearlyEqual(wallGeometry->a.x, 0.8) && nearlyEqual(wallGeometry->a.y, 0.5));
    EDI_CHECK(nearlyEqual(wallGeometry->b.x, 0.2) && nearlyEqual(wallGeometry->b.y, 0.5));
    EDI_CHECK(nearlyEqual(wallGeometry->thickness, 0.1));

    DraftingObject construction = object(
        "construction_1",
        DraftingShapeKind::ConstructionLine,
        ConstructionLineGeometry{{0.2, 0.0}, {0.8, 1.0}});
    auto mirroredConstruction = mirrorDraftingObject(construction, "construction_mirror", DraftingMirrorAxis::Vertical);
    EDI_CHECK(mirroredConstruction.ok);
    const auto *constructionGeometry = std::get_if<ConstructionLineGeometry>(&mirroredConstruction.object.geometry);
    EDI_CHECK(constructionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(constructionGeometry->a.x, 0.8));
    EDI_CHECK(nearlyEqual(constructionGeometry->a.y, 0.0));
    EDI_CHECK(nearlyEqual(constructionGeometry->b.x, 0.2));
    EDI_CHECK(nearlyEqual(constructionGeometry->b.y, 1.0));

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.2, 0.3}, {0.8, 0.5}, 0.04});
    auto mirroredDimension = mirrorDraftingObject(dimension, "dimension_mirror", DraftingMirrorAxis::Horizontal);
    EDI_CHECK(mirroredDimension.ok);
    const auto *dimensionGeometry = std::get_if<DimensionGeometry>(&mirroredDimension.object.geometry);
    EDI_CHECK(dimensionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(dimensionGeometry->a.x, 0.2));
    EDI_CHECK(nearlyEqual(dimensionGeometry->a.y, 0.5379473319));
    EDI_CHECK(nearlyEqual(dimensionGeometry->b.x, 0.8));
    EDI_CHECK(nearlyEqual(dimensionGeometry->b.y, 0.3379473319));
    EDI_CHECK(nearlyEqual(dimensionGeometry->offset, -0.04));

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.5, 0.5}, 0.2});
    auto mirroredCircle = mirrorDraftingObject(circle, "circle_mirror", DraftingMirrorAxis::Vertical);
    EDI_CHECK(mirroredCircle.ok);
    const auto *circleGeometry = std::get_if<CircleGeometry>(&mirroredCircle.object.geometry);
    EDI_CHECK(circleGeometry != nullptr);
    EDI_CHECK(nearlyEqual(circleGeometry->center.x, 0.5));
    EDI_CHECK(nearlyEqual(circleGeometry->center.y, 0.5));
    EDI_CHECK(nearlyEqual(circleGeometry->radius, 0.2));

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.2, 0.3}, 0.4, 0.2});
    auto mirroredRect = mirrorDraftingObject(rect, "rect_mirror", DraftingMirrorAxis::Horizontal);
    EDI_CHECK(mirroredRect.ok);
    const auto *rectGeometry = std::get_if<RectangleGeometry>(&mirroredRect.object.geometry);
    EDI_CHECK(rectGeometry != nullptr);
    EDI_CHECK(nearlyEqual(rectGeometry->origin.x, 0.2));
    EDI_CHECK(nearlyEqual(rectGeometry->origin.y, 0.3));
    EDI_CHECK(nearlyEqual(rectGeometry->rotationDeg, 0.0));

    // Reflection reverses orientation: a single-axis mirror of a ROTATED rectangle
    // negates rotationDeg (r=30° -> -30°). The axis-aligned case above still holds
    // (-0 == 0).
    DraftingObject rotatedRect = object("rrect_1", DraftingShapeKind::Rectangle,
                                        RectangleGeometry{{0.2, 0.3}, 0.4, 0.2, 30.0});
    auto mirroredRotatedRect = mirrorDraftingObject(rotatedRect, "rrect_mirror", DraftingMirrorAxis::Horizontal);
    EDI_CHECK(mirroredRotatedRect.ok);
    const auto *rotatedRectGeometry = std::get_if<RectangleGeometry>(&mirroredRotatedRect.object.geometry);
    EDI_CHECK(rotatedRectGeometry != nullptr);
    EDI_CHECK(nearlyEqual(rotatedRectGeometry->rotationDeg, -30.0));

    // DR-11 kaleidoscope across a 30° axis: an axis-aligned rect (r=0) reflects to
    // r = -0 + 2θ = 60° (the conjugation R(+θ)∘mirror∘R(−θ) nets 2θ now that the
    // canonical arm flips rotationDeg). 6 axes -> spacing 30°, so copy[1] is the 30°
    // axis. The rect is centred on the kaleidoscope centre, so its centre is fixed.
    DraftingObject centredRect = object("krect_1", DraftingShapeKind::Rectangle,
                                        RectangleGeometry{{0.45, 0.45}, 0.1, 0.1, 0.0});
    auto kRect = kaleidoscopeMirror(centredRect, {"kr_0", "kr_1", "kr_2", "kr_3", "kr_4", "kr_5"}, {0.5, 0.5}, 6);
    EDI_CHECK(kRect.ok);
    const auto *kRectGeometry = std::get_if<RectangleGeometry>(&kRect.objects[1].geometry);
    EDI_CHECK(kRectGeometry != nullptr);
    EDI_CHECK(nearlyEqual(kRectGeometry->rotationDeg, 60.0));
    // Centre is invariant (the rect sits on the centre, on every axis through it).
    EDI_CHECK(nearlyEqual(kRectGeometry->origin.x + kRectGeometry->width / 2.0, 0.5));
    EDI_CHECK(nearlyEqual(kRectGeometry->origin.y + kRectGeometry->height / 2.0, 0.5));

    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{0.25, 0.75}});
    auto mirroredPoint = mirrorDraftingObject(point, "point_mirror", DraftingMirrorAxis::Vertical);
    EDI_CHECK(mirroredPoint.ok);
    const auto *pointGeometry = std::get_if<PointGeometry>(&mirroredPoint.object.geometry);
    EDI_CHECK(pointGeometry != nullptr);
    EDI_CHECK(nearlyEqual(pointGeometry->point.x, 0.25));
    EDI_CHECK(nearlyEqual(pointGeometry->point.y, 0.75));

    DraftingObject polygon = object("polygon_1", DraftingShapeKind::Polygon, PolygonGeometry{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}});
    auto unsupported = mirrorDraftingObject(polygon, "polygon_mirror", DraftingMirrorAxis::Vertical);
    EDI_CHECK(!unsupported.ok);
    EDI_CHECK(unsupported.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Point;
    auto mismatch = mirrorDraftingObject(mismatched, "mismatch_mirror", DraftingMirrorAxis::Vertical);
    EDI_CHECK(!mismatch.ok);
    EDI_CHECK(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    // --- DR-11 kaleidoscope (multi-axis radial mirror) -------------------
    // A POINT reflected across 4 axes (45° spacing) through (0.5,0.5). Relative to
    // the centre the offset is (0.4,0.1); reflection across the axis at angle α is
    // the standard reflection Refl(2α), so the four copies land at the points below.
    {
        DraftingObject kPoint = object("k_point", DraftingShapeKind::Point, PointGeometry{{0.9, 0.6}});
        auto rosette = kaleidoscopeMirror(kPoint, {"kp_0", "kp_1", "kp_2", "kp_3"}, {0.5, 0.5}, 4);
        EDI_CHECK(rosette.ok);
        EDI_CHECK(rosette.objects.size() == 4);
        EDI_CHECK(rosette.objects[0].metadata.toolProvenance == "kaleidoscope");
        const double expected[4][2] = {
            {0.9, 0.4}, // axis 0°   (reflect across horizontal through centre)
            {0.6, 0.9}, // axis 45°  (Refl 90° swaps the relative offset)
            {0.1, 0.6}, // axis 90°  (reflect across vertical)
            {0.4, 0.1}, // axis 135° (Refl 270°)
        };
        for (std::size_t i = 0; i < 4; ++i) {
            const auto *p = std::get_if<PointGeometry>(&rosette.objects[i].geometry);
            EDI_CHECK(p != nullptr);
            EDI_CHECK(nearlyEqual(p->point.x, expected[i][0]));
            EDI_CHECK(nearlyEqual(p->point.y, expected[i][1]));
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
        EDI_CHECK(flipped.ok);
        const auto *d = std::get_if<DimensionGeometry>(&flipped.objects[0].geometry);
        EDI_CHECK(d != nullptr);
        EDI_CHECK(nearlyEqual(d->offset, -0.04)); // orientation flip: side negates
        EDI_CHECK(nearlyEqual(d->a.x, 0.4) && nearlyEqual(d->a.y, 0.5));
        EDI_CHECK(nearlyEqual(d->b.x, 0.6) && nearlyEqual(d->b.y, 0.5));
    }
    // Unsupported kind (Arc, absent from supportsMirror) rejects — exactly as the
    // canonical single-axis mirror does.
    {
        DraftingObject arc = object("k_arc", DraftingShapeKind::Arc, ArcGeometry{{0.5, 0.5}, 0.2, 0.0, 90.0});
        auto rejected = kaleidoscopeMirror(arc, {"ka_0"}, {0.5, 0.5}, 1);
        EDI_CHECK(!rejected.ok);
        EDI_CHECK(rejected.code == DraftingResultCode::InvalidSelectionTarget);
    }
    // axisCount < 1 and an id-count mismatch reject.
    {
        DraftingObject kPoint = object("k_point2", DraftingShapeKind::Point, PointGeometry{{0.7, 0.5}});
        EDI_CHECK(!kaleidoscopeMirror(kPoint, {}, {0.5, 0.5}, 0).ok);
        EDI_CHECK(!kaleidoscopeMirror(kPoint, {"only_one"}, {0.5, 0.5}, 2).ok); // size 1 != axisCount 2
    }

    return 0;
}
