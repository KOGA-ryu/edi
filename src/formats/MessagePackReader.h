// MessagePackReader.h
//
// Purpose:
//   Declares MessagePack readers for compact machine-owned state.
//
// Expected contracts:
//   - Read drafting documents.
//   - Read canvas object snapshots.
//   - Read undo/replay fixtures.
//   - Read compact machine records only after inspect metadata is present.
//
// Ownership rule:
//   MessagePack reader converts bytes into typed C++ contracts. It does not
//   accept raw binary data as app truth without validation.
//
// Must not depend on:
//   - UI widgets.
//   - Lua runtime.
//   - Raw untyped storage mutation.
//
// Preserve later:
//   No canonical MessagePack should be accepted without inspect/unpack tooling.
