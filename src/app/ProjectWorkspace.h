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
// Preserve later:
//   Workspace should remain an organizer. It should not perform drawing edits,
//   text edits, or format conversion directly.
//
// Surface contract:
//   - Primary responsibility: group the assets and documents that make up one
//     editable EDI project.
//   - Allowed data: project ID/name, document IDs, collection membership,
//     project setting handles, asset library handles, and workspace references.
//   - Call direction: app state/controllers may query workspace membership;
//     domain stores may be reached through public document handles.
//   - Mutation authority: may add/remove/reorder project members.
//   - Unit convention: no drawing coordinates or text offsets.
//   - Identity policy: stable IDs are the project-facing handles; array indexes
//     are internal ordering details.
//   - Lifetime: owns project membership; document stores own document contents.
//   - Composition boundary: joins drafting/text/planning collections without
//     merging their internal models.
//   - Promotion path: asset libraries and settings can split into dedicated
//     workspace services when they become substantial.
