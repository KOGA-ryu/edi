#pragma once

// The prototype's doric column (examples/doric_column_recipe_v0.json),
// translated key for key — the shared fixture every port test builds on.
// ONE construction site: the ops tests, the ascii goldens, and the sample
// generators must all describe the same column or the byte-guards lie.

#include "recipe/RecipeOps.h"

inline edi::recipe::RecipeOpStream doricColumnOpStream()
{
    using namespace edi::recipe;
    RecipeOpStream stream;
    stream.id = "doric_column_v0";
    stream.name = "Doric Column";

    AddBoxOp lowerStep;
    lowerStep.name = "plinth.lower_step";
    lowerStep.width = 18;
    lowerStep.depth = 18;
    lowerStep.height = 2.0;
    lowerStep.z = 1.0;
    stream.ops.push_back(lowerStep);

    AddBoxOp upperStep;
    upperStep.name = "plinth.upper_step";
    upperStep.width = 15.5;
    upperStep.depth = 15.5;
    upperStep.height = 2.0;
    upperStep.z = 3.0;
    stream.ops.push_back(upperStep);

    AddProfileMouldingOp baseMoulding;
    baseMoulding.name = "base.torus_scotia_moulding";
    baseMoulding.baseZ = 4.0;
    baseMoulding.sequence = {
        {"fillet", 0.08, 5.55, 5.55, {}, {}},
        {"torus", 0.46, {}, 6.65, {}, {}},
        {"scotia", 0.3, {}, 6.05, {}, {}},
        {"fillet", 0.16, {}, 5.12, {}, {}},
    };
    stream.ops.push_back(baseMoulding);

    AddCylinderOp shaft;
    shaft.name = "shaft.tapered_fluted_core";
    shaft.radius = 5.0;
    shaft.height = 50.4;
    shaft.z = 30.2;
    shaft.vertices = 128;
    shaft.taperTopRadius = 4.3;
    shaft.entasis = true;
    stream.ops.push_back(shaft);

    CutFlutesOp flutes;
    flutes.target = "shaft.tapered_fluted_core";
    flutes.count = 20;
    flutes.depth = 0.45;
    flutes.widthRatio = 0.34;
    flutes.startZ = 6.5;
    flutes.endZ = 53.9;
    stream.ops.push_back(flutes);

    AddProfileMouldingOp necking;
    necking.name = "capital.necking_annuli";
    necking.baseZ = 55.4;
    necking.sequence = {
        {"fillet", 0.25, 4.4, 5.2, {}, {}},
        {"scotia", 0.27, {}, 4.9, {}, {}},
        {"annulet", 0.38, {}, 5.4, {}, {}},
        {"fillet", 0.3, {}, 5.0, {}, {}},
    };
    stream.ops.push_back(necking);

    AddProfileMouldingOp echinusCushion;
    echinusCushion.name = "capital.echinus_cushion";
    echinusCushion.baseZ = 56.6;
    echinusCushion.sequence = {
        {"cavetto", 0.7, 5.0, 6.0, {}, {}},
        {"echinus", 2.35, {}, 7.9, {}, {}},
        {"fillet", 0.95, {}, 6.9, {}, {}},
    };
    stream.ops.push_back(echinusCushion);

    AddBoxOp abacus;
    abacus.name = "capital.abacus_square_slab";
    abacus.width = 16.5;
    abacus.depth = 16.5;
    abacus.height = 3.0;
    abacus.z = 62.1;
    stream.ops.push_back(abacus);

    AddBoxOp entablature;
    entablature.name = "entablature.test_block";
    entablature.width = 22;
    entablature.depth = 18;
    entablature.height = 5;
    entablature.z = 66.1;
    entablature.material = "limestone";
    stream.ops.push_back(entablature);

    return stream;
}
