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
