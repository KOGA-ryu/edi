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
// Preserve later:
//   TOON should be generated from typed summaries, never from raw storage or
//   hidden UI state.
//
// Surface contract:
//   - Primary responsibility: declare typed-summary to TOON export adapters.
//   - Allowed data: planning summaries, review summaries, handoff summaries,
//     document excerpts, and compact context records.
//   - Call direction: planning/export services call TOON export.
//   - Mutation authority: export only.
//   - Unit convention: summaries should carry explicit units when measurements
//     are included.
//   - Identity policy: stable IDs may be included as references for agents.
//   - Lifetime: returns owned text output.
//   - Composition boundary: TOON describes context; it does not own project
//     state.
//   - Promotion path: packet-specific exporters can split by audience.
