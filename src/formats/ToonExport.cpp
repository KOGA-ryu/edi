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
//
// Surface contract:
//   - Primary responsibility: implement compact TOON projections.
//   - Allowed data: typed summaries, selected fields, packet labels, and output
//     formatting options.
//   - Call direction: called by export/review/handoff tooling.
//   - Mutation authority: export only.
//   - Unit convention: preserve explicit units from source summaries.
//   - Identity policy: include IDs as references, not storage handles.
//   - Lifetime: no retained source state.
//   - Composition boundary: no AI decision logic here.
//   - Promotion path: streaming exporters can be added for large packets.
