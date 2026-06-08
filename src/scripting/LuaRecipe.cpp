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
