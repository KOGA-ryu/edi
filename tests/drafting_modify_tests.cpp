#include "drafting/DraftingModify.h"

#include <cassert>
#include <cmath>
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
        assert(x && pointNear(*x, 0.7, 0.5));

        // Parallel lines never cross.
        const LineGeometry parallel{{0.0, 0.6}, {1.0, 0.6}};
        assert(!segmentIntersection(horizontal, parallel));

        // Collinear (overlapping) is treated as no single crossing.
        const LineGeometry collinear{{0.2, 0.5}, {0.4, 0.5}};
        assert(!segmentIntersection(horizontal, collinear));

        // The infinite lines would cross, but off the segments: reject.
        const LineGeometry shortVertical{{0.7, 0.8}, {0.7, 0.9}}; // above the target
        assert(!segmentIntersection(horizontal, shortVertical));

        // A diagonal crossing, verified against the parametric point.
        const LineGeometry a{{0.0, 0.0}, {1.0, 1.0}};
        const LineGeometry b{{0.0, 1.0}, {1.0, 0.0}};
        const auto mid = segmentIntersection(a, b);
        assert(mid && pointNear(*mid, 0.5, 0.5));

        // SHORT but cleanly perpendicular segments cross — "parallel" must mean
        // a small ANGLE, not a small determinant. With an absolute determinant
        // threshold these (length ~5e-7) would be falsely rejected as parallel.
        const LineGeometry tinyH{{0.0, 0.0}, {5e-7, 0.0}};
        const LineGeometry tinyV{{2.5e-7, -2.5e-7}, {2.5e-7, 2.5e-7}};
        const auto tiny = segmentIntersection(tinyH, tinyV);
        assert(tiny && pointNear(*tiny, 2.5e-7, 0.0, 1e-12));
    }

    // --- trimLineAtPoint -------------------------------------------------
    const LineGeometry target{{0.0, 0.5}, {1.0, 0.5}};
    const LineGeometry cutAt07{{0.7, 0.0}, {0.7, 1.0}};

    {
        // Click the RIGHT stub (near b): the b end moves to the cut, keeping a..X.
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt07}, {0.9, 0.5});
        assert(r.ok);
        assert(pointNear(r.geometry.a, 0.0, 0.5)); // a kept
        assert(pointNear(r.geometry.b, 0.7, 0.5)); // b pulled to the crossing
    }
    {
        // Click the LEFT stub (near a): the a end moves to the cut, keeping X..b.
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt07}, {0.1, 0.5});
        assert(r.ok);
        assert(pointNear(r.geometry.a, 0.7, 0.5)); // a pulled to the crossing
        assert(pointNear(r.geometry.b, 1.0, 0.5)); // b kept
    }
    {
        // The doomed side is chosen by the CUT, not the midpoint. Cut at 0.8,
        // click at 0.7 (in the LARGE left piece [0,0.8]): that piece is removed,
        // keeping the short right stub. A midpoint split would wrongly keep the
        // clicked piece (0.7 is left of the 0.5 midpoint -> would drop the right).
        const LineGeometry cutAt08{{0.8, 0.0}, {0.8, 1.0}};
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt08}, {0.7, 0.5});
        assert(r.ok);
        assert(pointNear(r.geometry.a, 0.8, 0.5)); // a pulled up to the cut
        assert(pointNear(r.geometry.b, 1.0, 0.5)); // the stub the user did NOT click stays
    }
    {
        // Two boundaries: the crossing NEAREST the click is the cut. Clicking
        // far right (0.95) past the 0.7 cut trims only the stub beyond 0.7.
        const LineGeometry cutAt03{{0.3, 0.0}, {0.3, 1.0}};
        const DraftingTrimResult r = trimLineAtPoint(target, {cutAt03, cutAt07}, {0.95, 0.5});
        assert(r.ok);
        assert(pointNear(r.geometry.a, 0.0, 0.5));
        assert(pointNear(r.geometry.b, 0.7, 0.5)); // nearest cut to the click wins
    }
    {
        // No boundary crosses the target: reject with a reason.
        const LineGeometry parallel{{0.0, 0.2}, {1.0, 0.2}};
        const DraftingTrimResult r = trimLineAtPoint(target, {parallel}, {0.5, 0.5});
        assert(!r.ok && r.code == DraftingResultCode::InvalidSelectionTarget);
    }
    {
        // An empty boundary set cannot trim.
        assert(!trimLineAtPoint(target, {}, {0.5, 0.5}).ok);
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
        assert(!r.ok && r.code == DraftingResultCode::InvalidGeometry);
    }

    return 0;
}
