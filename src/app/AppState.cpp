// AppState.cpp
//
// Implementation responsibility:
//   Provides construction and small state-transition helpers for AppState.
//
// Belongs here:
//   - Default app/session state.
//   - Switching workspace modes.
//   - Marking top-level session dirty/clean when domain layers report changes.
//
// Must be delegated elsewhere:
//   - Drafting object mutation belongs in drafting commands/store.
//   - Text edits belong in text editor commands/store.
//   - File parsing/writing belongs in format adapters.
//   - UI event handling belongs in widgets/controllers.
//
// Boundary note:
//   This file should coordinate state references only. It should not become a
//   hidden command engine.
//
// Surface contract:
//   - Primary responsibility: implement simple AppState transitions.
//   - Allowed data: previous AppState, desired workspace mode, active document
//     IDs, and top-level dirty/status values.
//   - Call direction: called by app shell/controller code after domain commands
//     complete or after workspace navigation changes.
//   - Mutation authority: may produce updated AppState values; may not mutate
//     project documents directly.
//   - Unit convention: no pixels, document coordinates, or text offsets.
//   - Identity policy: preserves IDs as opaque handles.
//   - Lifetime: no ownership of documents, stores, widgets, or format buffers.
//   - Composition boundary: app coordination stays separate from persistence.
//   - Promotion path: larger state transitions can move into an AppController
//     while this file remains the value/helper implementation.
