// ToonExport.cpp
//
// Implementation responsibility:
//   Converts typed project/text/drafting summaries into TOON strings for
//   AI-facing handoff and review.
//
// Belongs here:
//   - Stable packet formatting.
//   - Compact summary projection.
//   - Diagnostics when required summary fields are missing.
//
// Must be delegated elsewhere:
//   - Domain mutation belongs in commands.
//   - Canonical persistence belongs in TOML/MessagePack adapters as appropriate.
//   - Script behavior belongs in Lua contracts.
//
// Boundary note:
//   TOON is output/communication. It should not become durable app state.
