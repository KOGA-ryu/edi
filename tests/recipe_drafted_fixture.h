#pragma once

// The DRAFTED doric column — pipeline A's acceptance benchmark, expressed
// in the surviving vocabulary (R1-B06). This is the ONE construction site
// for samples/doric_column/doric_column_drafted_ops.toml and its resolved
// artifact: the committed files are byte-compared against this fixture by
// recipe_drafted_column_tests, and the resolved numbers are pinned against
// a probe of pipeline A's resolveRecipe taken immediately before A's
// retirement (same .edidraw, same default 12x12 grid).
//
// The profiles are REFERENCES into doric_column_profiles.edidraw — the
// drafted measurement authority. Editing a profile vertex and re-resolving
// changes the column; this file holds only what was TYPED, never what was
// measured.

#include "recipe/RecipeOps.h"

namespace edi::recipe {

inline RecipeOpStream doricDraftedOpStream()
{
    RecipeOpStream stream;
    stream.id = "doric_column_drafted";
    stream.name = "Doric Column (drafted profiles)";

    AddBoxOp lowerStep;
    lowerStep.name = "plinth.lower_step";
    lowerStep.width = 3.0;
    lowerStep.depth = 3.0;
    lowerStep.height = 0.48;
    lowerStep.z = 0.0;
    lowerStep.zMode = ZMode::Base; // pipeline A's loc_z placed by BOTTOM
    stream.ops.push_back(lowerStep);

    AddBoxOp upperStep;
    upperStep.name = "plinth.upper_step";
    upperStep.width = 2.64;
    upperStep.depth = 2.64;
    upperStep.height = 0.36;
    upperStep.z = 0.48;
    upperStep.zMode = ZMode::Base;
    stream.ops.push_back(upperStep);

    AddRevolvedProfileOp baseCove;
    baseCove.name = "base.cove";
    baseCove.profile = "base_cove"; // drafted polyline: r 1.32 -> 1.056
    baseCove.vertices = 64;         // pipeline A's lathe segments, kept
    stream.ops.push_back(baseCove);

    AddRevolvedProfileOp shaft;
    shaft.name = "shaft.turned";
    shaft.profile = "shaft"; // drafted polyline: r 1.056 -> 0.792, entasis drafted IN
    shaft.vertices = 64;
    stream.ops.push_back(shaft);

    CutFlutesOp flutes;
    flutes.target = "shaft.turned";
    flutes.count = 20;
    flutes.depth = 0.12;
    // The explicit cutter (R1-B04b) — A's radial_groove numbers verbatim;
    // the width_ratio derivation cannot express this geometry.
    flutes.cutterRadius = 0.16;
    flutes.atRadius = 1.056;
    flutes.startZ = 1.2;
    flutes.endZ = 6.0;
    stream.ops.push_back(flutes);

    AddRevolvedProfileOp echinus;
    echinus.name = "capital.echinus";
    echinus.profile = "echinus"; // drafted polyline: r 0.792 -> 1.32
    echinus.vertices = 64;
    stream.ops.push_back(echinus);

    AddBoxOp abacusStep;
    abacusStep.name = "capital.abacus_step";
    abacusStep.width = 2.64;
    abacusStep.depth = 2.64;
    abacusStep.height = 0.24;
    abacusStep.z = 8.4;
    abacusStep.zMode = ZMode::Base;
    stream.ops.push_back(abacusStep);

    AddBoxOp abacus;
    abacus.name = "capital.abacus";
    abacus.width = 3.36;
    abacus.depth = 3.36;
    abacus.height = 0.96;
    abacus.z = 8.64;
    abacus.zMode = ZMode::Base;
    stream.ops.push_back(abacus);

    return stream;
}

} // namespace edi::recipe
