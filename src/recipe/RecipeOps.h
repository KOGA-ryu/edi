#pragma once

#include "recipe/RecipeMouldings.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace edi::recipe {

// The build-operation vocabulary, ported from the user's prototype
// (ascii_blender_dryrun_v0/ops.py). One op stream, three interpreters:
// ASCII preview (proof), validators, and the Blender craftsmen library
// (execution) — "Recipe is truth. ASCII preview is proof. Blender script
// is execution."
//
// This is the ARCHITECTURAL CORE of the vocabulary (the ops the doric
// column exercises). The floral family (section stacks, path sweeps,
// petal blooms) ports in its own phase, with its sweep geometry.
//
// Port divergences from v0, deliberate and documented:
// - z_mode and axis are ENUMS, not validated strings: an invalid mode is
//   unrepresentable here and rejected by the TOML loader with a named
//   key, instead of surfacing later as a validator finding.
// - materials validate against a table (v0's backend silently fell back
//   to 'stone' on a typo — guesswork by fallback).
// - entasis carries its ratio as a field (v0 hardcoded 0.045 in the
//   backend); the default preserves v0's numbers exactly.
// - AddRing.overhang widens the ring's radius in BOTH backends, exactly
//   as v0 wired it (radius + overhang). What stays unwired is true torus
//   geometry: the ring is a cylinder alias, so overhang widens the alias
//   rather than overhanging a tube — validators note that when it is set.

// Coordinate convention (v0 ops.py): X left/right, Y depth, Z up.
// Front elevation projects X/Z, side Y/Z, top X/Y.

enum class ZMode { Center, Base };
enum class Axis { X, Y, Z };

struct AddBoxOp {
    std::string name;
    double width = 0.0;
    double depth = 0.0;
    double height = 0.0;
    double z = 0.0;
    double x = 0.0;
    double y = 0.0;
    std::string material = "stone";
    ZMode zMode = ZMode::Center;
};

struct AddCylinderOp {
    std::string name;
    double radius = 0.0;
    double height = 0.0;
    double z = 0.0;
    double x = 0.0;
    double y = 0.0;
    int vertices = 96;
    std::string material = "stone";
    std::optional<double> taperTopRadius;
    bool entasis = false;
    // v0's backend bulge: sin(pi*t) * radius * 0.045. The constant is now
    // recipe data; the default IS v0's number.
    double entasisRatio = 0.045;
    Axis axis = Axis::Z;
    ZMode zMode = ZMode::Center;
};

struct AddSphereOp {
    std::string name;
    double radius = 0.0;
    double z = 0.0;
    double x = 0.0;
    double y = 0.0;
    int vertices = 24;
    std::string material = "stone";
};

struct AddRingOp {
    std::string name;
    double radius = 0.0;
    double tubeHeight = 0.0;
    double z = 0.0;
    double overhang = 0.0; // widens the cylinder-alias radius — see header note
    double x = 0.0;
    double y = 0.0;
    int vertices = 96;
    std::string material = "stone";
};

// Low-level moulding: explicit (z, radius) profile points. AddProfileMoulding
// compiles INTO this via the term compiler (RecipeMouldings).
struct AddMouldingOp {
    std::string name;
    double baseZ = 0.0;
    std::vector<MouldingPoint> profile;
    double x = 0.0;
    double y = 0.0;
    int vertices = 96;
    std::string material = "stone";
};

struct AddProfileMouldingOp {
    std::string name;
    double baseZ = 0.0;
    std::vector<MouldingSegment> sequence;
    double x = 0.0;
    double y = 0.0;
    int vertices = 96;
    std::string material = "stone";
};

// The lathe, op-vocabulary native (R1-B04): a REFERENCE to a drafted
// profile (line/polyline/arc), not a copy of its points — the drafting
// document stays the single measurement authority, and re-resolving after
// a profile edit picks up the new numbers. The resolve pass lowers this
// into an AddMouldingOp with exact physical (radius, z) points via the
// page-left-axis / page-bottom-z convention; downstream passes never see
// it and refuse it by name if they do. Divergence from pipeline A's
// lathe: vertices defaults to the op family's 96, not A's segments=64 —
// one family default, and the sample writes its values explicitly.
struct AddRevolvedProfileOp {
    std::string name;
    std::string profile; // a drafted object's id — the reference
    double baseZ = 0.0;
    double x = 0.0;
    double y = 0.0;
    int vertices = 96;
    std::string material = "stone";
};

