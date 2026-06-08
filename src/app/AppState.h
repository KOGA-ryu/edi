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
// Must not depend on:
//   - Qt Widgets or UI controls.
//   - TOML, TOON, MessagePack, Lua, or raw format parser types.
//   - Drafting/text mutation internals.
//
// Preserve later:
//   Keep this file small. If it starts knowing how drawing objects, text ranges,
//   or file formats work, that logic belongs in a lower domain layer.
