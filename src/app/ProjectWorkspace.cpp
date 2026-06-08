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
//
// Surface contract:
//   - Primary responsibility: implement workspace membership operations.
//   - Allowed data: workspace values, document IDs, ordered membership lists,
//     and project-level reference records.
//   - Call direction: called by app/project controllers; calls only public
//     domain collection helpers where needed.
//   - Mutation authority: may change membership and active/default references.
//   - Unit convention: no coordinate, measurement, or text-range units.
//   - Identity policy: lookups use stable IDs; ordering may use vectors.
//   - Lifetime: updates the workspace container, not the pointed-to documents.
//   - Composition boundary: keeps project composition separate from editing.
//   - Promotion path: persistence orchestration can wrap this layer later.
