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

struct CutFlutesOp {
    std::string target; // names an earlier op — validated in order
    int count = 0;
    double depth = 0.0;
    double widthRatio = 0.28;
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
