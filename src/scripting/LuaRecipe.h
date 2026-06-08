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
// Must not depend on:
//   - Drafting/text private stores.
//   - UI widgets.
//   - Raw format parser objects.
//
// Preserve later:
//   Lua is for authored behavior and composition. It must not become direct
//   state ownership.
