// TextEditorCommands.cpp
//
// Implementation responsibility:
//   Applies validated text editor commands to TextDocumentStore/TextDocument.
//
// Belongs here:
//   - Checking document existence.
//   - Validating edit ranges.
//   - Applying insert/replace/delete operations.
//   - Updating dirty flags and revisions after accepted mutation.
//
// Must be delegated elsewhere:
//   - Store membership logic belongs in TextDocumentStore.
//   - Pure range helpers belong in TextSelection.
//   - UI key handling belongs in widgets/controllers.
//   - Format export/import belongs in format adapters.
//
// Boundary note:
//   This file is the text mutation gate. UI and scripts should request commands,
//   not edit document strings directly.
//
// Surface contract:
//   - Primary responsibility: apply text commands to document/store state.
//   - Allowed data: mutable stores/documents, command values, range helpers,
//     and diagnostics.
//   - Call direction: called by UI/controller/script bridge code.
//   - Mutation authority: applies accepted text mutations and metadata edits.
//   - Unit convention: consumes TextSelection range units and string payloads.
//   - Identity policy: resolves stable document IDs at execution time.
//   - Lifetime: command execution does not own the store.
//   - Composition boundary: editor mutation stays separate from UI gestures.
//   - Promotion path: command coalescing/history can wrap this executor.
