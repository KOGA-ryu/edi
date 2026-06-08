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
// Must not depend on:
//   - UI widgets.
//   - Command mutation APIs.
//   - Lua runtime.
//
// Preserve later:
//   Inspection should be available before any canonical MessagePack fixture is
//   accepted into the project.
