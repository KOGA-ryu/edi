#include "drafting/DraftingArray.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingSnap.h"

#include <cmath>
#include <unordered_set>
#include <utility>
#include <variant>

namespace edi::drafting {

namespace {

constexpr double kSpacingEpsilon = 0.000001;

// Every array planner ends the same way: clone the source per offset, re-id,
// translate, stamp provenance, validate the copy, recompute bounds. The
// planners differ only in the offset sequence they produce — that difference
// is data (a vector of deltas), so the shared mechanics live here once
// instead of being re-inlined per planner and drifting apart.
DraftingArrayResult buildTranslatedCopies(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    const std::vector<Point2D> &offsets,
    const char *provenance)
{
    if (!kindMatchesGeometry(source.kind, source.geometry)) {
        return DraftingArrayResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    std::unordered_set<DraftingObjectId> usedIds;
    usedIds.reserve(newObjectIds.size());
    for (const DraftingObjectId &id : newObjectIds) {
        if (id.empty() || id == source.id || !usedIds.insert(id).second) {
            return DraftingArrayResult::rejected(DraftingResultCode::DuplicateObjectId, "array object ids must be unique");
        }
    }

    std::vector<DraftingObject> copies;
    copies.reserve(newObjectIds.size());
    for (std::size_t index = 0; index < newObjectIds.size(); ++index) {
        DraftingObject object = source;
        object.id = newObjectIds[index];
        object.geometry = translateGeometry(source.geometry, offsets[index].x, offsets[index].y);
        object.metadata.toolProvenance = provenance;
        object.metadata.source = source.id;

        const auto validation = validateDraftingObjectShape(object);
        if (!validation.ok) {
            return DraftingArrayResult::rejected(validation.code, validation.message);
        }
        object.bounds = computeBounds(object.geometry);
        copies.push_back(std::move(object));
    }

    return DraftingArrayResult::accepted(std::move(copies));
}

// The rotating sibling of buildTranslatedCopies: each copy is TRANSFORMED (rotated
// about `center` by its spoke angle, scale 1.0) rather than merely translated, so
// the geometry itself turns with the fan. Same clone/re-id/validate/bounds
// mechanics; the per-copy difference is data (a vector of angles in degrees).
DraftingArrayResult buildRotatedCopies(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    Point2D center,
    const std::vector<double> &anglesDeg,
    const char *provenance)
{
    if (!kindMatchesGeometry(source.kind, source.geometry)) {
        return DraftingArrayResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    std::unordered_set<DraftingObjectId> usedIds;
    usedIds.reserve(newObjectIds.size());
    for (const DraftingObjectId &id : newObjectIds) {
        if (id.empty() || id == source.id || !usedIds.insert(id).second) {
            return DraftingArrayResult::rejected(DraftingResultCode::DuplicateObjectId, "array object ids must be unique");
        }
    }

    std::vector<DraftingObject> copies;
    copies.reserve(newObjectIds.size());
    for (std::size_t index = 0; index < newObjectIds.size(); ++index) {
        DraftingObject object = source;
        object.id = newObjectIds[index];
        // transformGeometry rotates EVERY point about center, so the copy both
        // orbits onto its spoke (its bounds centre lands where the radial array
        // would place it) AND rotates in place (a rectangle's rotationDeg += angle).
        object.geometry = transformGeometry(source.geometry, center, anglesDeg[index], 1.0);
        object.metadata.toolProvenance = provenance;
        object.metadata.source = source.id;

        const auto validation = validateDraftingObjectShape(object);
        if (!validation.ok) {
            return DraftingArrayResult::rejected(validation.code, validation.message);
        }
        object.bounds = computeBounds(object.geometry);
        copies.push_back(std::move(object));
    }

    return DraftingArrayResult::accepted(std::move(copies));
}

// ---- helpers for arrayAlongCurve ----------------------------------------

constexpr double kPathPi = 3.14159265358979323846;
// Slack for "d fits within total length" comparisons — identical in spirit to
// kSpacingEpsilon but named for its distinct use (path overshoot guard).
constexpr double kPathEpsilon = 1e-9;

// Walk `segments` (an ordered list of points, each consecutive pair is an edge)
// and return the point at arc-length `d`. Returns the last point on overshoot
// instead of nullopt, because the caller already computed `d ≤ totalLength`;
// floating-point rounding can push `d` just past the edge of the last segment.
Point2D walkPath(const std::vector<Point2D> &pts, double d)
{
    if (pts.empty()) {
        return {};
    }
    if (d <= kPathEpsilon) {
        return pts.front();
    }
    double remaining = d;
    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        const double seg = distance(pts[i], pts[i + 1]);
        if (seg <= 0.0) {
            continue; // zero-length edge: skip, don't subtract
        }
        if (remaining <= seg + kPathEpsilon) {
            const double t = std::min(remaining / seg, 1.0);
            return {pts[i].x + t * (pts[i + 1].x - pts[i].x),
                    pts[i].y + t * (pts[i + 1].y - pts[i].y)};
        }
        remaining -= seg;
    }
    return pts.back(); // clamp to end on floating-point overshoot
}

// Total arc-length of the supported path kinds. Fills `splineSamples` (via
// sampleSpline) when the path is a Spline so the caller reuses it for station
// and tangent queries without re-sampling. Returns -1 for unsupported kinds.
double curveArcLength(const DraftingGeometry &path, std::vector<Point2D> &splineSamples)
{
    if (const auto *line = std::get_if<LineGeometry>(&path)) {
        return distance(line->a, line->b);
    }
    if (const auto *poly = std::get_if<PolylineGeometry>(&path)) {
        double len = 0.0;
        for (std::size_t i = 0; i + 1 < poly->vertices.size(); ++i) {
            len += distance(poly->vertices[i], poly->vertices[i + 1]);
        }
        return len;
    }
    if (const auto *arc = std::get_if<ArcGeometry>(&path)) {
        // Arc length = radius × |sweep in radians|.  The sign of the sweep
        // tells us direction (CW vs CCW) but length is always positive.
        const double sweepRad = (arc->endAngleDeg - arc->startAngleDeg) * kPathPi / 180.0;
        return arc->radius * std::abs(sweepRad);
    }
    if (const auto *spline = std::get_if<SplineGeometry>(&path)) {
        // Catmull-Rom spline: no closed-form arc-length, so approximate via
        // sampleSpline (shared tessellation helper, stepsPerSegment=16 default).
        splineSamples = sampleSpline(*spline);
        double len = 0.0;
        for (std::size_t i = 0; i + 1 < splineSamples.size(); ++i) {
            len += distance(splineSamples[i], splineSamples[i + 1]);
        }
        return len;
    }
    return -1.0; // Circle, Rectangle, Guide, … → unsupported
}

// The point at arc-length `d` along the supported path.
// For Line / Polyline / Arc, delegates to pointAlongEntity (the DR-04 helper)
// which already owns the per-kind walking logic; any floating-point overshoot
// beyond L is clamped by the caller before we arrive here, so the result is
// always ok.  For Spline, walks the pre-sampled polyline.
Point2D curveStationPoint(const DraftingGeometry &path,
                          double d,
                          const std::vector<Point2D> &splineSamples)
{
    if (!std::holds_alternative<SplineGeometry>(path)) {
        const DraftingPointAlongResult r = pointAlongEntity(path, d, /*fromEnd=*/false);
        if (r.ok) {
            return r.point;
        }
        // Should not happen after clamping — fall through to the walk-path
        // fallback for robustness (d might be at the floating-point edge of L).
    }
    return walkPath(splineSamples, d);
}

// Tangent direction angle (degrees, atan2 convention) at arc-length `d`.
// — Line:     constant; the line's own direction.
// — Polyline: the direction of whichever segment contains d.
// — Arc:      the tangent is perpendicular to the radius.  The point on the arc
//             at distance d has angle `startAngle + direction * (d/r * 180/π)`;
//             the tangent is that angle ± 90° (+ for positive sweep, − for
//             negative).  This derivation: the arc is parameterised by θ, so
//             dP/dθ = (−r sin θ, r cos θ); atan2 of that vector is θ + 90°.
// — Spline:   finite-difference between adjacent samples (same segment walk as
//             Polyline, applied to the pre-sampled point list).
double curveTangentDeg(const DraftingGeometry &path,
                       double d,
                       const std::vector<Point2D> &splineSamples)
{
    if (const auto *line = std::get_if<LineGeometry>(&path)) {
        return lineAngleDegrees(*line); // same for every station on a line
    }
    if (const auto *poly = std::get_if<PolylineGeometry>(&path)) {
        double remaining = d;
        for (std::size_t i = 0; i + 1 < poly->vertices.size(); ++i) {
            const double seg = distance(poly->vertices[i], poly->vertices[i + 1]);
            if (seg <= 0.0) {
                continue;
            }
            if (remaining <= seg + kPathEpsilon) {
                return lineAngleDegrees(LineGeometry{poly->vertices[i], poly->vertices[i + 1]});
            }
            remaining -= seg;
        }
        // d is at or past the last vertex: use the last segment's direction.
        const auto &vs = poly->vertices;
        if (vs.size() >= 2) {
            return lineAngleDegrees(LineGeometry{vs[vs.size() - 2], vs[vs.size() - 1]});
        }
        return 0.0;
    }
    if (const auto *arc = std::get_if<ArcGeometry>(&path)) {
        // Positive sweep (endAngle > startAngle): tangent = pointAngle + 90°.
        // Negative sweep: tangent = pointAngle − 90°.
        const double direction = arc->endAngleDeg >= arc->startAngleDeg ? 1.0 : -1.0;
        const double pointAngleDeg = arc->startAngleDeg + direction * (d / arc->radius) * 180.0 / kPathPi;
        return pointAngleDeg + direction * 90.0;
    }
    // Spline: walk the pre-sampled polyline, return the containing segment's angle.
    {
        double remaining = d;
        for (std::size_t i = 0; i + 1 < splineSamples.size(); ++i) {
            const double seg = distance(splineSamples[i], splineSamples[i + 1]);
            if (seg <= 0.0) {
                continue;
            }
            if (remaining <= seg + kPathEpsilon) {
                return lineAngleDegrees(LineGeometry{splineSamples[i], splineSamples[i + 1]});
            }
            remaining -= seg;
        }
        if (splineSamples.size() >= 2) {
            const auto &ss = splineSamples;
            return lineAngleDegrees(LineGeometry{ss[ss.size() - 2], ss[ss.size() - 1]});
        }
        return 0.0;
    }
}

} // namespace

DraftingArrayResult DraftingArrayResult::accepted(std::vector<DraftingObject> objects)
{
    DraftingArrayResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.objects = std::move(objects);
    return result;
}

DraftingArrayResult DraftingArrayResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingArrayResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::optional<DraftingArrayRepeatSettings> draftingArrayRepeatSettingsFromAxisId(
    const std::string &axisId,
    int copyCount,
    double spacingX,
    double spacingY)
{
    if (axisId == "x") {
        return DraftingArrayRepeatSettings{copyCount, spacingX, 0.0};
    }
    if (axisId == "y") {
        return DraftingArrayRepeatSettings{copyCount, 0.0, spacingY};
    }
    return std::nullopt;
}

DraftingArrayResult repeatDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    double spacingX,
    double spacingY)
{
    if (newObjectIds.empty()) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "repeat requires at least one copy");
    }
    if (!std::isfinite(spacingX) || !std::isfinite(spacingY)) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "repeat spacing must be finite");
    }
    if (std::abs(spacingX) <= kSpacingEpsilon && std::abs(spacingY) <= kSpacingEpsilon) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "repeat spacing must move copied objects");
    }

    std::vector<Point2D> offsets;
    offsets.reserve(newObjectIds.size());
    for (std::size_t index = 0; index < newObjectIds.size(); ++index) {
        const double step = static_cast<double>(index + 1);
        offsets.push_back({spacingX * step, spacingY * step});
    }
    return buildTranslatedCopies(source, newObjectIds, offsets, "repeat");
}

