#include "drafting/DraftingModify.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace edi::drafting {

namespace {

struct LineParams {
    double t; // position along the first line (p.a + t*(p.b-p.a))
    double u; // position along the second line
};

// Solve p.a + t*r = q.a + u*s for the two parameters. nullopt when the lines are
// ~parallel/collinear or a segment is degenerate. The determinant is
// |r||s|*sin(theta), so it shrinks with the segment LENGTHS as well as the
// angle; testing it against an ABSOLUTE threshold would mistake short segments
// for parallel ones. Normalising by the lengths makes the test on sin(theta) —
// a genuine small angle — independent of how long the segments are.
std::optional<LineParams> solveLineParams(const LineGeometry &p, const LineGeometry &q)
{
    const double rX = p.b.x - p.a.x;
    const double rY = p.b.y - p.a.y;
    const double sX = q.b.x - q.a.x;
    const double sY = q.b.y - q.a.y;
    const double denominator = rX * sY - rY * sX;
    const double rLength = std::sqrt(rX * rX + rY * rY);
    const double sLength = std::sqrt(sX * sX + sY * sY);
    if (rLength == 0.0 || sLength == 0.0
        || std::abs(denominator) <= 1e-9 * rLength * sLength) {
        return std::nullopt;
    }
    const double dX = q.a.x - p.a.x;
    const double dY = q.a.y - p.a.y;
    return LineParams{(dX * sY - dY * sX) / denominator, (dX * rY - dY * rX) / denominator};
}

Point2D pointAlong(const LineGeometry &line, double t)
{
    return {line.a.x + t * (line.b.x - line.a.x), line.a.y + t * (line.b.y - line.a.y)};
}

Point2D normalized(double x, double y)
{
    const double length = std::sqrt(x * x + y * y);
    return length == 0.0 ? Point2D{0.0, 0.0} : Point2D{x / length, y / length};
}

} // namespace

DraftingTrimResult DraftingTrimResult::accepted(LineGeometry geometry)
{
    DraftingTrimResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = geometry;
    return result;
}

DraftingTrimResult DraftingTrimResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingTrimResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::optional<Point2D> segmentIntersection(const LineGeometry &a, const LineGeometry &b)
{
    const std::optional<LineParams> params = solveLineParams(a, b);
    if (!params) {
        return std::nullopt; // a zero-length segment or an ~parallel/collinear pair
    }
    // The crossing of the two infinite lines must fall within BOTH finite
    // segments; otherwise the lines would only meet beyond an endpoint.
    if (params->t < 0.0 || params->t > 1.0 || params->u < 0.0 || params->u > 1.0) {
        return std::nullopt;
    }
    return pointAlong(a, params->t);
}

std::optional<Point2D> lineIntersection(const LineGeometry &a, const LineGeometry &b)
{
    const std::optional<LineParams> params = solveLineParams(a, b);
    if (!params) {
        return std::nullopt; // parallel/collinear or degenerate: no single vertex
    }
    return pointAlong(a, params->t); // no [0,1] clamp: the infinite lines' crossing
}

DraftingTrimResult trimLineAtPoint(const LineGeometry &target,
                                   const std::vector<LineGeometry> &boundaries,
                                   Point2D pick)
{
    // A zero-length target has no direction to project onto (and nothing to
    // trim); reject before the division below.
    const double dirX = target.b.x - target.a.x;
    const double dirY = target.b.y - target.a.y;
    const double lengthSq = dirX * dirX + dirY * dirY;
    if (lengthSq <= 0.000001 * 0.000001) {
        return DraftingTrimResult::rejected(DraftingResultCode::InvalidGeometry,
                                            "cannot trim a zero-length line");
    }

    // The cut is the boundary crossing nearest the click — the cut the user is
    // pointing at when several lines cross the target.
    std::optional<Point2D> cut;
    double bestDistance = 0.0;
    for (const LineGeometry &boundary : boundaries) {
        const std::optional<Point2D> crossing = segmentIntersection(target, boundary);
        if (!crossing) {
            continue;
        }
        const double distanceToPick = distance(*crossing, pick);
        if (!cut || distanceToPick < bestDistance) {
            cut = crossing;
            bestDistance = distanceToPick;
        }
    }
    if (!cut) {
        return DraftingTrimResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                            "no boundary line crosses the line to trim");
    }
    // Remove the piece the click fell on. The seam is the CUT, not the line's
    // midpoint: project the pick and the cut onto the line's direction and
    // compare those parameters. (Comparing the pick's distance to each ENDPOINT
    // instead would split at the midpoint, and whenever the cut and the midpoint
    // straddle the click it would delete the piece the user did NOT point at.)
    const auto projection = [&](Point2D point) {
        return ((point.x - target.a.x) * dirX + (point.y - target.a.y) * dirY) / lengthSq;
    };
    const LineGeometry trimmed = (projection(pick) < projection(*cut))
        ? LineGeometry{*cut, target.b}   // pick on the a-side of the cut: drop [a, cut]
        : LineGeometry{target.a, *cut};  // pick on the b-side of the cut: drop [cut, b]
    if (distance(trimmed.a, trimmed.b) <= 0.000001) {
        return DraftingTrimResult::rejected(DraftingResultCode::InvalidGeometry,
                                            "trim would collapse the line to zero length");
    }
    return DraftingTrimResult::accepted(trimmed);
}

DraftingFilletResult DraftingFilletResult::accepted(LineGeometry line1, LineGeometry line2, ArcGeometry arc)
{
    DraftingFilletResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.line1 = line1;
    result.line2 = line2;
    result.arc = arc;
    return result;
}

