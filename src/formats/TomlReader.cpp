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
//
// Surface contract:
//   - Primary responsibility: implement TOML parsing and typed conversion.
//   - Allowed data: parser output, schema labels, typed config constructors, and
//     FormatResult diagnostics.
//   - Call direction: called by settings/project load services.
//   - Mutation authority: translation only.
//   - Unit convention: convert textual units into typed config units.
//   - Identity policy: preserve declared IDs as typed values.
//   - Lifetime: parser data is temporary; returned contracts are owned values.
//   - Composition boundary: no runtime application of settings here.
//   - Promotion path: schema-specific readers can move into separate files.