DraftingArrayResult gridArrayDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    int columns,
    int rows,
    double spacingX,
    double spacingY)
{
    if (columns < 1 || rows < 1) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array needs at least one column and one row");
    }
    if (std::holds_alternative<GuideGeometry>(source.geometry)) {
        // A guide translates on one axis only (translateGeometry discards the
        // perpendicular component), so 2D placements would stack exact
        // duplicates on the source — reject instead of silently coinciding.
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array does not support guides");
    }
    // Multiply in size_t: two large-but-valid ints could overflow a signed
    // int product (UB) before the guard even runs; size_t cannot.
    const std::size_t cells = static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows);
    if (cells < 2) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array must create at least one copy");
    }
    if (!std::isfinite(spacingX) || !std::isfinite(spacingY)) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array spacing must be finite");
    }
    if (columns > 1 && std::abs(spacingX) <= kSpacingEpsilon) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array column spacing must move copied objects");
    }
    if (rows > 1 && std::abs(spacingY) <= kSpacingEpsilon) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array row spacing must move copied objects");
    }
    const std::size_t copyCells = cells - 1;
    if (newObjectIds.size() != copyCells) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "grid array id count must match its cells");
    }

    // Row-major over the grid, skipping cell (0,0) — the source already sits
    // there. Offsets, not absolute positions: planners place copies relative
    // to wherever the source is, they never move the source.
    std::vector<Point2D> offsets;
    offsets.reserve(copyCells);
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            if (row == 0 && column == 0) {
                continue;
            }
            offsets.push_back({spacingX * column, spacingY * row});
        }
    }
    return buildTranslatedCopies(source, newObjectIds, offsets, "grid_array");
}

