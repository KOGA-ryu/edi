#include "recipe/RecipeOpsValidate.h"

#include "recipe/RecipeTextNumbers.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace edi::recipe {

namespace {

using Severity = OpFinding::Severity;

void add(std::vector<OpFinding> &findings, Severity severity, std::string code, std::string message)
{
    findings.push_back({severity, std::move(code), std::move(message)});
}

void checkMaterial(std::vector<OpFinding> &findings, const std::string &name, const std::string &material)
{
    if (!recipeMaterialSupported(material)) {
        // Port divergence: v0's backend repainted unknown materials to
        // stone via dict fallback. A typo must be a named finding.
        add(findings, Severity::Error, "unknown_material",
            name + " has unknown material: '" + material + "'");
    }
}

// The shared lathe parameter checks, called from both the AddRevolvedProfile
// (lathe) arm and the AddMoulding arm it lowers to, so the two stay in step.
// BL-06: a revolve sweeps an arc in (0, 360]. 360 is the full ring (the
// default); 0 or negative makes no solid, and > 360 over-winds — refused by
// name. BL-07: screw_turns must be a positive, finite count (0 or negative
// makes no helix; non-finite is nonsense); screw_rise may be any FINITE value
// (0 = no helix, negative = a left-hand spiral, both legitimate).
void checkLatheParams(std::vector<OpFinding> &findings, const std::string &name,
                      double sweepDegrees, double screwRise, double screwTurns)
{
    if (sweepDegrees <= 0 || sweepDegrees > 360) {
        add(findings, Severity::Error, "bad_sweep_degrees",
            name + " sweep must be in (0, 360].");
    }
    if (!(screwTurns > 0) || !std::isfinite(screwTurns)) {
        add(findings, Severity::Error, "bad_screw_turns",
            name + " screw_turns must be a positive, finite count.");
    }
    if (!std::isfinite(screwRise)) {
        add(findings, Severity::Error, "bad_screw_rise",
            name + " screw_rise must be finite.");
    }
}

// The per-op checks, v0 codes and wording preserved. A visitor over the
// variant — the same dispatch shape the drafting commands use.
struct OpChecker {
    std::vector<OpFinding> &findings;
    const std::unordered_set<std::string> &namesSoFar;
    const std::unordered_set<std::string> &nonVerticalCylinders;

