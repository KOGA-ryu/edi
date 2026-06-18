#include "recipe/RecipeOpsResolve.h"

#include "recipe/RecipeMeasure.h"
#include "recipe/RecipeOpsBind.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <variant>

namespace edi::recipe {

OpResolveResult resolveRecipeOps(const RecipeOpStream &stream,
                                 const edi::drafting::DraftingDocument &drafting,
                                 const edi::drafting::DraftingGridProjection &grid)
{
    // Resolve into a COPY: the input stays untouched (the pass is pure), and a
    // partially-written copy can be discarded wholesale if a later binding
    // fails. The doubles in `resolved.ops` are the values every downstream
    // consumer already reads — resolution just replaces the inert struct
    // default a bound field carried since load (R1-B02) with the measured
    // number.
    OpResolveResult result;
    RecipeOpStream resolved = stream;

    for (const RecipeFieldBinding &binding : stream.bindings) {
        // The finding's address is the binding's op-field — the coordinate the
        // writer and reader already use to name a binding.
        const std::string key =
            "op." + std::to_string(binding.opIndex) + "." + binding.fieldKey;

        // Defensive: the store validates opIndex range at both read and write,
        // so a binding past the end of `ops` is only reachable on a hand-built
        // stream. Fail it (never index out of bounds); no production path
        // reaches here. This guard is outside the bucket's pinned refusals.
        if (binding.opIndex >= resolved.ops.size()) {
            result.findings.push_back({key, "no such op"});
            continue;
        }

        // Measure first, through the seam pipeline A shares: one vocabulary,
        // one set of wordings. `binding.field` is the MEASUREMENT field
        // (width/height/length/radius); `binding.fieldKey` is where on the op
        // the number lands — the two are independent.
        const MeasureFieldResult measured = resolveMeasurementField(
            drafting, grid, binding.objectId, binding.field);
        if (!measured.ok) {
            result.findings.push_back({key, measured.message});
            continue;
        }

        // Finiteness gate (R1-B05, decision 3): a degenerate grid or geometry
        // can drive a measured number to inf/nan. Pipeline A gated at emit; the
        // op pipeline gates at the SAME meeting point — here, with the value in
        // hand, BEFORE it is written and the stream is declared resolved. Per
        // offender, so the downstream doubles never carry a non-finite value.
        if (!std::isfinite(measured.value)) {
            result.findings.push_back({key, "not a finite number"});
            continue;
        }

        // Write through the B02 registry's member pointer. A false return means
        // the fieldKey is not bindable on this op kind — only reachable on a
        // hand-built stream (the store refuses such files at read and write),
        // reported in the writer's wording family.
        if (!setOpFieldValue(resolved.ops[binding.opIndex], binding.fieldKey, measured.value)) {
            result.findings.push_back({key, "not a bindable field"});
            continue;
        }
    }

    // Profile lowering (R1-B04): every AddRevolvedProfileOp becomes an
    // AddMouldingOp with exact physical (radius, z) points from the shared
    // seam — the lathe, op-vocabulary native. This runs AFTER bindings so a
    // bound base_z/x/y has already landed on the op being lowered. Points
    // get generated names ("profile_00"…): moulding points are pointable BY
    // NAME, and a drafted profile's points deserve addresses too.
    //
    // Direction note (decision 7 RESOLVED in R1-B05): the moulding container
    // lofts STACKED RINGS bottom-up and its validator requires rising z, while
    // A's SCREW-modifier lathe recalculated normals and accepted either drafted
    // orientation. A profile whose physical z STRICTLY falls end-to-end is the
    // same rings drawn top-down — it is direction-NORMALIZED below (point order
    // reversed, no coordinate touched) so it lofts identically to the bottom-up
    // drawing, restoring A's orientation-agnostic promise without inventing
    // data. A FOLDED (non-monotonic) profile is NOT rescued: it lowers and
    // still FAILS validation by name (moulding_profile_not_monotonic) — a
    // stacked-ring loft cannot represent an overhang; a future screw-loft op
    // owns folds.
    for (std::size_t index = 0; index < resolved.ops.size(); ++index) {
        const auto *revolved = std::get_if<AddRevolvedProfileOp>(&resolved.ops[index]);
        if (revolved == nullptr) {
            continue;
        }
        const std::string key = "op." + std::to_string(index) + ".profile";
        const ProfilePointsResult profile =
            resolveProfilePoints(drafting, grid, revolved->profile);
        if (!profile.ok) {
            result.findings.push_back({key, profile.message});
            continue;
        }
        // Normalize a strictly-falling profile to canonical rising order —
        // ALL adjacent pairs descending, not just the endpoints (planner
        // ruling on the B05 builder's flag #2): only a profile the loft can
        // FULLY understand gets normalized; a folded profile — even one whose
        // endpoints happen to descend net — lowers VERBATIM and fails
        // validation honestly, exactly as decision 7 promises. Only the
        // ORDER ever changes — every (radius, z) pair is preserved.
        std::vector<edi::drafting::Point2D> points = profile.points;
        const bool strictlyFalling = points.size() >= 2
            && std::adjacent_find(points.begin(), points.end(),
                                  [](const edi::drafting::Point2D &a,
                                     const edi::drafting::Point2D &b) {
                                      return b.y >= a.y; // a non-descending pair breaks it
                                  })
                == points.end();
        if (strictlyFalling) {
            std::reverse(points.begin(), points.end());
        }
        AddMouldingOp lowered;
        lowered.name = revolved->name;
        lowered.baseZ = revolved->baseZ;
        lowered.x = revolved->x;
        lowered.y = revolved->y;
        lowered.vertices = revolved->vertices;
        lowered.material = revolved->material;
        lowered.sweepDegrees = revolved->sweepDegrees; // BL-06: the arc survives lowering
        lowered.screwRise = revolved->screwRise;       // BL-07: the helix survives lowering
        lowered.screwTurns = revolved->screwTurns;
        lowered.profile.reserve(points.size());
        for (std::size_t point = 0; point < points.size(); ++point) {
            char term[16];
            std::snprintf(term, sizeof(term), "profile_%02zu", point);
            // Seam points are physical (x = radius, y = z from page bottom);
            // moulding points are (z, radius) local to baseZ. The column's
            // lathes used loc_z = 0, so baseZ + absolute-z reproduces A.
            lowered.profile.push_back({term, points[point].y, points[point].x});
        }
        resolved.ops[index] = std::move(lowered);
    }

    // Extrude lowering (BL-03): every surviving AddExtrudedProfileOp becomes a
    // concrete, buildable AddPrismOp — the same move as the lathe loop above,
    // but the projection is the drafted FOOTPRINT (physical x/y in the drawing
    // plane), not the lathe's (radius, z) spin profile. Runs AFTER bindings so a
    // bound `height` has already landed. A dedicated carrier (not a reuse of
    // AddMoulding) is the right call: a linear extrude is a planar loop swept
    // straight up, whereas AddMoulding's points are (z, radius) rings revolved
    // about an axis — forcing a footprint into that radius/z shape would lie
    // about the geometry and mislead every downstream consumer.
    for (std::size_t index = 0; index < resolved.ops.size(); ++index) {
        const auto *extruded = std::get_if<AddExtrudedProfileOp>(&resolved.ops[index]);
        if (extruded == nullptr) {
            continue;
        }
        const std::string key = "op." + std::to_string(index) + ".profile";
        const ProfilePointsResult footprint =
            resolveExtrudeProfilePoints(drafting, grid, extruded->profile);
        if (!footprint.ok) {
            result.findings.push_back({key, footprint.message}); // shared wordings
            continue;
        }
        // A prism needs a real polygon: count DISTINCT footprint points (a
        // closed loop may repeat its first vertex as the last) — fewer than
        // three means the drafted figure was a point or a line, not an area.
        std::vector<edi::drafting::Point2D> distinct;
        for (const edi::drafting::Point2D &point : footprint.points) {
            const bool seen = std::any_of(distinct.begin(), distinct.end(),
                                          [&point](const edi::drafting::Point2D &other) {
                                              return other.x == point.x && other.y == point.y;
                                          });
            if (!seen) {
                distinct.push_back(point);
            }
        }
        if (distinct.size() < 3) {
            result.findings.push_back({key, "profile must enclose at least three distinct points"});
            continue;
        }
        AddPrismOp lowered;
        lowered.name = extruded->name;
        lowered.height = extruded->height;
        lowered.baseZ = extruded->baseZ;
        lowered.x = extruded->x;
        lowered.y = extruded->y;
        lowered.material = extruded->material;
        lowered.footprint.reserve(footprint.points.size());
        for (const edi::drafting::Point2D &point : footprint.points) {
            lowered.footprint.push_back({point.x, point.y});
        }
        resolved.ops[index] = std::move(lowered);
    }

    // Sweep lowering (BL-08): every surviving AddSweepProfileOp becomes an
    // AddPrismOp whose footprint is the resolved cross-section and whose `path`
    // is the resolved sweep curve — both projected to the drawing-plane (the
    // same footprint convention). height is irrelevant for a sweep (the path
    // carries the run), so it is left at the default 0. This generalizes the ONE
    // carrier (a path-bearing prism) rather than adding a second carrier arm.
    const auto countDistinct = [](const std::vector<edi::drafting::Point2D> &pts) {
        std::vector<edi::drafting::Point2D> distinct;
        for (const edi::drafting::Point2D &point : pts) {
            const bool seen = std::any_of(distinct.begin(), distinct.end(),
                                          [&point](const edi::drafting::Point2D &o) {
                                              return o.x == point.x && o.y == point.y;
                                          });
            if (!seen) {
                distinct.push_back(point);
            }
        }
        return distinct.size();
    };
    for (std::size_t index = 0; index < resolved.ops.size(); ++index) {
        const auto *swept = std::get_if<AddSweepProfileOp>(&resolved.ops[index]);
        if (swept == nullptr) {
            continue;
        }
        const std::string profileKey = "op." + std::to_string(index) + ".profile";
        const std::string pathKey = "op." + std::to_string(index) + ".path";
        const ProfilePointsResult footprint =
            resolveExtrudeProfilePoints(drafting, grid, swept->profile);
        if (!footprint.ok) {
            result.findings.push_back({profileKey, footprint.message}); // shared wordings
            continue;
        }
        if (countDistinct(footprint.points) < 3) {
            result.findings.push_back({profileKey, "profile must enclose at least three distinct points"});
            continue;
        }
        const ProfilePointsResult path = resolveSweepPath(drafting, grid, swept->path);
        if (!path.ok) {
            result.findings.push_back({pathKey, path.message}); // shared wordings
            continue;
        }
        if (countDistinct(path.points) < 2) {
            result.findings.push_back({pathKey, "sweep path needs at least two distinct points"});
            continue;
        }
        AddPrismOp lowered;
        lowered.name = swept->name;
        lowered.baseZ = swept->baseZ;
        lowered.x = swept->x;
        lowered.y = swept->y;
        lowered.material = swept->material;
        lowered.taperEnd = swept->taperEnd;     // BL-09: the taper survives lowering
        lowered.taperCurve = swept->taperCurve; // P4: the curve exponent survives too
        // height stays 0 — the path, not a straight rise, defines the solid.
        lowered.footprint.reserve(footprint.points.size());
        for (const edi::drafting::Point2D &point : footprint.points) {
            lowered.footprint.push_back({point.x, point.y});
        }
        lowered.path.reserve(path.points.size());
        for (const edi::drafting::Point2D &point : path.points) {
            lowered.path.push_back({point.x, point.y});
        }
        resolved.ops[index] = std::move(lowered);
    }

    // All-or-nothing. `result` already defaults to ok=false with an empty
    // stream, so returning it now guarantees no half-resolved ops escape while
    // `findings` still carries every failure at once. Deleting this guard (or
    // committing `resolved` before it) lets a partial stream through — the
    // multi-failure test pins exactly that.
    if (!result.findings.empty()) {
        return result;
    }

    resolved.bindings.clear();
    result.stream = std::move(resolved);
    result.ok = true;
    return result;
}

bool recipeOpsResolved(const RecipeOpStream &stream)
{
    // Unresolved states, all fatal downstream: a measurement binding that
    // never got written, an AddRevolvedProfileOp that never got lowered, and
    // (BL-01) an AddExtrudedProfileOp that never got lowered. A successful
    // resolveRecipeOps clears them; anything that fails this is not safe to
    // compile, preview, or export.
    if (!stream.bindings.empty()) {
        return false;
    }
    for (const RecipeOp &op : stream.ops) {
        if (std::holds_alternative<AddRevolvedProfileOp>(op)
            || std::holds_alternative<AddExtrudedProfileOp>(op)
            || std::holds_alternative<AddSweepProfileOp>(op)) { // BL-08
            return false;
        }
    }
    return true;
}

} // namespace edi::recipe
