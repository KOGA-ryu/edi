#include "recipe/RecipeEmit.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <string>

namespace edi::recipe {

namespace {

// Deterministic number text: whole values print bare ("2"), fractional ones
// with up to nine significant digits — enough to round-trip a double through
// Blender without ever printing platform-dependent noise.
std::string numberText(double value)
{
    if (std::isfinite(value) && value == std::floor(value) && std::abs(value) < 1e15) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
        return buffer;
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    return buffer;
}

double paramValue(const ResolvedStep &step, const std::string &id)
{
    for (const ResolvedParam &param : step.params) {
        if (param.id == id) {
            return param.value;
        }
    }
    return 0.0;
}

std::string sourcesComment(const ResolvedStep &step)
{
    std::string measured;
    for (const ResolvedParam &param : step.params) {
        if (param.fromMeasurement) {
            measured += measured.empty() ? param.id : (", " + param.id);
        }
    }
    return measured.empty() ? std::string() : ("  # measured from canvas: " + measured + "\n");
}

// One emitter per shaper — a callable table, the same dispatch the shaper
// vocabulary itself uses. Each returns the Python block for its step.
using StepEmitter = std::function<std::string(const ResolvedStep &)>;

const std::map<std::string, StepEmitter> &stepEmitters()
{
    static const std::map<std::string, StepEmitter> emitters = {
        {"cube", [](const ResolvedStep &step) {
            // A unit cube scaled per axis: scale IS the dimension when the
            // base size is 1, so the measured numbers land verbatim.
            std::string block;
            // location pinned and align='WORLD': an unset location inherits
            // the 3D CURSOR, which would slide the part off the lathe axis
            // in any scene where the cursor moved.
            block += "bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0), align='WORLD')\n";
            block += "obj = bpy.context.active_object\n";
            block += "obj.scale = (" + numberText(paramValue(step, "size_x"))
                + ", " + numberText(paramValue(step, "size_y"))
                + ", " + numberText(paramValue(step, "size_z")) + ")\n";
            // Modifiers evaluate on the LOCAL mesh and the object transform
            // applies afterwards — an unapplied scale would silently multiply
            // every later modifier number (bevel width, array offset) by it.
            // Baking the scale keeps "every number is exact" true in world
            // space; location stays a pure transform for the line below.
            block += "bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)\n";
            // loc_z is the part's BOTTOM; Blender places by centre, so the
            // emitter does that arithmetic and shows its work in the comment.
            block += "obj.location[2] = "
                + numberText(paramValue(step, "loc_z") + paramValue(step, "size_z") / 2.0)
                + "  # base z = " + numberText(paramValue(step, "loc_z")) + "\n";
            return block;
        }},
        {"cylinder", [](const ResolvedStep &step) {
            std::string block;
            block += "bpy.ops.mesh.primitive_cylinder_add(radius="
                + numberText(paramValue(step, "radius"))
                + ", depth=" + numberText(paramValue(step, "depth"))
                + ", location=(0.0, 0.0, 0.0), align='WORLD')\n";
            block += "obj = bpy.context.active_object\n";
            block += "obj.location[2] = "
                + numberText(paramValue(step, "loc_z") + paramValue(step, "depth") / 2.0)
                + "  # base z = " + numberText(paramValue(step, "loc_z")) + "\n";
            return block;
        }},
        {"lathe", [](const ResolvedStep &step) {
            // The profile becomes mesh data directly (from_pydata), then a
            // SCREW modifier spins it a full turn around Z — Blender's lathe.
            // Exact drafted coordinates land verbatim in the vertex table;
            // %.9g printing absorbs binary noise so 0.105*12 reads "1.26".
            std::string verts;
            std::string edges;
            for (std::size_t i = 0; i < step.profilePoints.size(); ++i) {
                if (i > 0) {
                    verts += ", ";
                    edges += edges.empty() ? "" : ", ";
                    edges += "(" + numberText(static_cast<double>(i - 1))
                        + ", " + numberText(static_cast<double>(i)) + ")";
                }
                verts += "(" + numberText(step.profilePoints[i].x)
                    + ", 0.0, " + numberText(step.profilePoints[i].y) + ")";
            }
            std::string block;
            block += "mesh = bpy.data.meshes.new(\"lathe\")\n";
            block += "mesh.from_pydata([" + verts + "], [" + edges + "], [])\n";
            block += "obj = bpy.data.objects.new(\"lathe\", mesh)\n";
            block += "bpy.context.collection.objects.link(obj)\n";
            block += "bpy.context.view_layer.objects.active = obj\n";
            block += "mod = obj.modifiers.new(name=\"Lathe\", type='SCREW')\n";
            block += "mod.angle = 6.283185307179586\n";
            block += "mod.steps = " + numberText(paramValue(step, "segments")) + "\n";
            // render_steps is an INDEPENDENT property defaulting to 16: a
            // render would silently rebuild the lathe coarser than the
            // viewport the user inspected. One number, both places.
            block += "mod.render_steps = " + numberText(paramValue(step, "segments")) + "\n";
            block += "mod.use_merge_vertices = True\n";
            // Outward normals regardless of the profile's vertex order —
            // the groove booleans classify inside/outside by them, and a
            // bottom-up vs top-down drafted profile must not flip the cut.
            block += "mod.use_normal_calculate = True\n";
            // The drafted heights are authoritative; loc_z is an OFFSET for
            // reusing one profile at another height, 0 means "as drafted".
            block += "obj.location[2] = " + numberText(paramValue(step, "loc_z"))
                + "  # offset from drafted height\n";
            return block;
        }},
        {"radial_groove", [](const ResolvedStep &step) {
            // count cutters on an exact circle, each a boolean difference.
            // The loop stays IN the python: the count is one pointable
            // number, and unrolling 20 cutter blocks would bury it.
            const double count = paramValue(step, "count");
            const double cutterRadius = paramValue(step, "cutter_radius");
            const double depth = paramValue(step, "depth");
            const double atRadius = paramValue(step, "at_radius");
            const double zFrom = paramValue(step, "z_from");
            const double zTo = paramValue(step, "z_to");
            std::string block;
            block += "_target = obj\n";
            block += "for _i in range(" + numberText(count) + "):\n";
            block += "    _a = 6.283185307179586 * _i / " + numberText(count) + "\n";
            // Centre distance = at_radius + cutter_radius - depth: the cutter
            // overlaps the surface by exactly `depth`.
            block += "    _d = " + numberText(atRadius + cutterRadius - depth)
                + "  # at_radius " + numberText(atRadius)
                + " + cutter " + numberText(cutterRadius)
                + " - depth " + numberText(depth) + "\n";
            block += "    bpy.ops.mesh.primitive_cylinder_add(radius=" + numberText(cutterRadius)
                + ", depth=" + numberText(zTo - zFrom)
                + ", location=(0.0, 0.0, 0.0), align='WORLD')\n";
            block += "    _c = bpy.context.active_object\n";
            block += "    _c.location = (_d * math.cos(_a), _d * math.sin(_a), "
                + numberText((zFrom + zTo) / 2.0) + ")\n";
            block += "    _m = _target.modifiers.new(name=\"Groove\", type='BOOLEAN')\n";
            block += "    _m.operation = 'DIFFERENCE'\n";
            block += "    _m.object = _c\n";
            block += "    _c.hide_set(True)\n";
            block += "    _c.hide_render = True\n";
            block += "obj = _target\n";
            return block;
        }},
        {"bevel", [](const ResolvedStep &step) {
            std::string block;
            block += "mod = obj.modifiers.new(name=\"Bevel\", type='BEVEL')\n";
            block += "mod.width = " + numberText(paramValue(step, "width")) + "\n";
            block += "mod.segments = " + numberText(paramValue(step, "segments")) + "\n";
            return block;
        }},
        {"array", [](const ResolvedStep &step) {
            // Constant offset, not relative: the measured spacing must reach
            // Blender as the exact number, independent of the object's size.
            std::string block;
            block += "mod = obj.modifiers.new(name=\"Array\", type='ARRAY')\n";
            block += "mod.count = " + numberText(paramValue(step, "count")) + "\n";
            block += "mod.use_relative_offset = False\n";
            block += "mod.use_constant_offset = True\n";
            block += "mod.constant_offset_displace[0] = "
                + numberText(paramValue(step, "offset_x")) + "\n";
            return block;
        }},
    };
    return emitters;
}

} // namespace

namespace {

// Parameters that become python loop bounds or modifier step counts: a
// fractional or non-positive value would emit a script that CRASHES inside
// Blender (range(2.5) is a TypeError) — refusing here preserves the
// "never write a bad script" contract for values that arrive via
// measurement bindings, which bypass setParamLiteral's finite gate.
struct WholeNumberParam {
    const char *shaperId;
    const char *paramId;
};

constexpr WholeNumberParam kWholeNumberParams[] = {
    {"radial_groove", "count"},
    {"array", "count"},
    {"bevel", "segments"},
    {"lathe", "segments"},
};

std::string validateResolvedNumbers(const ResolvedRecipe &resolved)
{
    for (std::size_t i = 0; i < resolved.steps.size(); ++i) {
        const ResolvedStep &step = resolved.steps[i];
        for (const ResolvedParam &param : step.params) {
            if (!std::isfinite(param.value)) {
                return "step " + std::to_string(i) + ": " + step.shaperId + "." + param.id
                    + " is not a finite number";
            }
            for (const WholeNumberParam &rule : kWholeNumberParams) {
                if (step.shaperId == rule.shaperId && param.id == rule.paramId
                    && (param.value < 1.0 || param.value != std::floor(param.value))) {
                    return "step " + std::to_string(i) + ": " + step.shaperId + "." + param.id
                        + " must be a positive whole number";
                }
            }
        }
    }
    return {};
}

} // namespace

RecipeEmitResult emitBlenderPython(const RecipeDocument &document, const ResolvedRecipe &resolved)
{
    RecipeEmitResult result;
    if (!resolved.ok || resolved.steps.size() != document.steps.size()) {
        result.message = "recipe is not fully resolved; fix the stale bindings first";
        return result;
    }
    if (std::string numberProblem = validateResolvedNumbers(resolved); !numberProblem.empty()) {
        result.message = std::move(numberProblem);
        return result;
    }

    std::string script;
    script += "# edi recipe: " + (document.name.empty() ? document.id : document.name) + "\n";
    script += "# generated by edi - every number below is exact (typed or measured)\n";
    script += "import bpy\n";
    script += "import math\n";
    // A user startup file can be sitting in Edit Mode; primitive_add would
    // then merge the new part INTO the edited mesh. One guard line makes
    // the script honest about the scene state it needs.
    script += "if bpy.context.object and bpy.context.object.mode != 'OBJECT':\n";
    script += "    bpy.ops.object.mode_set(mode='OBJECT')\n";

    for (std::size_t i = 0; i < resolved.steps.size(); ++i) {
        const ResolvedStep &step = resolved.steps[i];
        const auto emitter = stepEmitters().find(step.shaperId);
        if (emitter == stepEmitters().end()) {
            result.message = "no emitter for shaper: " + step.shaperId;
            return result;
        }
        script += "\n# step " + numberText(static_cast<double>(i + 1)) + ": " + step.shaperId + "\n";
        script += sourcesComment(step);
        script += emitter->second(step);
    }

    result.ok = true;
    result.script = std::move(script);
    return result;
}

} // namespace edi::recipe
