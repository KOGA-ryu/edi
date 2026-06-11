#pragma once

#include "recipe/RecipeOps.h"

#include <string>

namespace edi::recipe {

// Op streams <-> strict TOML, the same contract the shaper recipes set:
// flat numbered keys (op.N.field, op.N.seq.K.field), every present key
// audited, every failure naming its offender. The prototype's JSON recipes
// translate 1:1 into this form — same op names, same field names — minus
// JSON (forbidden here) and minus its lenient corners (unknown keys raised
// a bare TypeError; here they are named; ambiguous radius keys are
// rejected instead of silently resolved).

struct OpStreamTextResult {
    bool ok = false;
    std::string text;
    std::string message;
};

OpStreamTextResult recipeOpsToToml(const RecipeOpStream &stream);

struct OpStreamParseResult {
    bool ok = false;
    RecipeOpStream stream;
    std::string message;
};

OpStreamParseResult recipeOpsFromToml(const std::string &text, const std::string &source = {});

} // namespace edi::recipe
