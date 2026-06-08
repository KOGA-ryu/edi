// TomlWriter.cpp
//
// Implementation responsibility:
//   Converts typed static configuration contracts into TOML text.
//
// Belongs here:
//   - Stable field ordering.
//   - Clear diagnostics for unsupported values.
//   - Formatting choices for human-authored config files.
//
// Must be delegated elsewhere:
//   - File IO policy belongs in higher-level persistence services.
//   - Runtime mutation belongs in app/domain command layers.
//   - Behavior recipes belong in Lua scripting contracts.
//
// Boundary note:
//   This is a projection layer. It should not decide application behavior.
//
// Surface contract:
//   - Primary responsibility: implement typed config projection to TOML text.
//   - Allowed data: typed config values, ordered output fields, and diagnostics.
//   - Call direction: called by save/export services.
//   - Mutation authority: translation only.
//   - Unit convention: preserve typed units in human-readable fields.
//   - Identity policy: deterministic ID field output.
//   - Lifetime: no retained references to input contracts.
//   - Composition boundary: no file IO orchestration.
//   - Promotion path: schema-specific writers can split out when needed.
