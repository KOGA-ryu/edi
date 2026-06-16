#pragma once

#include "recipe/RecipeOps.h"

#include <string>
#include <variant>
#include <vector>

namespace edi::recipe {

// The full scalar-editing schema for the human op inspector — everything
// opFields (the binding registry's doubles) does NOT cover: ints, bools, the
// z_mode/axis enums, the material vocabulary, and the string fields. Together
// with opFields it is every scalar the inspector renders.
//
// Data-oriented like the rest: a visitor lists each op's fields with their kind
// and current value; a second visitor sets one by key. Arrays (moulding
// profiles/sequences) and the optional fields (taper, cutter geometry) stay out
// — they carry presence/shape semantics a scalar editor cannot express yet.

enum class RecipeFieldKind { Number, Integer, Boolean, Choice, Text };

struct RecipeOpScalar {
    std::string key;                  // TOML spelling ("z_mode", "material")
    std::string label;                // human label ("Z Mode")
    RecipeFieldKind kind = RecipeFieldKind::Number;
    double number = 0.0;              // Number value
    int integer = 0;                  // Integer value
    bool boolean = false;             // Boolean value
    std::string text;                 // Text + Choice current value
    std::vector<std::string> choices; // Choice options (enum/material)
    bool editable = true;             // false = a read-only reference (revolved profile id)
};

// Every editable scalar of the op, in display order: the binding registry's
// doubles first (via opFields), then the op's ints/bools/enums/material/strings.
std::vector<RecipeOpScalar> opEditableScalars(const RecipeOp &op);

// Set one scalar by key from a typed value. Returns false if the key is not a
// scalar of this op or the value's type does not match the field's kind.
using RecipeScalarValue = std::variant<double, int, bool, std::string>;
bool setOpScalar(RecipeOp &op, const std::string &key, const RecipeScalarValue &value);

} // namespace edi::recipe
