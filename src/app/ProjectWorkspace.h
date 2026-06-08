// ProjectWorkspace.h
//
// Purpose:
//   Defines the project-level container for EDI work: drafting documents, text
//   documents, workspace settings, assets, and project identity.
//
// Expected contracts:
//   - Stable project/workspace ID.
//   - Collections of drafting and text document references.
//   - Project settings references, recent files, tool preset references, and
//     asset library references.
//   - Typed add/remove/find APIs by stable ID.
//
// Ownership rule:
//   The workspace owns membership and identity. Domain stores own document
//   contents and validation.
//
// Must not depend on:
//   - Concrete TOML/MessagePack/TOON/Lua parser objects.
//   - Widget layout classes.
//   - Raw drawing object or text edit implementation details beyond public
//     domain contracts.
//
// Preserve later:
//   Workspace should remain an organizer. It should not perform drawing edits,
//   text edits, or format conversion directly.
