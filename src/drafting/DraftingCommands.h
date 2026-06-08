// DraftingCommands.h
//
// Purpose:
//   Declares the drafting command contract: user/script intent in, validated
//   document mutation out.
//
// Expected contracts:
//   - Create object.
//   - Delete object.
//   - Move object or objects.
//   - Update edit handle.
//   - Update metadata/style.
//   - Select, toggle, clear, or replace selection.
//   - Command result with accepted/rejected status and diagnostic text.
//
// Ownership rule:
//   Commands own intent validation and mutation sequencing. The store owns raw
//   storage invariants. Geometry owns math.
//
// Must not depend on:
//   - Widget classes or pointer events.
//   - Raw Lua VM state.
//   - Raw TOML/TOON/MessagePack objects.
//
// Preserve later:
//   Commands are the boundary that scripting and UI should call. They should be
//   narrow, explicit, and replayable.
