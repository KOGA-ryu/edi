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
