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
// Preserve later:
//   No canonical MessagePack should be accepted without inspect/unpack tooling.
//
// Surface contract:
//   - Primary responsibility: declare MessagePack-to-contract read adapters.
//   - Allowed data: byte buffers, schema labels, typed document/snapshot/fixture
//     outputs, and FormatResult diagnostics.
//   - Call direction: persistence/replay tooling calls MessagePack readers.
//   - Mutation authority: translation only.
//   - Unit convention: binary fields convert into typed document units.
//   - Identity policy: stable IDs in bytes become typed IDs.
//   - Lifetime: owns returned typed values; byte input is caller-owned.
//   - Composition boundary: reader loads values, command layer applies changes.
//   - Promotion path: schema-version readers can split by format version.
