// MessagePackReader.cpp
//
// Implementation responsibility:
//   Converts MessagePack bytes into typed C++ contracts with diagnostics.
//
// Belongs here:
//   - Schema/version validation.
//   - Required field validation.
//   - Converting machine records into drafting/text/domain contracts.
//   - Returning FormatResult errors for unsupported or unsafe data.
//
// Must be delegated elsewhere:
//   - App/domain mutation belongs in command layers.
//   - Human-authored static config belongs in TOML adapters.
//   - AI handoff belongs in TOON export.
//
// Boundary note:
//   Binary compactness is not permission to skip explicit contracts.
//
// Surface contract:
//   - Primary responsibility: implement MessagePack decoding into typed values.
//   - Allowed data: decoded map/array structures, schema metadata, typed output
//     builders, and diagnostics.
//   - Call direction: called by machine-state load/replay tools.
//   - Mutation authority: translation only.
//   - Unit convention: decode units into typed C++ fields before returning.
//   - Identity policy: preserve stable IDs and keep indexes internal.
//   - Lifetime: decoder objects are temporary.
//   - Composition boundary: no command execution during read.
//   - Promotion path: hot binary paths can get specialized decoders later.
