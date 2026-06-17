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
    // BL-06: how many degrees of arc the ring set sweeps. 360 = a full
    // revolve (today's behavior, the byte-preserving default); < 360 is a
    // partial revolve (arch / apse niche), capped at both open ends.
    double sweepDegrees = 360.0;
};

// One planar footprint vertex, physical x/y in the drawing plane.
struct PrismPoint {
    double x = 0.0;
    double y = 0.0;
};

// The BUILDABLE prism carrier (BL-03): the concrete lowered form an
// AddExtrudedProfileOp resolves to, exactly parallel to how the lathe
// (AddRevolvedProfileOp) lowers to AddMouldingOp. It carries a closed planar
// `footprint` (physical x/y, like AddMoulding carries a (z, radius) point
// vector) swept straight up +z by `height` from `baseZ`. Unlike the profile-
// REFERENCE ops it is NOT refused before build — it passes compile, preview,
// and export like AddMoulding; its 3D proof is the OBJ mesh (BL-04), and like
// ScriptOp it draws nothing in 2D ASCII.
struct AddPrismOp {
    std::string name;
    std::vector<PrismPoint> footprint; // closed planar loop, physical x/y
    double height = 0.0;
    double baseZ = 0.0;
    double x = 0.0;
    double y = 0.0;
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
    // BL-06: partial-angle revolve (arch / apse niche). 360 = full revolve
    // (the behavior-preserving default). SURVIVES lowering — copied onto the
    // AddMouldingOp this lowers to, since the build runs on the moulding.
    double sweepDegrees = 360.0;
};

// The extrude, op-vocabulary native (BL-01): like the lathe a REFERENCE to a
// drafted profile (its closed cross-section), not a copy of its points — the
// drafting document stays the single measurement authority. Where the lathe
// REVOLVES the profile around an axis, this PRISMATICALLY extrudes it `height`
// up the Z axis from `baseZ`, placed at (x, y). The resolve pass will lower it
// (BL-03, not this slice); until then it is authored-only and refused by every
// buildable consumer by name, exactly like AddRevolvedProfileOp. `height` has
// no natural default (a zero-height extrude is degenerate — validate refuses
// it); a NEGATIVE height is allowed (it becomes a push/pull cut in BL-05).
struct AddExtrudedProfileOp {
    std::string name;
    std::string profile; // a drafted object's id — the reference, like the lathe
    double height = 0.0;
    double baseZ = 0.0;
    double x = 0.0;
    double y = 0.0;
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

// One untyped craftsman parameter: a TOML key and its RAW string value. The
// type lives in the craftsman's MANIFEST (radius=number, sides=integer,
// material=…), not here — C++ cannot see the Python script's schema, so it
// carries the value verbatim and the craftsman coerces it (float()/int()/…)
// at build/proof time. The inspector overlays the manifest types later.
struct ScriptParam {
    std::string key;
    std::string value;
};

// A CUSTOM CRAFTSMAN step: dispatch to a user Python script (tools/blender/
// craftsmen/<id>.py) that the built-in op vocabulary cannot express. The
// shape mirrors edi_craft.py's `Script` op EXACTLY — a craftsman id, a
// placement (x/y/z, bindable like every other op), and a free param bag —
// so a stream this side writes is one the craftsmen library reads. The bag
// is generic on purpose: parse-time knows the position and the id; the
// per-craftsman schema (the MANIFEST) types the rest. Param keys are
// constrained to flat TOML bare keys distinct from the built-ins
// (type/script/name/x/y/z) — see recipeScriptParamKeyProblem, enforced at
// read, write, AND validate so the C++ and python halves never diverge.
struct ScriptOp {
    std::string name;     // defaults to scriptId on load, as the python half does
    std::string scriptId; // the craftsman's MANIFEST id ("twisted_column")
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::vector<ScriptParam> params;
};

using RecipeOp = std::variant<
    AddBoxOp,
    AddCylinderOp,
    AddSphereOp,
    AddRingOp,
    AddMouldingOp,
    AddPrismOp,
    AddProfileMouldingOp,
    AddRevolvedProfileOp,
    AddExtrudedProfileOp,
    CutFlutesOp,
    AddLabelOp,
    ScriptOp>;

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

// A craftsman param key must be a flat TOML BARE key (letters, digits, '_'
// or '-') that does NOT collide with the Script op's built-in keys
// (type/script/name/x/y/z). Anything else cannot round-trip: a '.' nests
// into a table under the python half's tomllib (so the two readers hand the
// craftsman structurally different params), a space/'='/'#' makes the
// emitted line unreadable, and a built-in collision overwrites that field on
// write. Returns "" when the key is valid, else a one-line reason naming the
// problem — so the strict reader, the writer (B02), and the validator all
// refuse the same keys by name instead of leaving the contract to a comment.
std::string recipeScriptParamKeyProblem(const std::string &key);

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