DraftingFilletResult DraftingFilletResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingFilletResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingFilletResult filletLines(const LineGeometry &line1, const LineGeometry &line2,
                                 double radius, Point2D pick)
{
    if (!std::isfinite(radius) || radius <= 0.0) {
        return DraftingFilletResult::rejected(DraftingResultCode::InvalidGeometry,
                                              "fillet radius must be positive");
    }
    // The corner vertex is where the two INFINITE lines meet — it can lie beyond
    // either segment (two lines that don't quite touch still have a corner).
    // Parallel/collinear lines have none, so there is nothing to round.
    const std::optional<Point2D> vertex = lineIntersection(line1, line2);
    if (!vertex) {
        return DraftingFilletResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                              "the two lines are parallel — no corner to fillet");
    }
    const Point2D V = *vertex;

    // Unit ray along each line from V, oriented toward the picked corner (the
    // side of V the click is on). These two rays bound the corner to round.
    const auto cornerRay = [&](const LineGeometry &line) {
        const Point2D dir = normalized(line.b.x - line.a.x, line.b.y - line.a.y);
        const double towardPick = (pick.x - V.x) * dir.x + (pick.y - V.y) * dir.y;
        return towardPick < 0.0 ? Point2D{-dir.x, -dir.y} : dir;
    };
    const Point2D d1 = cornerRay(line1);
    const Point2D d2 = cornerRay(line2);

    // theta = the corner angle between the rays. Guard BOTH ends: sin(theta/2)~0
    // is an ~0deg wedge (rays nearly equal), cos(theta/2)~0 is an ~180deg wedge
    // (rays nearly opposite, so the bisector d1+d2 collapses). Either is too
    // close to straight/parallel to admit a real fillet, and both make the
    // half-angle divisions below blow up.
    const double cosTheta = std::clamp(d1.x * d2.x + d1.y * d2.y, -1.0, 1.0);
    const double theta = std::acos(cosTheta);
    const double sinHalf = std::sin(theta / 2.0);
    const double cosHalf = std::cos(theta / 2.0);
    if (sinHalf < 1e-6 || cosHalf < 1e-6) {
        return DraftingFilletResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                              "the lines are too close to straight to fillet");
    }

    // In the right triangle V-T-O (right angle at the tangent point T): the leg
    // V->T is radius/tan(theta/2), the hypotenuse V->O is radius/sin(theta/2).
    const double toTangent = radius * (cosHalf / sinHalf);
    const double toCenter = radius / sinHalf;
    const Point2D bisector = normalized(d1.x + d2.x, d1.y + d2.y); // into the corner
    const Point2D center{V.x + bisector.x * toCenter, V.y + bisector.y * toCenter};
    const Point2D t1{V.x + d1.x * toTangent, V.y + d1.y * toTangent};
    const Point2D t2{V.x + d2.x * toTangent, V.y + d2.y * toTangent};

    // Each line keeps the endpoint FARTHER along its corner ray (the +d side, the
    // arm the pick is on) and moves the nearer endpoint to the tangent point. The
    // tangent (at distance toTangent from V along d) must lie BETWEEN V and that
    // kept endpoint: if toTangent overshoots the kept end, the line is too short
    // for this radius — there is no fillet, and moving the endpoint anyway would
    // emit a reversed phantom segment (the length-only guard misses it). Reject.
    const auto meetTangent = [&](const LineGeometry &line, Point2D dir, Point2D tangent)
        -> std::optional<LineGeometry> {
        const double projA = (line.a.x - V.x) * dir.x + (line.a.y - V.y) * dir.y;
        const double projB = (line.b.x - V.x) * dir.x + (line.b.y - V.y) * dir.y;
        const double keptProjection = std::max(projA, projB);
        if (toTangent >= keptProjection) {
            return std::nullopt; // tangent at or past the kept end: radius too large
        }
        return projA <= projB ? LineGeometry{tangent, line.b} : LineGeometry{line.a, tangent};
    };
    const std::optional<LineGeometry> newLine1 = meetTangent(line1, d1, t1);
    const std::optional<LineGeometry> newLine2 = meetTangent(line2, d2, t2);
    if (!newLine1 || !newLine2) {
        return DraftingFilletResult::rejected(DraftingResultCode::InvalidGeometry,
                                              "fillet radius is too large for these lines");
    }

    // The fillet is the MINOR arc from t1 to t2 about the centre (its central
    // angle is 180-theta < 180). Order start/end so the increasing sweep is that
    // minor arc, matching sampleArc's start->end (degrees, atan2 convention).
    constexpr double pi = 3.14159265358979323846;
    const double angle1 = std::atan2(t1.y - center.y, t1.x - center.x);
    const double angle2 = std::atan2(t2.y - center.y, t2.x - center.x);
    double sweep = angle2 - angle1;
    while (sweep <= -pi) sweep += 2.0 * pi;
    while (sweep > pi) sweep -= 2.0 * pi; // wrap into (-pi, pi]: the minor arc
    double startDeg;
    double endDeg;
    if (sweep >= 0.0) {
        startDeg = angle1 * 180.0 / pi;
        endDeg = (angle1 + sweep) * 180.0 / pi;
    } else {
        startDeg = angle2 * 180.0 / pi;
        endDeg = (angle2 - sweep) * 180.0 / pi;
    }
    const ArcGeometry arc{center, radius, startDeg, endDeg};
    return DraftingFilletResult::accepted(*newLine1, *newLine2, arc);
}

} // namespace edi::drafting
