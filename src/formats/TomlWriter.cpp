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
