#pragma once

#include "recipe/RecipeOps.h"

#include <string>
#include <vector>

namespace edi::recipe {

// One editable scalar of an op: its TOML field key and current value. The same
// per-kind member-pointer registry that decides bindability lists these, so the
// human inspector and the resolve/store paths never drift on "which fields."
struct RecipeOpField {
    std::string key;
    double value = 0.0;
};

// The bindable-field registry: which TOML field keys of which op kinds
// may carry a measurement binding, as DATA (per-kind tables of
// fieldKey -> member pointer). The table serves the store's WRITER
// (validate bindings, emit binding keys instead of the literal) and the
// RESOLVE pass (write resolved numbers through the same pointers,
// R1-B03). The store's READER accepts the binding shape through its
// per-type bindableNumber call sites — the same listing style as every
// other field it reads — kept in step with this table by the tests'
// exhaustive registry pin and the all-kinds binding round trip; a reader
// call without a registry row would load a file the writer then refuses
// to save.
//
// Only double-typed fields are bindable. Ints (vertices, count) and
// optionals (taper_top_radius, start_z, end_z) stay literal-only — a
// deliberate narrowing of pipeline A, which let ANY param bind and gated
// whole-numberness after resolve; no use case binds a vertex count to a
// canvas measurement yet, and optionals add presence semantics a binding
// does not model. Widening later is adding rows.

// True when this op kind has a bindable field with this TOML key.
bool opFieldBindable(const RecipeOp &op, const std::string &fieldKey);

// Every editable scalar of this op (key + current value), in registry order —
// what the inspector renders as spinboxes. Reads through the same member-pointer
// table opFieldBindable/setOpFieldValue use, so the three never disagree.
std::vector<RecipeOpField> opFields(const RecipeOp &op);

// Bind op `opIndex`'s field `fieldKey` to drafted object `objectId`'s
// measurement `field` (width/height/length/radius), replacing any binding
// already on that field. False (no change) if the field is not a bindable
// double of that op. The binding picker's create verb.
bool addRecipeBinding(RecipeOpStream &stream, std::size_t opIndex, const std::string &fieldKey,
                      const std::string &objectId, const std::string &field);

// Remove any binding on op `opIndex`'s field `fieldKey` (back to a literal).
void clearRecipeBinding(RecipeOpStream &stream, std::size_t opIndex, const std::string &fieldKey);

// The binding on op `opIndex`'s field `fieldKey`, or nullptr — the inspector
// reads it to show "bound to <object>.<field>" and offer Unbind.
const RecipeFieldBinding *findRecipeBinding(const RecipeOpStream &stream, std::size_t opIndex,
                                            const std::string &fieldKey);

// Writes a value through the registry's member pointer. False (and no
// write) when the field is not bindable on this op — the resolve pass
// turns that into a named refusal.
bool setOpFieldValue(RecipeOp &op, const std::string &fieldKey, double value);

} // namespace edi::recipe