struct CutFlutesOp {
    std::string target; // names an earlier op — validated in order
    int count = 0;
    double depth = 0.0;
    double widthRatio = 0.28;
    // Explicit cutter geometry (R1-B04b) — pipeline A's radial_groove
    // semantics: the cutter ring's OWN radius and the radius it rides at.
    // OPTIONAL and present-together (the reader refuses a half pair), and
    // XOR'd with widthRatio (a file carrying both is refused — a ratio the
    // build would ignore is a lie in waiting). When set, the backends place
    // the cutter from these verbatim instead of deriving it from the
    // target's bounds × widthRatio; the benchmark doric flutes can ONLY be
    // specified this way. NOT bindable (optionals stay literal-only, B02
    // decision 3). widthRatio keeps its default/meaning when the pair is
    // absent.
    std::optional<double> cutterRadius;
    std::optional<double> atRadius;
    std::optional<double> startZ;
    std::optional<double> endZ;
};

struct AddLabelOp {
    std::string name;
    std::string text;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

using RecipeOp = std::variant<
    AddBoxOp,
    AddCylinderOp,
    AddSphereOp,
    AddRingOp,
    AddMouldingOp,
    AddProfileMouldingOp,
    AddRevolvedProfileOp,
    CutFlutesOp,
    AddLabelOp>;

// A measurement binding: "this op's field comes from that drafted
// object's measurement" — pipeline A's crown jewel, carried over
// (docs/recipe_binding_contract.md). Bindings live BESIDE the ops as a
// parallel table, not inside them: the ops' doubles stay the RESOLVED
// values every consumer already reads, and until resolution runs the
// gates refuse a stream whose binding table is non-empty. The losing
// alternative — a literal|binding sum type on every field — would have
// rewritten every consumer (compile, validators, both ASCII backends,
// python) to protect against a state the gates refuse anyway.
struct RecipeFieldBinding {
    std::size_t opIndex = 0;
    std::string fieldKey; // the TOML spelling: "width", "entasis_ratio", …
    std::string objectId; // the drafted object's immutable id
    std::string field;    // "width" / "height" / "length" / "radius"
};

struct RecipeOpStream {
    std::string id;
    std::string name;
    std::vector<RecipeOp> ops;
    std::vector<RecipeFieldBinding> bindings;
};

// The material vocabulary (v0's two + its README's planned set). A table,
// not a free string: v0's MATS.get(key, 'stone') silently repainted typos.
bool recipeMaterialSupported(const std::string &material);
const std::vector<std::string> &recipeMaterialTable();

// v0 op-type names kept verbatim ("AddBox", "CutFlutes") — the user's
// vocabulary, not re-invented. Empty string for unknown.
const char *recipeOpTypeName(const RecipeOp &op);

// The op types the human's step PALETTE offers — the primitives that are valid
// from a single click (unit starter dimensions). Mouldings need a term
// sequence, the lathe a drafted-profile reference, flutes a target op, so those
// are authored, not one-click-appended (yet).
const std::vector<std::string> &recipePaletteOpTypes();

// A new op of the named palette type with unit starter dimensions and `name`,
// or nullopt for a type the palette does not offer. The "string scripts by
// clicking" factory: the vocabulary owns what a fresh step looks like.
std::optional<RecipeOp> makeRecipeOp(const std::string &typeName, const std::string &name);

// Remove the op at `index`, keeping the binding table consistent: bindings on
// that op are dropped, and bindings on later ops shift their opIndex down. A
// no-op if the index is out of range. (CutFlutes targets ops by NAME, which is
// stable, so only the index-keyed bindings need fixing up.)
void removeRecipeOp(RecipeOpStream &stream, std::size_t index);

// Move the op at `from` to `to`, remapping every binding's opIndex through the
// same move so a field stays bound to its op. A no-op if either index is out of
// range or they are equal.
void moveRecipeOp(RecipeOpStream &stream, std::size_t from, std::size_t to);

// Compile pass: every AddProfileMouldingOp expands to an AddMouldingOp via
// the term compiler; all other ops pass through unchanged. Rejection names
// the op (the compiler's own pointable message).
struct RecipeCompileResult {
    bool ok = false;
    std::string message;
    std::vector<RecipeOp> ops;
};
RecipeCompileResult compileRecipeOps(const std::vector<RecipeOp> &ops);

} // namespace edi::recipe
