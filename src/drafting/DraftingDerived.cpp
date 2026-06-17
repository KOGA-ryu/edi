#include "drafting/DraftingDerived.h"

#include "drafting/DraftingGeometry.h" // distance(), isFinite(), arcPointAtAngle()

#include <cmath>
#include <numeric> // std::gcd
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

// A circle the inscribe ops can actually draw on: finite center, finite positive
// radius, finite start angle. Degenerate circles would yield coincident/NaN
// vertices, so they reject rather than emit garbage geometry.
bool inscribableCircle(const CircleGeometry &circle, double startAngleDeg)
{
    return isFinite(circle.center)
        && std::isfinite(circle.radius) && circle.radius > 0.0
        && std::isfinite(startAngleDeg);
}

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

DraftingInscribedResult DraftingInscribedResult::accepted(PolygonGeometry geometry)
{
    DraftingInscribedResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = std::move(geometry);
    return result;
}

DraftingInscribedResult DraftingInscribedResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingInscribedResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::vector<Point2D> divideCirclePoints(const CircleGeometry &circle, int n, double startAngleDeg)
{
    std::vector<Point2D> points;
    if (n < 2) {
        return points;
    }
    points.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double angle = startAngleDeg + static_cast<double>(i) * 360.0 / static_cast<double>(n);
        // arcPointAtAngle owns the trig (the model's angle convention) — no re-derive.
        points.push_back(arcPointAtAngle(circle.center, circle.radius, angle));
    }
    return points;
}

DraftingInscribedResult inscribeRegularPolygon(const CircleGeometry &circle, int n, double startAngleDeg)
{
    if (n < 3) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "a polygon needs at least 3 vertices");
    }
    if (!inscribableCircle(circle, startAngleDeg)) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "circle must have a positive finite radius");
    }
    PolygonGeometry geometry;
    geometry.vertices = divideCirclePoints(circle, n, startAngleDeg);
    return DraftingInscribedResult::accepted(std::move(geometry));
}

DraftingInscribedResult inscribeStarPolygon(const CircleGeometry &circle, int n, int step, double startAngleDeg)
{
    // {n/step} validity. A star is ONE closed path only when the every-step-th walk
    // visits all n points before returning to the start — i.e. when step and n are
    // coprime. If gcd(n, step) = g > 1 the walk closes after n/g points, leaving a
    // disjoint compound (e.g. {6/2} = two triangles) that no single PolygonGeometry
    // can hold. The other bounds remove the non-star degeneracies: step 1 is the
    // regular polygon, step >= n/2 just re-traces the complementary star, and n < 5
    // has no proper star at all.
    if (n < 5) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "a star polygon needs at least 5 points");
    }
    if (step < 2) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "star step must be at least 2 (step 1 is the regular polygon)");
    }
    if (step * 2 >= n) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "star step must be less than n/2 (a larger step retraces the complementary star)");
    }
    if (std::gcd(n, step) != 1) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "{n/step} is a compound star (gcd(n, step) != 1) — not a single closed path");
    }
    if (!inscribableCircle(circle, startAngleDeg)) {
        return DraftingInscribedResult::rejected(DraftingResultCode::InvalidGeometry, "circle must have a positive finite radius");
    }

    const std::vector<Point2D> points = divideCirclePoints(circle, n, startAngleDeg);
    PolygonGeometry geometry;
    geometry.vertices.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        geometry.vertices.push_back(points[static_cast<std::size_t>((i * step) % n)]);
    }
    return DraftingInscribedResult::accepted(std::move(geometry));
}

} // namespace edi::drafting
