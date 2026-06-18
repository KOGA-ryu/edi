#include "drafting/DraftingArray.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

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
    // The axis id picks which of the caller's two spacings survives (the
    // other is zeroed); count and spacing themselves are tool-option state
    // passed through verbatim.
    const std::optional<DraftingArrayRepeatSettings> repeatX = draftingArrayRepeatSettingsFromAxisId("x", 5, 0.25, 0.4);
    assert(repeatX);
    assert(repeatX->copyCount == 5);
    assert(nearlyEqual(repeatX->spacingX, 0.25));
    assert(nearlyEqual(repeatX->spacingY, 0.0));

    const std::optional<DraftingArrayRepeatSettings> repeatY = draftingArrayRepeatSettingsFromAxisId("y", 2, 0.25, 0.05);
    assert(repeatY);
    assert(repeatY->copyCount == 2);
    assert(nearlyEqual(repeatY->spacingX, 0.0));
    assert(nearlyEqual(repeatY->spacingY, 0.05));
    assert(!draftingArrayRepeatSettingsFromAxisId("diagonal", 3, 0.1, 0.1));

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.2}, {0.4, 0.2}});
    line.stroke.color = "#55ccaa";
    auto repeatedLine = repeatDraftingObject(line, {"repeat_1", "repeat_2", "repeat_3"}, 0.1, 0.0);
    assert(repeatedLine.ok);
    assert(repeatedLine.objects.size() == 3);
    assert(repeatedLine.objects[0].id == "repeat_1");
    assert(repeatedLine.objects[0].kind == DraftingShapeKind::Line);
    assert(repeatedLine.objects[0].metadata.toolProvenance == "repeat");
    assert(repeatedLine.objects[0].metadata.source == "line_1");
    assert(repeatedLine.objects[0].stroke.color == "#55ccaa");
    const auto *firstLine = std::get_if<LineGeometry>(&repeatedLine.objects[0].geometry);
    const auto *thirdLine = std::get_if<LineGeometry>(&repeatedLine.objects[2].geometry);
    assert(firstLine != nullptr);
    assert(thirdLine != nullptr);
    assert(nearlyEqual(firstLine->a.x, 0.2));
    assert(nearlyEqual(firstLine->a.y, 0.2));
    assert(nearlyEqual(thirdLine->a.x, 0.4));
    assert(nearlyEqual(thirdLine->b.x, 0.7));

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    auto repeatedCircle = repeatDraftingObject(circle, {"circle_repeat_1", "circle_repeat_2"}, 0.0, 0.2);
    assert(repeatedCircle.ok);
    const auto *secondCircle = std::get_if<CircleGeometry>(&repeatedCircle.objects[1].geometry);
    assert(secondCircle != nullptr);
    assert(nearlyEqual(secondCircle->center.x, 0.25));
    assert(nearlyEqual(secondCircle->center.y, 0.65));
    assert(nearlyEqual(secondCircle->radius, 0.1));

    DraftingObject guide = object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.8});
    auto invalidGuideRepeat = repeatDraftingObject(guide, {"guide_repeat_1", "guide_repeat_2", "guide_repeat_3"}, 0.0, 0.1);
    assert(!invalidGuideRepeat.ok);
    assert(invalidGuideRepeat.code == DraftingResultCode::InvalidGeometry);

    auto empty = repeatDraftingObject(line, {}, 0.1, 0.0);
    assert(!empty.ok);
    assert(empty.code == DraftingResultCode::InvalidGeometry);

    auto zeroSpacing = repeatDraftingObject(line, {"zero_repeat"}, 0.0, 0.0);
    assert(!zeroSpacing.ok);
    assert(zeroSpacing.code == DraftingResultCode::InvalidGeometry);

    auto badSpacing = repeatDraftingObject(line, {"bad_repeat"}, std::numeric_limits<double>::infinity(), 0.0);
    assert(!badSpacing.ok);
    assert(badSpacing.code == DraftingResultCode::InvalidGeometry);

    auto duplicateIds = repeatDraftingObject(line, {"duplicate_repeat", "duplicate_repeat"}, 0.1, 0.0);
    assert(!duplicateIds.ok);
    assert(duplicateIds.code == DraftingResultCode::DuplicateObjectId);

    auto sourceIdReuse = repeatDraftingObject(line, {"line_1"}, 0.1, 0.0);
    assert(!sourceIdReuse.ok);
    assert(sourceIdReuse.code == DraftingResultCode::DuplicateObjectId);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Point;
    auto mismatch = repeatDraftingObject(mismatched, {"mismatch_repeat"}, 0.1, 0.0);
    assert(!mismatch.ok);
    assert(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    // Grid array: source occupies cell (0,0); copies fill the rest row-major.
    {
        DraftingObject dot = object("dot_1", DraftingShapeKind::Point, PointGeometry{{0.2, 0.3}});
        dot.stroke.color = "#112233";
        auto grid = gridArrayDraftingObject(dot, {"g_1", "g_2", "g_3", "g_4", "g_5"}, 3, 2, 0.1, 0.2);
        assert(grid.ok);
        assert(grid.objects.size() == 5);
        assert(grid.objects[0].metadata.toolProvenance == "grid_array");
        assert(grid.objects[0].metadata.source == "dot_1");
        assert(grid.objects[0].stroke.color == "#112233");
        // Row 0: cells (1,0) and (2,0).
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[0].geometry).point.x, 0.3));
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[0].geometry).point.y, 0.3));
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[1].geometry).point.x, 0.4));
        // Row 1: cells (0,1), (1,1), (2,1).
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[2].geometry).point.x, 0.2));
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[2].geometry).point.y, 0.5));
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[4].geometry).point.x, 0.4));
        assert(nearlyEqual(std::get<PointGeometry>(grid.objects[4].geometry).point.y, 0.5));

        // A single column needs no X spacing — it is a vertical run.
        auto column = gridArrayDraftingObject(dot, {"c_1", "c_2"}, 1, 3, 0.0, 0.1);
        assert(column.ok);
        assert(column.objects.size() == 2);

        // Rejections: id count must match cells; the grid must actually
        // create something; spacing on a multi-cell axis must move copies.
        assert(!gridArrayDraftingObject(dot, {"bad_1"}, 3, 2, 0.1, 0.2).ok);
        assert(!gridArrayDraftingObject(dot, {}, 1, 1, 0.1, 0.1).ok);
        assert(!gridArrayDraftingObject(dot, {"bad_2"}, 0, 2, 0.1, 0.1).ok);
        assert(!gridArrayDraftingObject(dot, {"bad_3", "bad_4"}, 3, 1, 0.0, 0.1).ok);
        assert(!gridArrayDraftingObject(dot, {"bad_5"}, 2, 1,
                                        std::numeric_limits<double>::infinity(), 0.0).ok);
        assert(!gridArrayDraftingObject(dot, {"dot_1"}, 2, 1, 0.1, 0.0).ok);

        // Guides translate on one axis only, so 2D placements would stack
        // exact duplicates on the source — both planners refuse them.
        DraftingObject guideSource = object("guide_src", DraftingShapeKind::Guide,
                                            GuideGeometry{GuideOrientation::Horizontal, 0.5});
        auto guideGrid = gridArrayDraftingObject(guideSource, {"gg_1", "gg_2", "gg_3"}, 2, 2, 0.1, 0.1);
        assert(!guideGrid.ok);
        assert(guideGrid.code == DraftingResultCode::InvalidGeometry);
        auto guideRadial = radialArrayDraftingObject(guideSource, {"gr_1"}, {0.5, 0.7});
        assert(!guideRadial.ok);
        assert(guideRadial.code == DraftingResultCode::InvalidGeometry);
    }

    // Radial array: copies fill the remaining slots of an evenly divided
    // ring (copies + source) around the centre; geometry is only translated.
    {
        DraftingObject ringCircle = object("ring_1", DraftingShapeKind::Circle, CircleGeometry{{0.3, 0.5}, 0.05});
        auto radial = radialArrayDraftingObject(ringCircle, {"r_1", "r_2", "r_3"}, {0.5, 0.5});
        assert(radial.ok);
        assert(radial.objects.size() == 3);
        assert(radial.objects[0].metadata.toolProvenance == "radial_array");
        assert(radial.objects[0].metadata.source == "ring_1");
        // Arm is (-0.2, 0); slots = 4, so copies sit at 90/180/270 degrees.
        const auto *quarter = std::get_if<CircleGeometry>(&radial.objects[0].geometry);
        assert(quarter != nullptr);
        assert(nearlyEqual(quarter->center.x, 0.5));
        assert(nearlyEqual(quarter->center.y, 0.3));
        assert(nearlyEqual(quarter->radius, 0.05)); // size never changes
        const auto *half = std::get_if<CircleGeometry>(&radial.objects[1].geometry);
        assert(nearlyEqual(half->center.x, 0.7));
        assert(nearlyEqual(half->center.y, 0.5));
        const auto *threeQuarter = std::get_if<CircleGeometry>(&radial.objects[2].geometry);
        assert(nearlyEqual(threeQuarter->center.x, 0.5));
        assert(nearlyEqual(threeQuarter->center.y, 0.7));

        // The ring anchor is the source's BOUNDS centre — pinned with an
        // asymmetric source whose bounds centre differs from its first
        // endpoint. Line (0.2,0.45)-(0.4,0.35): bounds centre (0.3, 0.4),
        // arm (-0.2, 0) from centre (0.5, 0.4); one copy -> 180deg, so the
        // whole line translates by (+0.4, 0). An endpoint- or origin-anchored
        // planner produces different endpoints and fails here.
        DraftingObject ringLine = object("ring_line", DraftingShapeKind::Line,
                                         LineGeometry{{0.2, 0.45}, {0.4, 0.35}});
        auto lineRadial = radialArrayDraftingObject(ringLine, {"rl_1"}, {0.5, 0.4});
        assert(lineRadial.ok);
        const auto *flipped = std::get_if<LineGeometry>(&lineRadial.objects[0].geometry);
        assert(flipped != nullptr);
        assert(nearlyEqual(flipped->a.x, 0.6));
        assert(nearlyEqual(flipped->a.y, 0.45));
        assert(nearlyEqual(flipped->b.x, 0.8));
        assert(nearlyEqual(flipped->b.y, 0.35));

        // Rejections: no copies; non-finite centre; centre on the object
        // (every copy would land exactly on the source).
        assert(!radialArrayDraftingObject(ringCircle, {}, {0.5, 0.5}).ok);
        assert(!radialArrayDraftingObject(ringCircle, {"r_bad"},
                                          {std::numeric_limits<double>::quiet_NaN(), 0.5}).ok);
        auto degenerate = radialArrayDraftingObject(ringCircle, {"r_bad"}, {0.3, 0.5});
        assert(!degenerate.ok);
        assert(degenerate.code == DraftingResultCode::InvalidGeometry);
    }

    // Rotate-copies: same arm distribution as the radial array (step =
    // totalAngle/(copies+1)), but each copy is ROTATED to its spoke. A square at
    // bounds centre (0.75,0.5), centre (0.5,0.5), full 360 ring, 3 copies → spokes
    // 90/180/270: rotationDeg advances per spoke AND the centre orbits arm 0.25.
    {
        DraftingObject square = object("rc_src", DraftingShapeKind::Rectangle,
                                       RectangleGeometry{{0.7, 0.45}, 0.1, 0.1, 0.0, 0.0, 0.0});
        auto rosette = rotateCopiesDraftingObject(square, {"rc_1", "rc_2", "rc_3"}, {0.5, 0.5}, 360.0);
        assert(rosette.ok);
        assert(rosette.objects.size() == 3);
        assert(rosette.objects[0].metadata.toolProvenance == "rotate_copies");
        assert(rosette.objects[0].metadata.source == "rc_src");

        const double arm = 0.25; // |(0.75,0.5) - (0.5,0.5)|
        const double expectedAngle[3] = {90.0, 180.0, 270.0};
        for (std::size_t i = 0; i < 3; ++i) {
            const auto *r = std::get_if<RectangleGeometry>(&rosette.objects[i].geometry);
            assert(r != nullptr);
            assert(nearlyEqual(r->rotationDeg, expectedAngle[i])); // turns with the spoke
            assert(nearlyEqual(r->width, 0.1) && nearlyEqual(r->height, 0.1)); // scale 1.0
            // The rectangle's centre (origin + half-extent) orbits the ring.
            const double cx = r->origin.x + r->width / 2.0;
            const double cy = r->origin.y + r->height / 2.0;
            assert(nearlyEqual(std::hypot(cx - 0.5, cy - 0.5), arm));
        }
        // The 90° copy's centre is (0.75,0.5) rotated 90° about (0.5,0.5) → (0.5,0.75).
        const auto *first = std::get_if<RectangleGeometry>(&rosette.objects[0].geometry);
        assert(nearlyEqual(first->origin.x + first->width / 2.0, 0.5));
        assert(nearlyEqual(first->origin.y + first->height / 2.0, 0.75));

        // Rejections: duplicate ids; no copies; non-finite; guides (cannot rotate).
        auto dup = rotateCopiesDraftingObject(square, {"d", "d"}, {0.5, 0.5}, 360.0);
        assert(!dup.ok);
        assert(dup.code == DraftingResultCode::DuplicateObjectId);
        assert(!rotateCopiesDraftingObject(square, {}, {0.5, 0.5}, 360.0).ok);
        assert(!rotateCopiesDraftingObject(square, {"x"},
                                           {std::numeric_limits<double>::quiet_NaN(), 0.5}, 360.0).ok);
        DraftingObject guideSrc = object("rc_guide", DraftingShapeKind::Guide,
                                         GuideGeometry{GuideOrientation::Horizontal, 0.5});
        auto guideRosette = rotateCopiesDraftingObject(guideSrc, {"g1"}, {0.5, 0.5}, 360.0);
        assert(!guideRosette.ok);
        assert(guideRosette.code == DraftingResultCode::InvalidGeometry);
    }

    // Array along a curve: K copies whose bounds-centre lands on evenly
    // arc-length-spaced stations of the path, optionally rotated to the tangent.
    {
        // -- Line, no tangent: path (0,0)→(1,0), K=3 ----------------------
        // Three copies; stations at d = 0, 0.5, 1.0.
        // Source is a small circle at the origin (bounds-centre C = (0,0)), so
        // each copy's circle-centre ends up exactly on its station.
        DraftingObject srcCircle = object("aac_src",
            DraftingShapeKind::Circle,
            CircleGeometry{{0.0, 0.0}, 0.05});
        const DraftingGeometry hLine{LineGeometry{{0.0, 0.0}, {1.0, 0.0}}};
        auto lineNoTan = arrayAlongCurve(srcCircle, {"aac_0", "aac_1", "aac_2"}, hLine, /*alignToTangent=*/false);
        assert(lineNoTan.ok);
        assert(lineNoTan.objects.size() == 3);
        assert(lineNoTan.objects[0].metadata.toolProvenance == "array_along_curve");
        assert(lineNoTan.objects[0].metadata.source == "aac_src");
        const auto *c0 = std::get_if<CircleGeometry>(&lineNoTan.objects[0].geometry);
        const auto *c1 = std::get_if<CircleGeometry>(&lineNoTan.objects[1].geometry);
        const auto *c2 = std::get_if<CircleGeometry>(&lineNoTan.objects[2].geometry);
        assert(c0 != nullptr && c1 != nullptr && c2 != nullptr);
        // stations: x = 0.0, 0.5, 1.0; all y = 0
        assert(nearlyEqual(c0->center.x, 0.0) && nearlyEqual(c0->center.y, 0.0));
        assert(nearlyEqual(c1->center.x, 0.5) && nearlyEqual(c1->center.y, 0.0));
        assert(nearlyEqual(c2->center.x, 1.0) && nearlyEqual(c2->center.y, 0.0));
        assert(nearlyEqual(c0->radius, 0.05)); // size is preserved

        // -- Line, tangent: same path at 45° → each copy rotationDeg = 45 --
        // Source: unit square Rectangle at origin (rotationDeg = 0).
        // After alignToTangent the rectangle's rotationDeg accumulates the
        // line angle because transformGeometry adds rotationDeg in-place.
        DraftingObject srcRect = object("aac_rect",
            DraftingShapeKind::Rectangle,
            RectangleGeometry{{0.0, 0.0}, 0.1, 0.1, 0.0, 0.0, 0.0});
        const DraftingGeometry diagLine{LineGeometry{{0.0, 0.0}, {1.0, 1.0}}};
        auto lineTan = arrayAlongCurve(srcRect, {"aac_t0", "aac_t1"}, diagLine, /*alignToTangent=*/true);
        assert(lineTan.ok);
        assert(lineTan.objects.size() == 2);
        for (const DraftingObject &obj : lineTan.objects) {
            const auto *r = std::get_if<RectangleGeometry>(&obj.geometry);
            assert(r != nullptr);
            // The tangent of a 45° line is 45°; transformGeometry adds it to
            // the rectangle's existing rotationDeg (0 → 45).
            assert(nearlyEqual(r->rotationDeg, 45.0));
        }

        // -- Arc: quarter circle, K=3 → centres ON the arc ----------------
        // Arc centre (0,0), radius 1, sweep 0°→90°.
        // Stations: 0°, 45°, 90° on the circle.  Any point at arc-distance r
        // from the arc centre satisfies this.  Use a PointGeometry source with
        // its own point at (2,0) (off the arc) so the translate has a clear
        // effect; after placement each copy sits on the arc.
        DraftingObject srcPt = object("aac_pt",
            DraftingShapeKind::Point,
            PointGeometry{{2.0, 0.0}});
        const DraftingGeometry quarterArc{ArcGeometry{{0.0, 0.0}, 1.0, 0.0, 90.0}};
        auto arcResult = arrayAlongCurve(srcPt, {"arc_0", "arc_1", "arc_2"}, quarterArc, /*alignToTangent=*/false);
        assert(arcResult.ok);
        assert(arcResult.objects.size() == 3);
        for (const DraftingObject &obj : arcResult.objects) {
            const auto *pt = std::get_if<PointGeometry>(&obj.geometry);
            assert(pt != nullptr);
            // Each station is on the unit circle: distance from (0,0) ≈ 1.
            const double dist = std::hypot(pt->point.x - 0.0, pt->point.y - 0.0);
            assert(nearlyEqual(dist, 1.0));
        }
        // First station is the arc's start (1,0), last is the arc's end (0,1).
        {
            const auto *pt0 = std::get_if<PointGeometry>(&arcResult.objects[0].geometry);
            const auto *pt2 = std::get_if<PointGeometry>(&arcResult.objects[2].geometry);
            assert(nearlyEqual(pt0->point.x, 1.0) && nearlyEqual(pt0->point.y, 0.0));
            assert(nearlyEqual(pt2->point.x, 0.0) && nearlyEqual(pt2->point.y, 1.0));
        }

        // -- Rejections ---------------------------------------------------
        // Empty id list.
        auto emptyIds = arrayAlongCurve(srcCircle, {}, hLine, false);
        assert(!emptyIds.ok);
        assert(emptyIds.code == DraftingResultCode::InvalidGeometry);

        // Unsupported path kind (Circle).
        const DraftingGeometry circlePath{CircleGeometry{{0.0, 0.0}, 1.0}};
        auto circleReject = arrayAlongCurve(srcCircle, {"rej_0"}, circlePath, false);
        assert(!circleReject.ok);
        assert(circleReject.code == DraftingResultCode::InvalidGeometry);

        // Unsupported path kind (Rectangle).
        const DraftingGeometry rectPath{RectangleGeometry{{0.0, 0.0}, 1.0, 1.0, 0.0, 0.0, 0.0}};
        auto rectReject = arrayAlongCurve(srcCircle, {"rej_1"}, rectPath, false);
        assert(!rectReject.ok);
        assert(rectReject.code == DraftingResultCode::InvalidGeometry);

        // Zero-length path.
        const DraftingGeometry zeroLine{LineGeometry{{0.5, 0.5}, {0.5, 0.5}}};
        auto zeroLen = arrayAlongCurve(srcCircle, {"rej_2"}, zeroLine, false);
        assert(!zeroLen.ok);
        assert(zeroLen.code == DraftingResultCode::InvalidGeometry);
    }

    return 0;
}
