#include "recipe/RecipeOpsValidate.h"

#include <algorithm>
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

// The per-op checks, v0 codes and wording preserved. A visitor over the
// variant — the same dispatch shape the drafting commands use.
struct OpChecker {
    std::vector<OpFinding> &findings;
    const std::unordered_set<std::string> &namesSoFar;

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
            // Declared in v0's schema, never read by its backend. Carried,
            // but a recipe relying on it deserves to know it does nothing
            // until the craftsmen library wires it.
            add(findings, Severity::Warning, "ring_overhang_unwired",
                op.name + " sets overhang, which no backend honors yet.");
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
        bool monotonicReported = false;
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
            if (index > 0 && point.z < op.profile[index - 1].z && !monotonicReported) {
                add(findings, Severity::Error, "moulding_profile_not_monotonic",
                    op.name + " profile z values must rise in order.");
                monotonicReported = true;
            }
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
        if (op.count < 12) {
            add(findings, Severity::Warning, "low_flute_count",
                "Flute count looks low: " + std::to_string(op.count));
        }
        if (op.depth <= 0) {
            add(findings, Severity::Error, "bad_flute_depth", "Flute depth must be positive.");
        }
        if (!(op.widthRatio >= 0.05 && op.widthRatio <= 0.9)) {
            add(findings, Severity::Warning, "odd_flute_width_ratio",
                "Flute width ratio looks odd: " + std::to_string(op.widthRatio));
        }
        // Port addition: v0 never cross-checked the z range; a reversed
        // range produced a negative cutter depth downstream.
        if (op.startZ.has_value() && op.endZ.has_value() && !(*op.startZ < *op.endZ)) {
            add(findings, Severity::Error, "bad_flute_z_range",
                "Flute start_z must be below end_z for: " + op.target);
        }
    }

    void operator()(const AddLabelOp &) const {}
};

const std::string *opName(const RecipeOp &op)
{
    struct NameGetter {
        const std::string *operator()(const AddBoxOp &o) const { return &o.name; }
        const std::string *operator()(const AddCylinderOp &o) const { return &o.name; }
        const std::string *operator()(const AddSphereOp &o) const { return &o.name; }
        const std::string *operator()(const AddRingOp &o) const { return &o.name; }
        const std::string *operator()(const AddMouldingOp &o) const { return &o.name; }
        const std::string *operator()(const AddProfileMouldingOp &o) const { return &o.name; }
        const std::string *operator()(const CutFlutesOp &) const { return nullptr; }
        const std::string *operator()(const AddLabelOp &o) const { return &o.name; }
    };
    return std::visit(NameGetter{}, op);
}

} // namespace

OpValidationReport validateRecipeOps(const std::vector<RecipeOp> &ops)
{
    OpValidationReport report;
    std::unordered_set<std::string> names;

    for (const RecipeOp &op : ops) {
        // Names register BEFORE the per-op checks of LATER ops: a CutFlutes
        // may target anything already on the bench, nothing after it.
        if (const std::string *name = opName(op); name != nullptr && !name->empty()) {
            if (names.count(*name) > 0) {
                add(report.findings, Severity::Error, "duplicate_name", "Duplicate object name: " + *name);
            }
            names.insert(*name);
        }
        std::visit(OpChecker{report.findings, names}, op);
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
