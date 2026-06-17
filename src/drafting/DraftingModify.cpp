#include "drafting/DraftingModify.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace edi::drafting {

namespace {

// Unit vector (zero stays zero). Local to the fillet's bisector/ray math; the
// intersection primitives it once shared this namespace with now live in
// DraftingGeometry (a second, unrelated consumer — the snap engine — appeared,
// so they were promoted from "modify" to the shared geometry home).
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

DraftingChamferResult DraftingChamferResult::accepted(LineGeometry line1, LineGeometry line2, LineGeometry bevel)
{
    DraftingChamferResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.line1 = line1;
    result.line2 = line2;
    result.bevel = bevel;
    return result;
}

DraftingChamferResult DraftingChamferResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingChamferResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingChamferResult chamferLines(const LineGeometry &line1, const LineGeometry &line2,
                                   double setback1, double setback2, Point2D pick)
{
    if (!std::isfinite(setback1) || setback1 <= 0.0 || !std::isfinite(setback2) || setback2 <= 0.0) {
        return DraftingChamferResult::rejected(DraftingResultCode::InvalidGeometry,
                                               "chamfer setback must be positive");
    }
    // Same corner-from-pick machinery as filletLines (mirrored here rather than
    // extracted, to leave the fillet path byte-for-byte unchanged): the corner is
    // where the two INFINITE lines meet; parallel/collinear pairs have none.
    const std::optional<Point2D> vertex = lineIntersection(line1, line2);
    if (!vertex) {
        return DraftingChamferResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                               "the two lines are parallel — no corner to chamfer");
    }
    const Point2D V = *vertex;

    // Unit ray along each line from V, oriented toward the picked corner — the same
    // selection filletLines uses (the side of V the click is on).
    const auto cornerRay = [&](const LineGeometry &line) {
        const Point2D dir = normalized(line.b.x - line.a.x, line.b.y - line.a.y);
        const double towardPick = (pick.x - V.x) * dir.x + (pick.y - V.y) * dir.y;
        return towardPick < 0.0 ? Point2D{-dir.x, -dir.y} : dir;
    };
    const Point2D d1 = cornerRay(line1);
    const Point2D d2 = cornerRay(line2);

    // The setback points: `setback` along each corner ray from V.
    const Point2D s1{V.x + d1.x * setback1, V.y + d1.y * setback1};
    const Point2D s2{V.x + d2.x * setback2, V.y + d2.y * setback2};

    // A near-degenerate corner — the two corner rays almost coincide (a ~0° wedge),
    // which lineIntersection's far looser parallel test still lets through — would
    // collapse the bevel to ~nothing. Reject rather than emit a degenerate bevel:
    // the controller commits the CreateObject without checking the geometry, so a
    // zero-length bevel would otherwise land as a partial, malformed chamfer.
    if (distance(s1, s2) <= 0.000001) {
        return DraftingChamferResult::rejected(DraftingResultCode::InvalidGeometry,
                                               "chamfer corner too shallow");
    }

    // Each line keeps the endpoint FARTHER along its corner ray and moves the
    // nearer endpoint to the setback point — the same trim filletLines does for the
    // tangent point. If the setback reaches or passes the kept end the line is too
    // short, so reject (mirrors the fillet "radius too large" guard, which also
    // stops a reversed phantom segment the length check alone would miss).
    const auto meetSetback = [&](const LineGeometry &line, Point2D dir, Point2D setbackPoint, double setback)
        -> std::optional<LineGeometry> {
        const double projA = (line.a.x - V.x) * dir.x + (line.a.y - V.y) * dir.y;
        const double projB = (line.b.x - V.x) * dir.x + (line.b.y - V.y) * dir.y;
        const double keptProjection = std::max(projA, projB);
        if (setback >= keptProjection) {
            return std::nullopt; // setback at or past the kept end: too large
        }
        return projA <= projB ? LineGeometry{setbackPoint, line.b} : LineGeometry{line.a, setbackPoint};
    };
    const std::optional<LineGeometry> newLine1 = meetSetback(line1, d1, s1, setback1);
    const std::optional<LineGeometry> newLine2 = meetSetback(line2, d2, s2, setback2);
    if (!newLine1 || !newLine2) {
        return DraftingChamferResult::rejected(DraftingResultCode::InvalidGeometry,
                                               "chamfer setback is too large for these lines");
    }

    return DraftingChamferResult::accepted(*newLine1, *newLine2, LineGeometry{s1, s2});
}

DraftingExtendResult DraftingExtendResult::accepted(LineGeometry geometry)
{
    DraftingExtendResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = geometry;
    return result;
}

DraftingExtendResult DraftingExtendResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingExtendResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingExtendResult extendLineToBoundary(const LineGeometry &target,
                                          const std::vector<LineGeometry> &boundaries,
                                          Point2D pick)
{
    // A zero-length target has no direction to extend along.
    const double dirX = target.b.x - target.a.x;
    const double dirY = target.b.y - target.a.y;
    const double lengthSq = dirX * dirX + dirY * dirY;
    if (lengthSq <= 0.000001 * 0.000001) {
        return DraftingExtendResult::rejected(DraftingResultCode::InvalidGeometry,
                                              "cannot extend a zero-length line");
    }

    // Extend the END nearer the pick — the mirror of trim removing the picked end.
    const bool extendB = distance(target.b, pick) <= distance(target.a, pick);
    const auto projection = [&](Point2D point) {
        return ((point.x - target.a.x) * dirX + (point.y - target.a.y) * dirY) / lengthSq;
    };
    // The crossing must lie ON the boundary segment (we extend to the real edge).
    const auto onSegment = [](const LineGeometry &segment, Point2D point) {
        const double sx = segment.b.x - segment.a.x;
        const double sy = segment.b.y - segment.a.y;
        const double len2 = sx * sx + sy * sy;
        if (len2 <= 0.0) {
            return false;
        }
        const double u = ((point.x - segment.a.x) * sx + (point.y - segment.a.y) * sy) / len2;
        return u >= -0.000001 && u <= 1.0 + 0.000001;
    };

    // The new endpoint is the boundary crossing nearest the click, among those that
    // lie BEYOND the extended end (an extension, not a point already on the segment)
    // — mirroring trim's nearest-crossing-wins selection.
    std::optional<Point2D> best;
    double bestDistance = 0.0;
    for (const LineGeometry &boundary : boundaries) {
        const std::optional<Point2D> crossing = lineIntersection(target, boundary);
        if (!crossing || !onSegment(boundary, *crossing)) {
            continue;
        }
        const double t = projection(*crossing);
        const bool beyondEnd = extendB ? (t > 1.0) : (t < 0.0);
        if (!beyondEnd) {
            continue; // the crossing is not past the end being extended
        }
        const double distanceToPick = distance(*crossing, pick);
        if (!best || distanceToPick < bestDistance) {
            best = crossing;
            bestDistance = distanceToPick;
        }
    }
    if (!best) {
        return DraftingExtendResult::rejected(DraftingResultCode::InvalidSelectionTarget,
                                              "no boundary crosses the line's extension");
    }
    const LineGeometry extended = extendB ? LineGeometry{target.a, *best} : LineGeometry{*best, target.b};
    return DraftingExtendResult::accepted(extended);
}

} // namespace edi::drafting
