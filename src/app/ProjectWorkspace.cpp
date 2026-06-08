// ProjectWorkspace.cpp
//
// Implementation responsibility:
//   Implements project membership helpers and default workspace construction.
//
// Belongs here:
//   - Enforcing unique document IDs within a workspace.
//   - Adding/removing document references.
//   - Looking up workspace members by stable ID.
//   - Tracking active/default project-level references when appropriate.
//
// Must be delegated elsewhere:
//   - Drafting document mutation belongs in drafting store/commands.
//   - Text document mutation belongs in text store/commands.
//   - Format loading/saving belongs in format adapters.
//
// Boundary note:
//   This implementation should be deterministic and boring: project membership
//   changes in, updated workspace state out.
