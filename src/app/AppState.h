// AppState.h
//
// Purpose:
//   Defines the top-level in-memory application/session state for EDI.
//   This is the app coordinator contract, not a persistence or UI contract.
//
// Expected contracts:
//   - Workspace mode, such as drafting, text, or planning.
//   - Active project/workspace identifier.
//   - Active drafting document and active text document references.
//   - Dirty/session flags and lightweight status state.
//
// Ownership rule:
//   AppState owns which workspace surface is active and which domain document is
//   selected. It does not own the contents of drafting or text documents.
//
// Preserve later:
//   Keep this file small. If it starts knowing how drawing objects, text ranges,
//   or file formats work, that logic belongs in a lower domain layer.
//
// Surface contract:
//   - Primary responsibility: describe the current application session at the
//     level of active mode, active workspace, and active document IDs.
//   - Allowed data: stable IDs, enum modes, dirty flags, lightweight status
//     strings, and references to public project/document handles.
//   - Call direction: controllers and top-level widgets may read/update this
//     state; this state should call only public app/workspace helpers.
//   - Mutation authority: coordinates session-level state only.
//   - Unit convention: no geometry or text units; this layer stores references.
//   - Identity policy: stable string/value IDs instead of raw widget pointers.
//   - Lifetime: owns small state values; borrows domain content by ID.
//   - Composition boundary: combines active surface choices, not domain logic.
//   - Promotion path: if multiple windows/sessions arrive, split per-window
//     session state from durable project state.
