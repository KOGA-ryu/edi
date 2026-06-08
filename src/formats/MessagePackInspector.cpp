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
//
// Surface contract:
//   - Primary responsibility: implement safe binary inspection summaries.
//   - Allowed data: decoded top-level metadata, counts, IDs, sizes, and summary
//     diagnostics.
//   - Call direction: called by CLI tools, review tooling, and load preflights.
//   - Mutation authority: inspect-only.
//   - Unit convention: reports units textually without applying conversions.
//   - Identity policy: surfaces IDs for humans/tools to verify.
//   - Lifetime: no retained decoded state.
//   - Composition boundary: no full document construction.
//   - Promotion path: canonical fixture reports can be built from this.
