#include "drafting/DraftingDimensionOps.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingDimensionPlan DraftingDimensionPlan::accepted(DimensionGeometry geometry)
{
    DraftingDimensionPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = geometry;
    return result;
}

DraftingDimensionPlan DraftingDimensionPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingDimensionPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::optional<DimensionKind> draftingDimensionKindFromId(const std::string &kindId)
{
    if (kindId == "distance") {
        return DimensionKind::Distance;
    }
    if (kindId == "width") {
        return DimensionKind::Width;
    }
    if (kindId == "height") {
        return DimensionKind::Height;
    }
    if (kindId == "radius") {
        return DimensionKind::Radius;
    }
    if (kindId == "diameter") {
        return DimensionKind::Diameter;
    }
    if (kindId == "angular") {
        return DimensionKind::Angular;
    }
    return std::nullopt;
}

DraftingDimensionPlan planDimensionKindChange(const DimensionGeometry &dimension, DimensionKind kind)
{
    // Angular is a STRUCTURAL change: it needs a vertex + two rays that a linear
    // dimension does not carry, and `offset` is repurposed from a standoff length
    // to an included angle in degrees. There is no lossless mapping from a/b/offset
    // in the linear convention to the angular convention, so this conversion is
    // deliberately rejected. The user creates an Angular dimension from two lines
    // via planAngularDimension instead of cycling an existing dim to Angular.
    if (kind == DimensionKind::Angular) {
        return DraftingDimensionPlan::rejected(DraftingResultCode::InvalidSelectionTarget,
            "cannot convert an existing dimension to Angular — use planAngularDimension with two lines");
    }

    const double currentLength = distance(dimension.a, dimension.b);
    if (!std::isfinite(currentLength) || currentLength <= 0.000001) {
        return DraftingDimensionPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension requires two distinct finite points");
    }

    DimensionGeometry next = dimension;
    next.kind = kind;
    if (next.kind == DimensionKind::Width) {
        const double sign = next.b.x < next.a.x ? -1.0 : 1.0;
        next.b = {next.a.x + sign * currentLength, next.a.y};
    } else if (next.kind == DimensionKind::Height) {
        const double sign = next.b.y < next.a.y ? -1.0 : 1.0;
        next.b = {next.a.x, next.a.y + sign * currentLength};
    }

    return DraftingDimensionPlan::accepted(next);
}

DraftingDimensionPlan planAngularDimension(const LineGeometry &l1, const LineGeometry &l2)
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kEpsilon = 1e-10;

    // The vertex is where the two infinite lines meet.  lineIntersection returns
    // nullopt for parallel/collinear pairs — we surface that as a user-facing reject.
    const std::optional<Point2D> V = lineIntersection(l1, l2);
    if (!V) {
        return DraftingDimensionPlan::rejected(DraftingResultCode::InvalidSelectionTarget,
            "lines are parallel — no angular dimension vertex");
    }

    // ray1 direction: the unit vector from l1.a toward l1.b.
    const double l1dx = l1.b.x - l1.a.x;
    const double l1dy = l1.b.y - l1.a.y;
    const double l1len = std::hypot(l1dx, l1dy);
    if (l1len < kEpsilon) {
        return DraftingDimensionPlan::rejected(DraftingResultCode::InvalidSelectionTarget,
            "first line is degenerate (zero length)");
    }
    const double dir1x = l1dx / l1len;
    const double dir1y = l1dy / l1len;

    // ray2 direction: the unit vector from l2.a toward l2.b.
    const double l2dx = l2.b.x - l2.a.x;
    const double l2dy = l2.b.y - l2.a.y;
    const double l2len = std::hypot(l2dx, l2dy);
    if (l2len < kEpsilon) {
        return DraftingDimensionPlan::rejected(DraftingResultCode::InvalidSelectionTarget,
            "second line is degenerate (zero length)");
    }
    const double dir2x = l2dx / l2len;
    const double dir2y = l2dy / l2len;

    // Signed angle from ray1 to ray2 via the 2D cross and dot products.
    // cross = dir1 × dir2 (z-component); positive means ray2 is CCW from ray1.
    // dot   = dir1 · dir2; together with cross, atan2 gives the full ±180° range.
    const double cross = dir1x * dir2y - dir1y * dir2x;
    const double dot   = dir1x * dir2x + dir1y * dir2y;
    const double angleDeg = std::atan2(cross, dot) * 180.0 / kPi;

    // Encode in the settled Candidate A representation:
    //   a      = vertex V
    //   b      = V + dir1 * kDefaultAngularArc   (ray1 tip; user can drag to resize)
    //   offset = signed included angle in degrees (ray1 → ray2)
    DimensionGeometry geo;
    geo.kind   = DimensionKind::Angular;
    geo.a      = *V;
    geo.b      = {V->x + dir1x * kDefaultAngularArc, V->y + dir1y * kDefaultAngularArc};
    geo.offset = angleDeg;
    return DraftingDimensionPlan::accepted(geo);
}

} // namespace edi::drafting