DraftingArrayResult radialArrayDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    Point2D center)
{
    if (newObjectIds.empty()) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "radial array requires at least one copy");
    }
    if (!std::isfinite(center.x) || !std::isfinite(center.y)) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "radial array centre must be finite");
    }
    if (std::holds_alternative<GuideGeometry>(source.geometry)) {
        // Same single-axis-translation problem as the grid planner: ring
        // offsets with equal axis components would stack coincident guides.
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "radial array does not support guides");
    }

    // The ring's reference point is the source's bounds centre — recomputed
    // here rather than trusting the stored bounds, so the planner cannot be
    // poisoned by a stale cache.
    const Bounds2D bounds = computeBounds(source.geometry);
    const Point2D reference{bounds.x + bounds.width / 2.0, bounds.y + bounds.height / 2.0};
    const double armX = reference.x - center.x;
    const double armY = reference.y - center.y;
    if (std::hypot(armX, armY) <= kSpacingEpsilon) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "radial array centre must differ from the object position");
    }

    // Copies fill the remaining slots of a ring divided evenly among
    // copies + source. Each offset rotates the centre->source arm and takes
    // the delta from the reference, so the copy lands on the ring while the
    // geometry itself is only translated (axis-aligned kinds cannot rotate).
    const std::size_t slots = newObjectIds.size() + 1;
    std::vector<Point2D> offsets;
    offsets.reserve(newObjectIds.size());
    for (std::size_t index = 0; index < newObjectIds.size(); ++index) {
        const double angle = (2.0 * M_PI * static_cast<double>(index + 1)) / static_cast<double>(slots);
        const double rotatedX = armX * std::cos(angle) - armY * std::sin(angle);
        const double rotatedY = armX * std::sin(angle) + armY * std::cos(angle);
        offsets.push_back({center.x + rotatedX - reference.x, center.y + rotatedY - reference.y});
    }
    return buildTranslatedCopies(source, newObjectIds, offsets, "radial_array");
}

