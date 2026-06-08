// LuaRecipe.cpp
//
// Implementation responsibility:
//   Implements Lua recipe metadata construction and validation helpers.
//
// Belongs here:
//   - Checking recipe identity fields.
//   - Checking declared capability names.
//   - Reporting validation diagnostics before execution is possible.
//
// Must be delegated elsewhere:
//   - Lua VM integration belongs in a later runtime adapter.
//   - Command translation belongs in ScriptCommandBridge.
//   - Domain mutation belongs in drafting/text command layers.
//
// Boundary note:
//   Recipes are authored plans. They are not trusted mutation authority.
//
// Surface contract:
//   - Primary responsibility: implement recipe metadata helpers.
//   - Allowed data: recipe metadata values, capability declarations, source
//     references, and diagnostics.
//   - Call direction: called by recipe loaders, settings views, and bridge
//     preflight code.
//   - Mutation authority: metadata construction/normalization only.
//   - Unit convention: no geometry/text unit conversion.
//   - Identity policy: recipe ID and version remain stable.
//   - Lifetime: no Lua VM ownership.
//   - Composition boundary: validation does not execute behavior.
//   - Promotion path: capability registry lookups can attach here later.
