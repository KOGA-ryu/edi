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
