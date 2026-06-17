#include "drafting/DraftingDerived.h"

#include "drafting/DraftingGeometry.h" // distance(), isFinite()

#include <cmath>
#include <utility>

namespace edi::drafting {
namespace {

// The circumcenter determinant `d` is 4× the signed area of triangle abc, so it
// vanishes exactly when the three points are collinear. A small epsilon treats a
// near-degenerate (almost-collinear) triple — whose circumcircle would balloon to
// a near-infinite radius — as "no circle" too. Coordinates live in the normalized
// 0..1 canvas space, so a genuine triangle's `d` is O(1) and 1e-9 is safely below
// any real construction while still catching the degenerate case.
constexpr double kDegeneracyEpsilon = 0.000000001;

} // namespace

DraftingDerivedCircleResult DraftingDerivedCircleResult::accepted(CircleGeometry geometry)
{
    DraftingDerivedCircleResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = std::move(geometry);
    return result;
}

DraftingDerivedCircleResult DraftingDerivedCircleResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingDerivedCircleResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingDerivedCircleResult circleThroughThreePoints(Point2D a, Point2D b, Point2D c)
{
    if (!isFinite(a) || !isFinite(b) || !isFinite(c)) {
        return DraftingDerivedCircleResult::rejected(DraftingResultCode::InvalidGeometry, "points must be finite");
    }

    // Circumcenter by the determinant form (equivalent to intersecting two
    // perpendicular bisectors, but branch-free): with d = 2·(ax(by−cy) +
    // bx(cy−ay) + cx(ay−by)) the center is the ratio of two more determinants.
    const double d = 2.0 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::abs(d) <= kDegeneracyEpsilon) {
        return DraftingDerivedCircleResult::rejected(DraftingResultCode::InvalidGeometry, "points are collinear — no unique circle");
    }

    const double a2 = a.x * a.x + a.y * a.y;
    const double b2 = b.x * b.x + b.y * b.y;
    const double c2 = c.x * c.x + c.y * c.y;
    const Point2D center{
        (a2 * (b.y - c.y) + b2 * (c.y - a.y) + c2 * (a.y - b.y)) / d,
        (a2 * (c.x - b.x) + b2 * (a.x - c.x) + c2 * (b.x - a.x)) / d,
    };
    const double radius = distance(center, a); // any of the three points is equidistant
    return DraftingDerivedCircleResult::accepted(CircleGeometry{center, radius});
}

DraftingDerivedCircleResult circleThroughTwoPoints(Point2D a, Point2D b)
{
    if (!isFinite(a) || !isFinite(b)) {
        return DraftingDerivedCircleResult::rejected(DraftingResultCode::InvalidGeometry, "points must be finite");
    }
    const double diameter = distance(a, b);
    if (diameter <= kDegeneracyEpsilon) {
        return DraftingDerivedCircleResult::rejected(DraftingResultCode::InvalidGeometry, "points are coincident — zero-diameter circle");
    }
    return DraftingDerivedCircleResult::accepted(CircleGeometry{
        {(a.x + b.x) / 2.0, (a.y + b.y) / 2.0},
        diameter / 2.0,
    });
}

} // namespace edi::drafting
