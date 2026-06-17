#pragma once

#include "recipe/RecipeOps.h"

#include <string>
#include <vector>

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

// The named-recipe LIBRARY: a directory of reusable `*.ops.toml` recipes (strict
// TOML, never JSON — reusing recipeOpsToToml/recipeOpsFromToml). The filename is
// the recipe's name (or id when name is empty), sanitized to bare chars so it is
// a safe, deterministic path. These are thin free functions over std::filesystem;
// parse/IO failures propagate via the existing result type (ok=false + message).

// Write `stream` to <dirPath>/<sanitized name-or-id>.ops.toml (creating dirPath
// if needed). On success, `text` carries the written TOML.
OpStreamTextResult saveLibraryRecipe(const std::string &dirPath, const RecipeOpStream &stream);

// Read <dirPath>/<name>.ops.toml back into a stream (`name` is the bare recipe
// name, no extension). A missing file or parse error fails with a named message.
OpStreamParseResult loadLibraryRecipe(const std::string &dirPath, const std::string &name);

// The bare names (no `.ops.toml` extension) of the recipes in `dirPath`, sorted.
// A missing directory yields an empty list (not an error).
std::vector<std::string> listLibraryRecipes(const std::string &dirPath);

} // namespace edi::recipe
