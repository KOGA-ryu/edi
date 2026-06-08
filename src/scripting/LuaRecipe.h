// LuaRecipe.h
//
// Purpose:
//   Reserves the authored Lua recipe contract for future scripting support.
//
// Expected contracts:
//   - Recipe ID, name, version, and description.
//   - Required capabilities.
//   - Source reference or embedded source handle.
//   - Validation result and diagnostics.
//
// Ownership rule:
//   LuaRecipe owns recipe metadata and validation shape. It does not own app
//   state, document storage, or mutation.
//
// Preserve later:
//   Lua is for authored behavior and composition. It must not become direct
//   state ownership.
//
// Surface contract:
//   - Primary responsibility: declare authored recipe metadata and validation
//     shapes.
//   - Allowed data: recipe ID/name/version, description, capability list, source
//     reference, authoring metadata, and validation diagnostics.
//   - Call direction: scripting loaders and settings UI may inspect recipes;
//     bridge code consumes validated recipes.
//   - Mutation authority: metadata only.
//   - Unit convention: no drafting/text units unless recipe metadata declares
//     required capabilities.
//   - Identity policy: recipe ID/version identify authored behavior.
//   - Lifetime: recipe metadata owns descriptive values, not VM state.
//   - Composition boundary: recipe description stays separate from execution.
//   - Promotion path: Lua VM bindings can layer on validated recipes later.
