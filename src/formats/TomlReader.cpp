// TomlReader.cpp
//
// Implementation responsibility:
//   Implements TOML parser adapters once the TOML dependency/reader strategy is
//   chosen.
//
// Belongs here:
//   - Parsing TOML text/bytes.
//   - Validating required static configuration fields.
//   - Converting parsed values into typed C++ contracts.
//   - Returning FormatResult diagnostics.
//
// Must be delegated elsewhere:
//   - App state mutation belongs in app/controller layers.
//   - Drafting/text command mutation belongs in domain command layers.
//   - Lua behavior belongs in scripting.
//
// Boundary note:
//   Parsed TOML objects should not escape this adapter.
