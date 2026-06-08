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
// Preserve later:
//   Every canonical binary output should be inspectable before it is trusted.
//
// Surface contract:
//   - Primary responsibility: declare contract-to-MessagePack write adapters.
//   - Allowed data: typed documents, snapshots, replay fixtures, schema metadata,
//     output options, and diagnostics.
//   - Call direction: persistence/replay/golden tooling calls writers.
//   - Mutation authority: translation only.
//   - Unit convention: typed units are encoded with schema-defined meaning.
//   - Identity policy: stable IDs are encoded as canonical handles.
//   - Lifetime: returns owned byte buffers.
//   - Composition boundary: writer does not choose document behavior.
//   - Promotion path: chunked writers can be added for large documents.
