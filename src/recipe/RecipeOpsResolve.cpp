#include "recipe/RecipeOpsResolve.h"

#include "recipe/RecipeMeasure.h"
#include "recipe/RecipeOpsBind.h"

#include <cstdio>
#include <string>
#include <utility>

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
    // Divergence note (vs pipeline A's lathe, recorded for the B05 gate
    // decision): the moulding container lofts STACKED RINGS and its
    // validator requires rising z, while A's SCREW-modifier lathe accepted
    // any drafted orientation (its emitter recalculated normals). A
    // falling-z or folded drafted profile therefore lowers verbatim here
    // and then FAILS validation by name (moulding_profile_not_monotonic /
    // bad_moulding_radius) instead of silently winding inside-out in
    // Blender. Refusal over reorder: silently rewriting drafted truth
    // would be guesswork. B05 decides whether resolved-lathe profiles get
    // direction-normalization or a true screw-loft op.
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
        AddMouldingOp lowered;
        lowered.name = revolved->name;
        lowered.baseZ = revolved->baseZ;
        lowered.x = revolved->x;
        lowered.y = revolved->y;
        lowered.vertices = revolved->vertices;
        lowered.material = revolved->material;
        lowered.profile.reserve(profile.points.size());
        for (std::size_t point = 0; point < profile.points.size(); ++point) {
            char term[16];
            std::snprintf(term, sizeof(term), "profile_%02zu", point);
            // Seam points are physical (x = radius, y = z from page bottom);
            // moulding points are (z, radius) local to baseZ. The column's
            // lathes used loc_z = 0, so baseZ + absolute-z reproduces A.
            lowered.profile.push_back({term, profile.points[point].y, profile.points[point].x});
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

} // namespace edi::recipe