DraftingArrayResult rotateCopiesDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    Point2D center,
    double totalAngleDeg)
{
    if (newObjectIds.empty()) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "rotate-copies requires at least one copy");
    }
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(totalAngleDeg)) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "rotate-copies centre and angle must be finite");
    }
    if (std::holds_alternative<GuideGeometry>(source.geometry)) {
        // transformGeometry leaves a guide unchanged (an infinite axis-aligned line
        // cannot rotate about a pivot), so every copy would be identical — reject,
        // as the radial array does for the analogous stacking reason.
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry, "rotate-copies does not support guides");
    }

    // Same arm distribution as radialArrayDraftingObject: the fan is divided evenly
    // among the copies PLUS the source, so the step is totalAngle / (copies + 1) and
    // copy i sits at step*(i+1). totalAngle = 360 reproduces the radial ring exactly
    // (copy angles 360/(copies+1) * (i+1)); the only difference is the copy ALSO
    // rotates by that angle.
    const std::size_t slots = newObjectIds.size() + 1;
    std::vector<double> anglesDeg;
    anglesDeg.reserve(newObjectIds.size());
    for (std::size_t index = 0; index < newObjectIds.size(); ++index) {
        anglesDeg.push_back(totalAngleDeg * static_cast<double>(index + 1) / static_cast<double>(slots));
    }
    return buildRotatedCopies(source, newObjectIds, center, anglesDeg, "rotate_copies");
}

