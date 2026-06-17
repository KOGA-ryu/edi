#include "drafting/DraftingArray.h"

#include "drafting/DraftingGeometry.h"

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

} // namespace edi::drafting
