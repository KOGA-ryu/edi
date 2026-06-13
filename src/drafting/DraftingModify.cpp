#include "drafting/DraftingModify.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <utility>

namespace edi::drafting {

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

std::optional<Point2D> segmentIntersection(const LineGeometry &p, const LineGeometry &q)
{
    // Solve p.a + t*r = q.a + u*s for t, u (r, s the segment directions). The
    // 2D cross product of r and s is the determinant; zero means parallel or
    // collinear — no single crossing, so there is nothing to trim against.
    const double rX = p.b.x - p.a.x;
    const double rY = p.b.y - p.a.y;
    const double sX = q.b.x - q.a.x;
    const double sY = q.b.y - q.a.y;
    const double denominator = rX * sY - rY * sX;
    // The determinant equals |r||s|*sin(theta), so it shrinks with the segment
    // LENGTHS as well as the angle between them. An absolute threshold would
    // conflate "short" with "parallel" — two cleanly-perpendicular but tiny
    // segments have a tiny determinant yet a real crossing. Normalize by the
    // lengths so the test is on sin(theta): reject only a genuinely small angle.
    const double rLength = std::sqrt(rX * rX + rY * rY);
    const double sLength = std::sqrt(sX * sX + sY * sY);
    if (rLength == 0.0 || sLength == 0.0
        || std::abs(denominator) <= 1e-9 * rLength * sLength) {
        return std::nullopt; // a zero-length segment or an ~parallel/collinear pair
    }
    const double dX = q.a.x - p.a.x;
    const double dY = q.a.y - p.a.y;
    const double t = (dX * sY - dY * sX) / denominator; // position along p
    const double u = (dX * rY - dY * rX) / denominator; // position along q
    // The crossing of the two infinite lines must fall within BOTH finite
    // segments; otherwise the lines would only meet beyond an endpoint.
    if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0) {
        return std::nullopt;
    }
    return Point2D{p.a.x + t * rX, p.a.y + t * rY};
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

} // namespace edi::drafting