DraftingArrayResult arrayAlongCurve(const DraftingObject &source,
                                    const std::vector<DraftingObjectId> &newObjectIds,
                                    const DraftingGeometry &path,
                                    bool alignToTangent)
{
    const std::size_t K = newObjectIds.size();

    // Reject immediately on empty id list — nothing to place.
    if (K == 0) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry,
            "array-along-curve requires at least one copy");
    }

    // Compute arc-length and (for Spline) the sampled point list.  curveArcLength
    // returns -1 for unsupported path kinds, so this also acts as a kind filter:
    // only Line / Polyline / Arc / Spline produce a non-negative length.
    std::vector<Point2D> splineSamples;
    const double L = curveArcLength(path, splineSamples);
    if (L < 0.0) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry,
            "array-along-curve path must be a Line, Polyline, Arc, or Spline");
    }
    if (!std::isfinite(L) || !(L > kPathEpsilon)) {
        return DraftingArrayResult::rejected(DraftingResultCode::InvalidGeometry,
            "array-along-curve path has zero or non-finite arc-length");
    }

    // Same kind/geometry consistency and unique-id checks that every array
    // planner performs — reuse the pattern from buildTranslatedCopies.
    if (!kindMatchesGeometry(source.kind, source.geometry)) {
        return DraftingArrayResult::rejected(DraftingResultCode::KindGeometryMismatch,
            "shape kind does not match geometry");
    }
    std::unordered_set<DraftingObjectId> usedIds;
    usedIds.reserve(K);
    for (const DraftingObjectId &id : newObjectIds) {
        if (id.empty() || id == source.id || !usedIds.insert(id).second) {
            return DraftingArrayResult::rejected(DraftingResultCode::DuplicateObjectId,
                "array object ids must be unique");
        }
    }

    // The anchor for translation: the source's bounds centre.  Recomputed from
    // geometry (not from the stored bounds) so a stale bounds cache cannot
    // shift every copy — same discipline as radialArrayDraftingObject.
    const Bounds2D srcBounds = computeBounds(source.geometry);
    const Point2D C{srcBounds.x + srcBounds.width / 2.0,
                    srcBounds.y + srcBounds.height / 2.0};

    std::vector<DraftingObject> copies;
    copies.reserve(K);

    for (std::size_t i = 0; i < K; ++i) {
        // Evenly arc-length-spaced stations that include BOTH path endpoints.
        // K == 1: only d = 0 (path start), so the single copy lands at the
        //         start point.
        // K >= 2: d_i = i * L / (K - 1), spanning 0 … L.
        // Clamp to [0, L] to absorb floating-point rounding at the last station.
        const double d = (K == 1)
            ? 0.0
            : std::min(static_cast<double>(i) * L / static_cast<double>(K - 1), L);

        const Point2D station = curveStationPoint(path, d, splineSamples);

        // Step 1: translate so the copy's centre lands on the station.
        DraftingObject copy = source;
        copy.id = newObjectIds[i];
        copy.geometry = translateGeometry(source.geometry,
                                          station.x - C.x,
                                          station.y - C.y);

        // Step 2 (optional): rotate to the path tangent, pivoting about the
        // station itself so the centre stays pinned on the station.
        // transformGeometry handles all geometry kinds: it rotates every point
        // about the pivot AND increments any stored rotationDeg field — so a
        // Rectangle's rotationDeg ends up at `sourceDeg + tangentDeg`.
        if (alignToTangent) {
            const double tangentDeg = curveTangentDeg(path, d, splineSamples);
            copy.geometry = transformGeometry(copy.geometry, station, tangentDeg, 1.0);
        }

        copy.metadata.toolProvenance = "array_along_curve";
        copy.metadata.source = source.id;

        const auto validation = validateDraftingObjectShape(copy);
        if (!validation.ok) {
            return DraftingArrayResult::rejected(validation.code, validation.message);
        }
        copy.bounds = computeBounds(copy.geometry);
        copies.push_back(std::move(copy));
    }

    return DraftingArrayResult::accepted(std::move(copies));
}

} // namespace edi::drafting
