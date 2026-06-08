// MessagePackWriter.h
//
// Purpose:
//   Declares MessagePack writers for compact machine-owned state.
//
// Expected contracts:
//   - Write drafting documents.
//   - Write canvas object snapshots.
//   - Write undo/replay fixtures.
//   - Write binary golden fixtures only with inspectable schema metadata.
//
// Ownership rule:
//   MessagePack writer projects typed contracts into bytes. It does not decide
//   domain behavior or mutate runtime state.
//
// Must not depend on:
//   - UI widgets.
//   - Lua runtime.
//   - Raw private store internals.
//
// Preserve later:
//   Every canonical binary output should be inspectable before it is trusted.
