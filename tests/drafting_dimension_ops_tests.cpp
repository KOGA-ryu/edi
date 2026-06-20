#include "drafting/DraftingDimensionOps.h"
#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    EDI_CHECK(draftingDimensionKindFromId("distance") == DimensionKind::Distance);
    EDI_CHECK(draftingDimensionKindFromId("width") == DimensionKind::Width);
    EDI_CHECK(draftingDimensionKindFromId("height") == DimensionKind::Height);
    EDI_CHECK(draftingDimensionKindFromId("radius") == DimensionKind::Radius);
    EDI_CHECK(draftingDimensionKindFromId("diameter") == DimensionKind::Diameter);
    EDI_CHECK(!draftingDimensionKindFromId("ordinal"));

    DimensionGeometry angled{DimensionKind::Distance, {0.1, 0.2}, {0.4, 0.6}, 0.04};
    const double angledLength = std::hypot(0.3, 0.4);

    auto width = planDimensionKindChange(angled, DimensionKind::Width);
    EDI_CHECK(width.ok);
    EDI_CHECK(width.geometry.kind == DimensionKind::Width);
    EDI_CHECK(near(width.geometry.a.x, 0.1));
    EDI_CHECK(near(width.geometry.a.y, 0.2));
    EDI_CHECK(near(width.geometry.b.x, 0.1 + angledLength));
    EDI_CHECK(near(width.geometry.b.y, 0.2));
    EDI_CHECK(near(width.geometry.offset, 0.04));

    DimensionGeometry reversedWidth{DimensionKind::Distance, {0.5, 0.2}, {0.1, 0.6}, 0.04};
    auto widthNegative = planDimensionKindChange(reversedWidth, DimensionKind::Width);
    EDI_CHECK(widthNegative.ok);
    EDI_CHECK(near(widthNegative.geometry.b.x, 0.5 - std::hypot(0.4, 0.4)));
    EDI_CHECK(near(widthNegative.geometry.b.y, 0.2));

    auto height = planDimensionKindChange(angled, DimensionKind::Height);
    EDI_CHECK(height.ok);
    EDI_CHECK(height.geometry.kind == DimensionKind::Height);
    EDI_CHECK(near(height.geometry.b.x, 0.1));
    EDI_CHECK(near(height.geometry.b.y, 0.2 + angledLength));

    DimensionGeometry reversedHeight{DimensionKind::Distance, {0.1, 0.6}, {0.4, 0.2}, 0.04};
    auto heightNegative = planDimensionKindChange(reversedHeight, DimensionKind::Height);
    EDI_CHECK(heightNegative.ok);
    EDI_CHECK(near(heightNegative.geometry.b.x, 0.1));
    EDI_CHECK(near(heightNegative.geometry.b.y, 0.6 - std::hypot(0.3, 0.4)));

    auto diameter = planDimensionKindChange(angled, DimensionKind::Diameter);
    EDI_CHECK(diameter.ok);
    EDI_CHECK(diameter.geometry.kind == DimensionKind::Diameter);
    EDI_CHECK(near(diameter.geometry.b.x, 0.4));
    EDI_CHECK(near(diameter.geometry.b.y, 0.6));

    auto collapsed = planDimensionKindChange(
        DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.1, 0.2}, 0.04},
        DimensionKind::Width);
    EDI_CHECK(!collapsed.ok);
    EDI_CHECK(collapsed.code == DraftingResultCode::InvalidGeometry);

    auto nonFinite = planDimensionKindChange(
        DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {std::numeric_limits<double>::infinity(), 0.2}, 0.04},
        DimensionKind::Width);
    EDI_CHECK(!nonFinite.ok);
    EDI_CHECK(nonFinite.code == DraftingResultCode::InvalidGeometry);

    // planDimensionKindChange → Angular must be rejected (S5): there is no
    // lossless mapping from a linear dimension's a/b/offset to the angular
    // vertex + two-ray + included-angle encoding.
    auto toAngular = planDimensionKindChange(angled, DimensionKind::Angular);
    EDI_CHECK(!toAngular.ok);
    EDI_CHECK(toAngular.code == DraftingResultCode::InvalidSelectionTarget);

    // planAngularDimension — perpendicular lines (S7 nominal case).
    // l1 along +X, l2 along +Y; they cross at the origin.
    // Expected: vertex = (0,0), ray1 tip = (0 + 0.1, 0) = (0.1, 0),
    //           offset (included angle) ≈ 90°.
    {
        const LineGeometry l1{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry l2{{0.0, 0.0}, {0.0, 1.0}};
        auto plan = planAngularDimension(l1, l2);
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.geometry.kind == DimensionKind::Angular);
        EDI_CHECK(near(plan.geometry.a.x, 0.0) && near(plan.geometry.a.y, 0.0));
        EDI_CHECK(near(plan.geometry.b.x, kDefaultAngularArc) && near(plan.geometry.b.y, 0.0));
        EDI_CHECK(near(plan.geometry.offset, 90.0));
    }

    // Offset-origin: lines crossing off-origin still produce correct vertex.
    // l1 horizontal at y=1, l2 vertical at x=2 → vertex = (2,1).
    {
        const LineGeometry l1{{0.0, 1.0}, {3.0, 1.0}};
        const LineGeometry l2{{2.0, 0.0}, {2.0, 3.0}};
        auto plan = planAngularDimension(l1, l2);
        EDI_CHECK(plan.ok);
        EDI_CHECK(near(plan.geometry.a.x, 2.0) && near(plan.geometry.a.y, 1.0));
        EDI_CHECK(near(plan.geometry.offset, 90.0));
    }

    // Parallel lines must be rejected (S7).
    {
        const LineGeometry l1{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry l2{{0.0, 1.0}, {1.0, 1.0}};
        auto plan = planAngularDimension(l1, l2);
        EDI_CHECK(!plan.ok);
        EDI_CHECK(plan.code == DraftingResultCode::InvalidSelectionTarget);
    }

    // dimensionMeasuredAngle (S3): Angular returns offset; linear returns
    // the base-ray direction from dimensionAngleDegrees.
    {
        DimensionGeometry angDim;
        angDim.kind   = DimensionKind::Angular;
        angDim.a      = {0.0, 0.0};
        angDim.b      = {0.1, 0.0};
        angDim.offset = 45.0;
        EDI_CHECK(near(dimensionMeasuredAngle(angDim), 45.0));

        DimensionGeometry linDim{DimensionKind::Distance, {0.0, 0.0}, {1.0, 1.0}, 0.04};
        EDI_CHECK(near(dimensionMeasuredAngle(linDim), 45.0)); // atan2(1,1)*180/π = 45°
    }

    // draftingDimensionKindFromId round-trip for "angular" (S1).
    EDI_CHECK(draftingDimensionKindFromId("angular") == DimensionKind::Angular);

    // DR-14: convenience planners over existing kinds -------------------------

    // planRadialDimensionForCircle: kind=Diameter; stored |b-a|=radius;
    // displayedDimensionLength doubles it to yield the diameter.
    {
        CircleGeometry unit{{0.0, 0.0}, 1.0};
        auto plan = planRadialDimensionForCircle(unit);
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.geometry.kind == DimensionKind::Diameter);
        EDI_CHECK(near(plan.geometry.a.x, 0.0) && near(plan.geometry.a.y, 0.0));
        const double stored = std::hypot(plan.geometry.b.x - plan.geometry.a.x,
                                         plan.geometry.b.y - plan.geometry.a.y);
        EDI_CHECK(near(stored, 1.0));                               // stored == radius
        EDI_CHECK(near(displayedDimensionLength(stored, DimensionKind::Diameter), 2.0)); // shown == diameter
        // zero-radius rejects
        EDI_CHECK(!planRadialDimensionForCircle(CircleGeometry{{0.0, 0.0}, 0.0}).ok);
        EDI_CHECK(!planRadialDimensionForCircle(
                   CircleGeometry{{0.0, 0.0}, std::numeric_limits<double>::infinity()}).ok);
    }

    // planRadialDimensionForArc: kind=Radius; b is ON the arc; |b-a|==radius.
    {
        ArcGeometry halfArc{{1.0, 2.0}, 3.0, 30.0, 90.0};
        auto plan = planRadialDimensionForArc(halfArc);
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.geometry.kind == DimensionKind::Radius);
        EDI_CHECK(near(plan.geometry.a.x, 1.0) && near(plan.geometry.a.y, 2.0));
        const double dist = std::hypot(plan.geometry.b.x - plan.geometry.a.x,
                                       plan.geometry.b.y - plan.geometry.a.y);
        EDI_CHECK(near(dist, 3.0)); // |b-a| == radius, so b lies on the arc
        // zero-radius rejects
        EDI_CHECK(!planRadialDimensionForArc(ArcGeometry{{0.0, 0.0}, 0.0, 0.0, 90.0}).ok);
    }

    // planArcSweepDimension: kind=Angular; offset == signed sweep;
    // dimensionMeasuredAngle returns offset.
    {
        // 0°→90° quarter arc
        ArcGeometry q{{0.0, 0.0}, 2.0, 0.0, 90.0};
        auto plan = planArcSweepDimension(q);
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.geometry.kind == DimensionKind::Angular);
        EDI_CHECK(near(plan.geometry.offset, 90.0));
        EDI_CHECK(near(dimensionMeasuredAngle(plan.geometry), 90.0));

        // 30°→120° arc — sweep is still 90°
        ArcGeometry shifted{{0.0, 0.0}, 1.0, 30.0, 120.0};
        auto plan2 = planArcSweepDimension(shifted);
        EDI_CHECK(plan2.ok);
        EDI_CHECK(near(plan2.geometry.offset, 90.0));

        // ray1 tip is at distance kDefaultAngularArc from center along startAngle
        EDI_CHECK(near(std::hypot(plan.geometry.b.x - plan.geometry.a.x,
                               plan.geometry.b.y - plan.geometry.a.y),
                    kDefaultAngularArc));

        // zero-radius rejects
        EDI_CHECK(!planArcSweepDimension(ArcGeometry{{0.0, 0.0}, 0.0, 0.0, 90.0}).ok);
        // zero-sweep rejects
        EDI_CHECK(!planArcSweepDimension(ArcGeometry{{0.0, 0.0}, 1.0, 45.0, 45.0}).ok);
    }

    return 0;
}