    void operator()(const AddBoxOp &op) const
    {
        if (op.width <= 0 || op.depth <= 0 || op.height <= 0) {
            add(findings, Severity::Error, "bad_box_dimensions", op.name + " has non-positive dimensions.");
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddCylinderOp &op) const
    {
        if (op.radius <= 0 || op.height <= 0) {
            add(findings, Severity::Error, "bad_cylinder_dimensions", op.name + " has non-positive dimensions.");
        }
        if (op.vertices < 12) {
            add(findings, Severity::Warning, "low_cylinder_vertices",
                op.name + " has low vertex count: " + std::to_string(op.vertices));
        }
        if (op.taperTopRadius.has_value() && *op.taperTopRadius <= 0) {
            add(findings, Severity::Error, "bad_taper_radius", op.name + " has invalid top radius.");
        }
        // Port addition: entasis_ratio is a new field (v0 hardcoded the
        // 0.045 bulge), so v0 had no code for it.
        if (op.entasisRatio < 0) {
            add(findings, Severity::Error, "bad_entasis_ratio", op.name + " entasis_ratio must be non-negative.");
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddSphereOp &op) const
    {
        if (op.radius <= 0) {
            add(findings, Severity::Error, "bad_sphere_radius", op.name + " has non-positive radius.");
        }
        if (op.vertices < 8) {
            add(findings, Severity::Warning, "low_sphere_vertices",
                op.name + " has low vertex count: " + std::to_string(op.vertices));
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddRingOp &op) const
    {
        if (op.radius <= 0 || op.tubeHeight <= 0) {
            add(findings, Severity::Error, "bad_ring_dimensions", op.name + " has non-positive dimensions.");
        }
        if (op.overhang != 0.0) {
            // Port addition: both backends honor overhang as v0 did
            // (radius + overhang), but the ring is a cylinder ALIAS — the
            // overhang widens it; no torus tube exists to overhang.
            add(findings, Severity::Warning, "ring_overhang_alias",
                op.name + " overhang widens the cylinder-alias radius; no torus is built.");
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddMouldingOp &op) const
    {
        if (op.baseZ < 0) {
            add(findings, Severity::Warning, "negative_moulding_base_z", op.name + " starts below z=0.");
        }
        if (op.vertices < 12) {
            add(findings, Severity::Warning, "low_moulding_vertices",
                op.name + " has low vertex count: " + std::to_string(op.vertices));
        }
        if (op.profile.size() < 2) {
            add(findings, Severity::Error, "short_moulding_profile", op.name + " needs at least two profile points.");
        }
        for (std::size_t index = 0; index < op.profile.size(); ++index) {
            const MouldingPoint &point = op.profile[index];
            if (point.radius <= 0) {
                add(findings, Severity::Error, "bad_moulding_radius",
                    op.name + " point " + std::to_string(index) + " has non-positive radius.");
            }
            if (point.z < 0) {
                add(findings, Severity::Error, "bad_moulding_z",
                    op.name + " point " + std::to_string(index) + " has negative local z.");
            }
            // One finding PER violating point, as v0 reports it — the
            // report should name every kink, not just the first.
            if (index > 0 && point.z < op.profile[index - 1].z) {
                add(findings, Severity::Error, "moulding_profile_not_monotonic",
                    op.name + " profile z values must rise in order.");
            }
        }
        checkLatheParams(findings, op.name, op.sweepDegrees, op.screwRise, op.screwTurns);
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddRevolvedProfileOp &op) const
    {
        // Port addition (no v0 analog — the drafted-profile reference is
        // pipeline A's contribution): the reference itself must exist; its
        // resolvability against a drawing is the resolve pass's concern.
        if (op.profile.empty()) {
            add(findings, Severity::Error, "missing_profile_reference",
                op.name + " needs a drafted profile reference.");
        }
        if (op.baseZ < 0) {
            add(findings, Severity::Warning, "negative_revolved_profile_base_z",
                op.name + " starts below z=0.");
        }
        if (op.vertices < 12) {
            add(findings, Severity::Warning, "low_revolved_profile_vertices",
                op.name + " has low vertex count: " + std::to_string(op.vertices));
        }
        checkLatheParams(findings, op.name, op.sweepDegrees, op.screwRise, op.screwTurns);
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddExtrudedProfileOp &op) const
    {
        // BL-01: like the lathe, the drafted-profile reference must exist; its
        // resolvability is the resolve pass's concern (BL-03).
        if (op.profile.empty()) {
            add(findings, Severity::Error, "missing_profile_reference",
                op.name + " needs a drafted profile reference.");
        }
        // A zero or non-finite extrude has no prism — refuse it. A NEGATIVE
        // height is deliberately allowed: BL-05 reads it as a push/pull cut, so
        // refusing it here would block that future meaning.
        if (op.height == 0.0 || !std::isfinite(op.height)) {
            add(findings, Severity::Error, "extruded_profile_zero_height",
                op.name + " needs a non-zero, finite height.");
        }
        if (op.baseZ < 0) {
            add(findings, Severity::Warning, "negative_extruded_profile_base_z",
                op.name + " starts below z=0.");
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddPrismOp &op) const
    {
        // BL-03: the lowered prism carrier. A real prism needs a polygon (>= 3
        // footprint points) and a non-zero, finite height. A NEGATIVE height is
        // allowed (BL-05 reads it as a push/pull cut), mirroring the extrude.
        if (op.footprint.size() < 3) {
            add(findings, Severity::Error, "prism_degenerate_footprint",
                op.name + " needs at least three footprint points.");
        }
        if (op.height == 0.0 || !std::isfinite(op.height)) {
            add(findings, Severity::Error, "prism_zero_height",
                op.name + " needs a non-zero, finite height.");
        }
        if (op.baseZ < 0) {
            add(findings, Severity::Warning, "negative_prism_base_z",
                op.name + " starts below z=0.");
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const AddProfileMouldingOp &op) const
    {
        if (op.baseZ < 0) {
            add(findings, Severity::Warning, "negative_profile_moulding_base_z", op.name + " starts below z=0.");
        }
        if (op.vertices < 12) {
            add(findings, Severity::Warning, "low_profile_moulding_vertices",
                op.name + " has low vertex count: " + std::to_string(op.vertices));
        }
        if (op.sequence.empty()) {
            add(findings, Severity::Error, "empty_profile_moulding_sequence", op.name + " needs at least one segment.");
        }
        for (std::size_t index = 0; index < op.sequence.size(); ++index) {
            const MouldingSegment &segment = op.sequence[index];
            if (!mouldingTermSupported(segment.term)) {
                add(findings, Severity::Error, "unknown_profile_term",
                    op.name + " segment " + std::to_string(index) + " has unknown term: '" + segment.term + "'");
            }
            if (segment.height <= 0) {
                add(findings, Severity::Error, "bad_profile_segment_height",
                    op.name + " segment " + std::to_string(index) + " height must be positive.");
            }
            if (index == 0 && !segment.startRadius.has_value()) {
                add(findings, Severity::Error, "missing_profile_start_radius",
                    op.name + " first segment needs start_radius.");
            }
            if (segment.startRadius.has_value() && *segment.startRadius <= 0) {
                add(findings, Severity::Error, "bad_profile_segment_radius",
                    op.name + " segment " + std::to_string(index) + " start_radius must be positive.");
            }
            if (segment.endRadius.has_value() && *segment.endRadius <= 0) {
                add(findings, Severity::Error, "bad_profile_segment_radius",
                    op.name + " segment " + std::to_string(index) + " end_radius must be positive.");
            }
            if (segment.steps.has_value() && *segment.steps < 1) {
                add(findings, Severity::Error, "bad_profile_segment_steps",
                    op.name + " segment " + std::to_string(index) + " steps must be at least 1.");
            }
        }
        checkMaterial(findings, op.name, op.material);
    }

    void operator()(const CutFlutesOp &op) const
    {
        if (namesSoFar.find(op.target) == namesSoFar.end()) {
            add(findings, Severity::Error, "flute_missing_target",
                "CutFlutes target does not exist before cut: " + op.target);
        }
        // Port addition: flutes are VERTICAL grooves (the cutter ring
        // stands along Z); now that the backends honor cylinder axis, a
        // sideways target would be cut nonsense.
        if (nonVerticalCylinders.find(op.target) != nonVerticalCylinders.end()) {
            add(findings, Severity::Error, "flute_target_not_vertical",
                "CutFlutes cuts vertical grooves; target is not a z-axis cylinder: " + op.target);
        }
        if (op.count < 12) {
            add(findings, Severity::Warning, "low_flute_count",
                "Flute count looks low: " + std::to_string(op.count));
        }
        if (op.depth <= 0) {
            add(findings, Severity::Error, "bad_flute_depth", "Flute depth must be positive.");
        }
        // R1-B04b: width_ratio is the cutter source ONLY when no explicit pair
        // is given; an explicit cutter makes the ratio irrelevant, so it must
        // not be judged (decision 5).
        const bool explicitCutter = op.cutterRadius.has_value() && op.atRadius.has_value();
        if (!explicitCutter && !(op.widthRatio >= 0.05 && op.widthRatio <= 0.9)) {
            // numberKeyText, not std::to_string: "0.34", never "0.340000".
            add(findings, Severity::Warning, "odd_flute_width_ratio",
                "Flute width ratio looks odd: " + numberKeyText(op.widthRatio));
        }
        // Port additions (R1-B04b): explicit cutter geometry must be positive,
        // same grade as bad_flute_depth — a zero/negative cutter ring or seat
        // radius is a degenerate cut.
        if (op.cutterRadius.has_value() && *op.cutterRadius <= 0.0) {
            add(findings, Severity::Error, "bad_cutter_radius",
                "Flute cutter_radius must be positive.");
        }
        if (op.atRadius.has_value() && *op.atRadius <= 0.0) {
            add(findings, Severity::Error, "bad_at_radius",
                "Flute at_radius must be positive.");
        }
        // Port addition: v0 never cross-checked the z range; a reversed
        // range produced a negative cutter depth downstream.
        if (op.startZ.has_value() && op.endZ.has_value() && !(*op.startZ < *op.endZ)) {
            add(findings, Severity::Error, "bad_flute_z_range",
                "Flute start_z must be below end_z for: " + op.target);
        }
    }

    void operator()(const AddLabelOp &) const {}

    void operator()(const ScriptOp &op) const
    {
        // The MANIFEST (param types, ranges, materials) lives in the Python
        // craftsman, invisible to C++ — so the deep checks are the craftsman's
        // own job at build (edi_craft coerces + validates). The one invariant
        // this side CAN enforce is that there is a craftsman to dispatch to.
        if (op.scriptId.empty()) {
            add(findings, Severity::Error, "missing_script_reference",
                op.name + " needs a craftsman script id.");
        }
        // The param bag is opaque, but its KEYS must round-trip: a collision
        // with a built-in or a non-bare key would silently corrupt the file or
        // diverge between the C++ and python readers. Surface it as a named
        // finding (the writer refuses it too) instead of leaving the contract
        // to a comment.
        for (const ScriptParam &param : op.params) {
            const std::string problem = recipeScriptParamKeyProblem(param.key);
            if (!problem.empty()) {
                add(findings, Severity::Error, "bad_param_key", op.name + " " + problem);
            }
        }
    }
};

const std::string *opName(const RecipeOp &op)
{
    struct NameGetter {
        const std::string *operator()(const AddBoxOp &o) const { return &o.name; }
        const std::string *operator()(const AddCylinderOp &o) const { return &o.name; }
        const std::string *operator()(const AddSphereOp &o) const { return &o.name; }
        const std::string *operator()(const AddRingOp &o) const { return &o.name; }
        const std::string *operator()(const AddMouldingOp &o) const { return &o.name; }
        const std::string *operator()(const AddPrismOp &o) const { return &o.name; }
        const std::string *operator()(const AddProfileMouldingOp &o) const { return &o.name; }
        const std::string *operator()(const AddRevolvedProfileOp &o) const { return &o.name; }
        const std::string *operator()(const AddExtrudedProfileOp &o) const { return &o.name; }
        const std::string *operator()(const CutFlutesOp &) const { return nullptr; }
        const std::string *operator()(const AddLabelOp &o) const { return &o.name; }
        const std::string *operator()(const ScriptOp &o) const { return &o.name; }
    };
    return std::visit(NameGetter{}, op);
}

} // namespace

OpValidationReport validateRecipeOps(const std::vector<RecipeOp> &ops)
{
    OpValidationReport report;
    std::unordered_set<std::string> names;
    std::unordered_set<std::string> nonVerticalCylinders;

    for (const RecipeOp &op : ops) {
        // Names register BEFORE the per-op checks of LATER ops: a CutFlutes
        // may target anything already on the bench, nothing after it.
        if (const std::string *name = opName(op); name != nullptr && !name->empty()) {
            if (names.count(*name) > 0) {
                add(report.findings, Severity::Error, "duplicate_name", "Duplicate object name: " + *name);
            }
            names.insert(*name);
        }
        if (const auto *cylinder = std::get_if<AddCylinderOp>(&op);
            cylinder != nullptr && cylinder->axis != Axis::Z) {
            nonVerticalCylinders.insert(cylinder->name);
        }
        std::visit(OpChecker{report.findings, names, nonVerticalCylinders}, op);
    }

    for (const OpFinding &finding : report.findings) {
        if (finding.severity == Severity::Error) {
            report.ok = false;
            break;
        }
    }
    return report;
}

std::vector<OpFinding> lintColumnConventions(const std::vector<RecipeOp> &ops)
{
    std::vector<OpFinding> findings;

    const auto nameStartsWith = [](const RecipeOp &op, const char *prefix) {
        const std::string *name = opName(op);
        return name != nullptr && name->rfind(prefix, 0) == 0;
    };

    bool looksLikeColumn = false;
    for (const RecipeOp &op : ops) {
        if (nameStartsWith(op, "plinth.") || nameStartsWith(op, "shaft.") || nameStartsWith(op, "capital.")) {
            looksLikeColumn = true;
            break;
        }
    }
    if (!looksLikeColumn) {
        return findings;
    }

    // v0's literal expectation, kept verbatim INSIDE the lint: the doric
    // convention names its shaft exactly this. A different column recipe
    // simply skips this lint.
    const AddCylinderOp *shaft = nullptr;
    for (const RecipeOp &op : ops) {
        if (const auto *cylinder = std::get_if<AddCylinderOp>(&op)) {
            if (cylinder->name == "shaft.tapered_fluted_core") {
                shaft = cylinder;
            }
        }
    }
    if (shaft == nullptr) {
        add(findings, Severity::Error, "missing_shaft", "Expected shaft.tapered_fluted_core.");
        return findings;
    }

    double widestBase = 0.0;
    bool hasBaseBox = false;
    bool hasCapital = false;
    for (const RecipeOp &op : ops) {
        if (const auto *box = std::get_if<AddBoxOp>(&op)) {
            if (box->name.rfind("plinth.", 0) == 0) {
                hasBaseBox = true;
                widestBase = std::max(widestBase, std::max(box->width, box->depth));
            }
        }
        if (nameStartsWith(op, "capital.")) {
            hasCapital = true;
        }
    }
    if (hasBaseBox && widestBase <= shaft->radius * 2) {
        add(findings, Severity::Warning, "base_not_wider_than_shaft", "Base should be wider than shaft.");
    }
    if (!hasCapital) {
        add(findings, Severity::Warning, "missing_capital", "No capital parts found.");
    }
    return findings;
}

} // namespace edi::recipe
