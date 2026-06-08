// ToonExport.h
//
// Purpose:
//   Declares TOON export adapters for AI-facing context and handoff packets.
//
// Expected contracts:
//   - Export planning packets.
//   - Export review packets.
//   - Export agent handoff bundles.
//   - Export compact document/context summaries.
//
// Ownership rule:
//   TOON export owns communication shape only. It is not app truth, not project
//   persistence, and not command input authority.
//
// Must not depend on:
//   - UI widgets.
//   - Raw mutable stores.
//   - Lua runtime.
//
// Preserve later:
//   TOON should be generated from typed summaries, never from raw storage or
//   hidden UI state.
