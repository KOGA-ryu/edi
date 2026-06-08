// MessagePackWriter.cpp
//
// Implementation responsibility:
//   Converts typed domain contracts into MessagePack bytes.
//
// Belongs here:
//   - Stable schema/version emission.
//   - Stable field ordering where the encoder supports it.
//   - Required inspect metadata.
//   - Diagnostics for unsupported values.
//
// Must be delegated elsewhere:
//   - File IO orchestration belongs in persistence services.
//   - Runtime mutation belongs in app/domain command layers.
//   - Human-authored behavior belongs in scripting.
//
// Boundary note:
//   This file writes representations, not truth. Truth is typed C++ state.
//
// Surface contract:
//   - Primary responsibility: implement typed value encoding to MessagePack.
//   - Allowed data: typed contracts, schema/version labels, encoder state, and
//     diagnostics.
//   - Call direction: called by save/export/replay tooling.
//   - Mutation authority: translation only.
//   - Unit convention: encode typed units according to schema.
//   - Identity policy: deterministic stable ID encoding.
//   - Lifetime: no retained references after output is produced.
//   - Composition boundary: no file IO or command dispatch.
//   - Promotion path: binary fixture normalization can layer on this writer.
