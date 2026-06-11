#pragma once

#include "recipe/RecipeOps.h"

#include <string>
#include <vector>

namespace edi::recipe {

// The validation pass, ported from v0's validators.py — in its own words
// "deliberately boring and blunt. It should catch the dumb failures before
// Blender gets involved." A REPORT of findings, not a first-error: a build
// plan wants every problem named in one pass.

struct OpFinding {
    enum class Severity { Error, Warning };
    Severity severity = Severity::Error;
    std::string code;    // stable, machine-pointable ("bad_flute_depth")
    std::string message; // human diagnosis, v0 wording kept
};

struct OpValidationReport {
    bool ok = true; // false when any finding is an error (warnings pass)
    std::vector<OpFinding> findings;
};

// Generic per-op checks + cross-op checks (duplicate names, CutFlutes
// targets must exist BEFORE the cut — order is part of the contract).
// Port divergences: unknown materials are errors (v0 silently fell back
// to stone); a set AddRing.overhang warns (declared in v0, never wired).
OpValidationReport validateRecipeOps(const std::vector<RecipeOp> &ops);

// v0 baked column-shaped expectations (a literal "shaft.tapered_fluted_core"
// lookup) into the core validator — prototype convention, not law. Ported
// faithfully but QUARANTINED as an opt-in lint so non-column recipes are
// never judged by a column's conventions.
std::vector<OpFinding> lintColumnConventions(const std::vector<RecipeOp> &ops);

} // namespace edi::recipe
