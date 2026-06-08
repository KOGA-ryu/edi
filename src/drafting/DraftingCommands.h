// DraftingCommands.h
//
// Purpose:
//   Declares the drafting command contract: user/script intent in, validated
//   document mutation out.
//
// Expected contracts:
//   - Create object.
//   - Delete object.
//   - Move object or objects.
//   - Update edit handle.
//   - Update metadata/style.
//   - Select, toggle, clear, or replace selection.
//   - Command result with accepted/rejected status and diagnostic text.
//
// Ownership rule:
//   Commands own intent validation and mutation sequencing. The store owns raw
//   storage invariants. Geometry owns math.
//
// Preserve later:
//   Commands are the boundary that scripting and UI should call. They should be
//   narrow, explicit, and replayable.
//
// Surface contract:
//   - Primary responsibility: declare drafting command requests and results.
//   - Allowed data: command kind, target IDs, command payloads, selection edits,
//     movement vectors, handle update requests, and command diagnostics.
//   - Call direction: UI/controllers/scripts create command requests; command
//     implementation calls store, geometry, selection, and measurement helpers.
//   - Mutation authority: command execution is the public mutation gate.
//   - Unit convention: command payloads state their unit explicitly; drafting
//     geometry defaults to document-space values.
//   - Identity policy: commands address stable IDs, not vector indexes.
//   - Lifetime: commands are value records suitable for replay/undo history.
//   - Composition boundary: commands combine intent with store operations.
//   - Promotion path: undo/redo can wrap these value command records later.
