#include "drafting/DraftingModify.h"
#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
#include <cmath>
#include <variant>
#include <vector>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

bool pointNear(Point2D p, double x, double y, double eps = 0.000001)
{
    return nearlyEqual(p.x, x, eps) && nearlyEqual(p.y, y, eps);
}

} // namespace

int main()
{
    // --- segmentIntersection ---------------------------------------------
    {
        // A clean cross at (0.7, 0.5).
        const LineGeometry horizontal{{0.0, 0.5}, {1.0, 0.5}};
        const LineGeometry vertical{{0.7, 0.0}, {0.7, 1.0}};
        const auto x = segmentIntersection(horizontal, vertical);
        EDI_CHECK(x && pointNear(*x, 0.7, 0.5));

        // Parallel lines never cross.
        const LineGeometry parallel{{0.0, 0.6}, {1.0, 0.6}};
        EDI_CHECK(!segmentIntersection(horizontal, parallel));

        // Collinear (overlapping) is treated as no single crossing.
        const LineGeometry collinear{{0.2, 0.5}, {0.4, 0.5}};
        EDI_CHECK(!segmentIntersection(horizontal, collinear));

        // The infinite lines would cross, but off the segments: reject.
        const LineGeometry shortVertical{{0.7, 0.8}, {0.7, 0.9}}; // above the target
        EDI_CHECK(!segmentIntersection(horizontal, shortVertical));

        // A diagonal crossing, verified against the parametric point.
        const LineGeometry a{{0.0, 0.0}, {1.0, 1.0}};
        const LineGeometry b{{0.0, 1.0}, {1.0, 0.0}};
        const auto mid = segmentIntersection(a, b);
        EDI_CHECK(mid && pointNear(*mid, 0.5, 0.5));

        // SHORT but cleanly perpendicular segments cross — "parallel" must mean
        // a small ANGLE, not a small determinant. With an absolute determinant
        // threshold these (length ~5e-7) would be falsely rejected as parallel.
        const LineGeometry tinyH{{0.0, 0.0}, {5e-7, 0.0}};
        const LineGeometry tinyV{{2.5e-7, -2.5e-7}, {2.5e-7, 2.5e-7}};
        const auto tiny = segmentIntersection(tinyH, tinyV);
        EDI_CHECK(tiny && pointNear(*tiny, 2.5e-7, 0.0, 1e-12));
    }

    // --- trimLineAtPoint -------------------------------------------------
    const LineGeometry target{{0.0, 0.5}, {1.0, 0.5}};
    const LineGeometry cutAt07{{0.7, 0.0}, {0.7, 1.0}};

    {
        // Click the RIGHT stub (near b): the b end moves to the cut, keeping a..X.
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt07}, {0.9, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.geometry.a, 0.0, 0.5)); // a kept
        EDI_CHECK(pointNear(r.geometry.b, 0.7, 0.5)); // b pulled to the crossing
    }
    {
        // Click the LEFT stub (near a): the a end moves to the cut, keeping X..b.
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt07}, {0.1, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.geometry.a, 0.7, 0.5)); // a pulled to the crossing
        EDI_CHECK(pointNear(r.geometry.b, 1.0, 0.5)); // b kept
    }
    {
        // The doomed side is chosen by the CUT, not the midpoint. Cut at 0.8,
        // click at 0.7 (in the LARGE left piece [0,0.8]): that piece is removed,
        // keeping the short right stub. A midpoint split would wrongly keep the
        // clicked piece (0.7 is left of the 0.5 midpoint -> would drop the right).
        const LineGeometry cutAt08{{0.8, 0.0}, {0.8, 1.0}};
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt08}, {0.7, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.geometry.a, 0.8, 0.5)); // a pulled up to the cut
        EDI_CHECK(pointNear(r.geometry.b, 1.0, 0.5)); // the stub the user did NOT click stays
    }
    {
        // Two boundaries: the crossing NEAREST the click is the cut. Clicking
        // far right (0.95) past the 0.7 cut trims only the stub beyond 0.7.
        const LineGeometry cutAt03{{0.3, 0.0}, {0.3, 1.0}};
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt03, cutAt07}, {0.95, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.geometry.a, 0.0, 0.5));
        EDI_CHECK(pointNear(r.geometry.b, 0.7, 0.5)); // nearest cut to the click wins
    }
    {
        // No boundary crosses the target: reject with a reason.
        const LineGeometry parallel{{0.0, 0.2}, {1.0, 0.2}};
        const DraftingTrimResult r = trimLineAtPoint(target, {parallel}, {0.5, 0.5});
        EDI_CHECK(!r.ok && r.code == DraftingResultCode::InvalidSelectionTarget);
    }
    {
        // An empty boundary set cannot trim.
        EDI_CHECK(!trimLineAtPoint(target, {}, {0.5, 0.5}).ok);
    }
    {
        // A cut at the very end the click keeps would collapse the line: a
        // boundary crossing exactly at endpoint a, clicked near a -> a moves to
        // a (zero change) but the OTHER direction (clicking near b with a cut at
        // b) collapses. Construct the collapse: cut at x=1.0 (endpoint b),
        // clicked near b -> b moves to (1.0,0.5) == b... still length 1. Instead
        // cut at x=0.0 (endpoint a) clicked near a keeps a..b unchanged. The
        // genuine collapse: cut at x=1.0, click near a -> keep X..b = (1.0)..(1.0).
        const LineGeometry cutAtB{{1.0, 0.0}, {1.0, 1.0}};
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAtB}, {0.1, 0.5});
        // click near a -> remove a side -> keep (X=(1.0,0.5))..b=(1.0,0.5): zero length.
        EDI_CHECK(!r.ok && r.code == DraftingResultCode::InvalidGeometry);
    }

    // --- lineIntersection (infinite) ------------------------------------
    {
        // Two SHORT segments that never reach each other, but whose infinite
        // lines cross at (0.5, 0.0): segment intersection fails, line does not.
        const LineGeometry h{{0.0, 0.0}, {0.3, 0.0}};
        const LineGeometry v{{0.5, -0.2}, {0.5, -0.1}};
        EDI_CHECK(!segmentIntersection(h, v));
        const auto x = lineIntersection(h, v);
        EDI_CHECK(x && pointNear(*x, 0.5, 0.0));
        // Parallel still has no crossing.
        EDI_CHECK(!lineIntersection(h, LineGeometry{{0.0, 0.4}, {0.3, 0.4}}));
    }

    // --- filletLines -----------------------------------------------------
    {
        // L-corner sharing the origin, right angle, radius 0.2. Each line's
        // corner end pulls back to its tangent point; the far end stays.
        const LineGeometry h{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry v{{0.0, 0.0}, {0.0, 1.0}};
        const DraftingFilletResult r = filletLines(h, v, 0.2, {0.5, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.line1.a, 0.2, 0.0) && pointNear(r.line1.b, 1.0, 0.0));
        EDI_CHECK(pointNear(r.line2.a, 0.0, 0.2) && pointNear(r.line2.b, 0.0, 1.0));
        EDI_CHECK(pointNear(r.arc.center, 0.2, 0.2) && nearlyEqual(r.arc.radius, 0.2));
        // Central angle = 180 - 90 = 90 (the minor, corner-rounding arc).
        EDI_CHECK(nearlyEqual(std::abs(r.arc.endAngleDeg - r.arc.startAngleDeg), 90.0));
        // The arc's endpoints ARE the two tangent points (it joins the lines).
        const Point2D arcStart = arcPointAtAngle(r.arc.center, r.arc.radius, r.arc.startAngleDeg);
        const Point2D arcEnd = arcPointAtAngle(r.arc.center, r.arc.radius, r.arc.endAngleDeg);
        const bool joins =
            (pointNear(arcStart, 0.2, 0.0, 1e-9) && pointNear(arcEnd, 0.0, 0.2, 1e-9))
            || (pointNear(arcStart, 0.0, 0.2, 1e-9) && pointNear(arcEnd, 0.2, 0.0, 1e-9));
        EDI_CHECK(joins);
    }
    {
        // X-crossing: filleting the +x+y corner keeps each line's + arm.
        const LineGeometry h{{-1.0, 0.0}, {1.0, 0.0}};
        const LineGeometry v{{0.0, -1.0}, {0.0, 1.0}};
        const DraftingFilletResult r = filletLines(h, v, 0.3, {0.5, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.line1.a, 0.3, 0.0) && pointNear(r.line1.b, 1.0, 0.0));
        EDI_CHECK(pointNear(r.line2.a, 0.0, 0.3) && pointNear(r.line2.b, 0.0, 1.0));
        EDI_CHECK(pointNear(r.arc.center, 0.3, 0.3) && nearlyEqual(r.arc.radius, 0.3));
        // A DIFFERENT corner of the same X: pick -x-y keeps each line's - arm.
        const DraftingFilletResult r2 = filletLines(h, v, 0.3, {-0.5, -0.5});
        EDI_CHECK(r2.ok);
        EDI_CHECK(pointNear(r2.arc.center, -0.3, -0.3));
        EDI_CHECK(pointNear(r2.line1.a, -1.0, 0.0) && pointNear(r2.line1.b, -0.3, 0.0));
        EDI_CHECK(pointNear(r2.line2.a, 0.0, -1.0) && pointNear(r2.line2.b, 0.0, -0.3));
    }
    {
        // Acute 60deg corner: the tangency invariant holds for a non-right angle
        // (each tangent point sits at exactly the radius from the centre), and
        // the central angle is 180 - 60 = 120.
        const double cos60 = 0.5;
        const double sin60 = 0.8660254037844386;
        const LineGeometry l1{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry l2{{0.0, 0.0}, {cos60, sin60}};
        const DraftingFilletResult r = filletLines(l1, l2, 0.1, {0.4, 0.2});
        EDI_CHECK(r.ok);
        const Point2D arcStart = arcPointAtAngle(r.arc.center, r.arc.radius, r.arc.startAngleDeg);
        const Point2D arcEnd = arcPointAtAngle(r.arc.center, r.arc.radius, r.arc.endAngleDeg);
        EDI_CHECK(nearlyEqual(distance(r.arc.center, arcStart), 0.1));
        EDI_CHECK(nearlyEqual(distance(r.arc.center, arcEnd), 0.1));
        EDI_CHECK(nearlyEqual(std::abs(r.arc.endAngleDeg - r.arc.startAngleDeg), 120.0));
    }
    {
        // Parallel lines have no corner; a non-positive radius is refused.
        const LineGeometry l1{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry l2{{0.0, 0.3}, {1.0, 0.3}};
        EDI_CHECK(!filletLines(l1, l2, 0.1, {0.5, 0.15}).ok);
        const LineGeometry vv{{0.0, 0.0}, {0.0, 1.0}};
        EDI_CHECK(!filletLines(l1, vv, 0.0, {0.5, 0.5}).ok);
        // A radius whose tangent point reaches the line's far end collapses it.
        const LineGeometry shortH{{0.0, 0.0}, {0.2, 0.0}};
        EDI_CHECK(!filletLines(shortH, vv, 0.2, {0.5, 0.5}).ok);
        // A radius whose tangent OVERSHOOTS the kept far end must reject — not
        // emit a reversed phantom segment that the length-only guard misses.
        EDI_CHECK(!filletLines(shortH, vv, 0.5, {0.5, 0.5}).ok);
        // The pick-side arm shorter than the radius (both ends below the tangent)
        // rejects rather than producing a backwards line.
        const LineGeometry stubH{{-1.0, 0.0}, {0.05, 0.0}};
        EDI_CHECK(!filletLines(stubH, vv, 0.2, {0.5, 0.5}).ok);
    }

    // --- chamferLines ----------------------------------------------------
    {
        // L-corner at the origin, equal setback → a 45° bevel between the two
        // setback points; each line's corner end pulls back, the far end stays.
        const LineGeometry h{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry v{{0.0, 0.0}, {0.0, 1.0}};
        const DraftingChamferResult r = chamferLines(h, v, 0.3, 0.3, {0.5, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.line1.a, 0.3, 0.0) && pointNear(r.line1.b, 1.0, 0.0));
        EDI_CHECK(pointNear(r.line2.a, 0.0, 0.3) && pointNear(r.line2.b, 0.0, 1.0));
        EDI_CHECK(pointNear(r.bevel.a, 0.3, 0.0) && pointNear(r.bevel.b, 0.0, 0.3));
        // Equal setback on a right angle → the bevel runs at 45° (|dx| == |dy|).
        EDI_CHECK(nearlyEqual(std::abs(r.bevel.b.x - r.bevel.a.x), std::abs(r.bevel.b.y - r.bevel.a.y)));
    }
    {
        // X-crossing: chamfering the -x-y corner keeps each line's - arm and bevels
        // across that corner (the pick selects the corner, like fillet).
        const LineGeometry h{{-1.0, 0.0}, {1.0, 0.0}};
        const LineGeometry v{{0.0, -1.0}, {0.0, 1.0}};
        const DraftingChamferResult r = chamferLines(h, v, 0.3, 0.3, {-0.5, -0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.line1.a, -1.0, 0.0) && pointNear(r.line1.b, -0.3, 0.0));
        EDI_CHECK(pointNear(r.line2.a, 0.0, -1.0) && pointNear(r.line2.b, 0.0, -0.3));
        EDI_CHECK(pointNear(r.bevel.a, -0.3, 0.0) && pointNear(r.bevel.b, 0.0, -0.3));
    }
    {
        // Asymmetric setbacks land at different distances along each line.
        const LineGeometry h{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry v{{0.0, 0.0}, {0.0, 1.0}};
        const DraftingChamferResult r = chamferLines(h, v, 0.2, 0.4, {0.5, 0.5});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.bevel.a, 0.2, 0.0) && pointNear(r.bevel.b, 0.0, 0.4));
    }
    {
        // Parallel lines have no corner; a non-positive setback is refused.
        const LineGeometry l1{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry l2{{0.0, 0.3}, {1.0, 0.3}};
        EDI_CHECK(!chamferLines(l1, l2, 0.1, 0.1, {0.5, 0.15}).ok);
        const LineGeometry vv{{0.0, 0.0}, {0.0, 1.0}};
        EDI_CHECK(!chamferLines(l1, vv, 0.0, 0.1, {0.5, 0.5}).ok);
        EDI_CHECK(!chamferLines(l1, vv, 0.1, -0.1, {0.5, 0.5}).ok);
        // A setback that reaches/overruns its line's far end rejects.
        const LineGeometry shortH{{0.0, 0.0}, {0.2, 0.0}};
        EDI_CHECK(!chamferLines(shortH, vv, 0.3, 0.1, {0.5, 0.5}).ok);
    }
    {
        // Near-0° wedge: the two lines meet at a vanishingly shallow angle (not
        // parallel — lineIntersection still finds a vertex), so the bevel collapses
        // to ~nothing. Reject "too shallow" rather than commit a degenerate bevel.
        const LineGeometry h{{0.0, 0.0}, {1.0, 0.0}};
        const LineGeometry almost{{0.0, 0.0}, {1.0, 0.0000004}}; // slope 4e-7
        const DraftingChamferResult r = chamferLines(h, almost, 0.3, 0.3, {0.5, 0.25});
        EDI_CHECK(!r.ok);
        EDI_CHECK(r.code == DraftingResultCode::InvalidGeometry);
        EDI_CHECK(r.message == "chamfer corner too shallow");
    }

    // --- extendLineToBoundary --------------------------------------------
    {
        // A short horizontal line extends its b-end (nearer the pick) to where its
        // extension meets a perpendicular boundary at x = 1.
        const LineGeometry target{{0.0, 0.0}, {0.5, 0.0}};
        const LineGeometry boundary{{1.0, -1.0}, {1.0, 1.0}};
        const DraftingExtendResult r = extendLineToBoundary(target, {boundary}, {0.6, 0.0});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.geometry.a, 0.0, 0.0)); // anchor end unchanged
        EDI_CHECK(pointNear(r.geometry.b, 1.0, 0.0)); // b extended to the boundary
    }
    {
        // Picking near the a-end extends the a-end to a boundary on that side.
        const LineGeometry target{{0.0, 0.0}, {0.5, 0.0}};
        const LineGeometry boundary{{-0.5, -1.0}, {-0.5, 1.0}};
        const DraftingExtendResult r = extendLineToBoundary(target, {boundary}, {-0.1, 0.0});
        EDI_CHECK(r.ok);
        EDI_CHECK(pointNear(r.geometry.a, -0.5, 0.0)); // a extended back to the boundary
        EDI_CHECK(pointNear(r.geometry.b, 0.5, 0.0));  // b unchanged
    }
    {
        // No boundary reaches the extension → reject. A parallel line never
        // crosses; a boundary only on the OTHER side of the picked end is skipped.
        const LineGeometry target{{0.0, 0.0}, {0.5, 0.0}};
        const LineGeometry parallel{{0.0, 0.5}, {1.0, 0.5}};
        EDI_CHECK(!extendLineToBoundary(target, {parallel}, {0.6, 0.0}).ok);
        // Boundary at x=1 is on the b-side; picking the a-end finds nothing.
        const LineGeometry boundary{{1.0, -1.0}, {1.0, 1.0}};
        EDI_CHECK(!extendLineToBoundary(target, {boundary}, {-0.1, 0.0}).ok);
    }

    // --- breakGeometryAtPoint --------------------------------------------
    {
        // A line broken at its midpoint → two half-lines sharing the split point.
        const DraftingBreakResult r = breakGeometryAtPoint(DraftingGeometry{LineGeometry{{0.0, 0.0}, {1.0, 0.0}}}, {0.5, 0.2});
        EDI_CHECK(r.ok);
        const auto *first = std::get_if<LineGeometry>(&r.first);
        const auto *second = std::get_if<LineGeometry>(&r.second);
        EDI_CHECK(first != nullptr && second != nullptr);
        EDI_CHECK(pointNear(first->a, 0.0, 0.0) && pointNear(first->b, 0.5, 0.0));
        EDI_CHECK(pointNear(second->a, 0.5, 0.0) && pointNear(second->b, 1.0, 0.0));
    }
    {
        // A polyline broken mid-segment splits the right segment, with the split
        // vertex INSERTED into BOTH halves.
        const PolylineGeometry source{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}};
        const DraftingBreakResult r = breakGeometryAtPoint(DraftingGeometry{source}, {1.0, 0.5});
        EDI_CHECK(r.ok);
        const auto *first = std::get_if<PolylineGeometry>(&r.first);
        const auto *second = std::get_if<PolylineGeometry>(&r.second);
        EDI_CHECK(first != nullptr && second != nullptr);
        // first = [(0,0),(1,0),split=(1,0.5)]; second = [split=(1,0.5),(1,1)].
        EDI_CHECK(first->vertices.size() == 3);
        EDI_CHECK(pointNear(first->vertices[0], 0.0, 0.0));
        EDI_CHECK(pointNear(first->vertices[1], 1.0, 0.0));
        EDI_CHECK(pointNear(first->vertices[2], 1.0, 0.5));
        EDI_CHECK(second->vertices.size() == 2);
        EDI_CHECK(pointNear(second->vertices[0], 1.0, 0.5)); // shared split vertex
        EDI_CHECK(pointNear(second->vertices[1], 1.0, 1.0));
    }
    {
        // A break too close to an endpoint rejects (one piece would be zero-length).
        const DraftingBreakResult r = breakGeometryAtPoint(DraftingGeometry{LineGeometry{{0.0, 0.0}, {1.0, 0.0}}}, {0.0, 0.0});
        EDI_CHECK(!r.ok);
        EDI_CHECK(r.code == DraftingResultCode::InvalidGeometry);
        // An unsupported kind rejects too.
        const DraftingBreakResult c = breakGeometryAtPoint(DraftingGeometry{CircleGeometry{{0.5, 0.5}, 0.2}}, {0.7, 0.5});
        EDI_CHECK(!c.ok);
        EDI_CHECK(c.code == DraftingResultCode::InvalidGeometry);
    }

    return 0;
}
