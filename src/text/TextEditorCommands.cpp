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
