// MessagePackInspector.h
//
// Purpose:
//   Declares inspection helpers for MessagePack fixtures and documents.
//
// Expected contracts:
//   - Inspect schema name.
//   - Inspect schema version.
//   - Report object/document counts.
//   - Report byte size and key identifiers.
//   - Report warnings before a binary file is accepted as canonical.
//
// Ownership rule:
//   Inspector owns human-readable diagnostics and summaries. It does not load
//   binary data into app state as truth.
//
// Preserve later:
//   Inspection should be available before any canonical MessagePack fixture is
//   accepted into the project.
//
// Surface contract:
//   - Primary responsibility: declare safe MessagePack summary/inspection APIs.
//   - Allowed data: byte buffers, source labels, schema names, versions, counts,
//     IDs, sizes, and warning records.
//   - Call direction: tooling and review workflows call inspector before reader.
//   - Mutation authority: inspect-only.
//   - Unit convention: reports encoded units as labels, not applied values.
//   - Identity policy: exposes IDs for review without loading full state.
//   - Lifetime: returns owned summaries.
//   - Composition boundary: inspector summarizes; reader constructs typed state.
//   - Promotion path: add diff/pretty-unpack views here before accepting more
//     binary fixtures.
