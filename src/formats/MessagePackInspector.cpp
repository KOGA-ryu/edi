// MessagePackInspector.cpp
//
// Implementation responsibility:
//   Produces compact, human-readable summaries of MessagePack data.
//
// Belongs here:
//   - Schema/version summary.
//   - Counts and IDs.
//   - Structural warnings.
//   - Safe diagnostics for review and tooling.
//
// Must be delegated elsewhere:
//   - Full document loading belongs in MessagePackReader.
//   - Domain mutation belongs in commands.
//   - UI presentation belongs in widgets/tools.
//
// Boundary note:
//   Inspect first, load later. This protects binary fixtures from becoming
//   opaque project state.
