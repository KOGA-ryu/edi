// DraftingGeometry.h
//
// Purpose:
//   Declares pure geometry operations for drafting objects.
//
// Expected contracts:
//   - Compute bounds from typed geometry and style.
//   - Translate geometry.
//   - Transform points.
//   - Measure distances and dimensions.
//   - Compute hit-test scores.
//   - Generate handle anchors for legal edit intents.
//
// Ownership rule:
//   Geometry owns math only. It does not own document storage, selection, command
//   mutation, rendering, or persistence.
//
// Must not depend on:
//   - DraftingStore mutation APIs.
//   - Qt painting classes.
//   - Lua, TOML, TOON, or MessagePack.
//
// Preserve later:
//   Every function here should be deterministic, finite for valid input, and
//   testable as a pure function.
