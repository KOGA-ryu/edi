// text_document_tests.cpp
//
// Contract family:
//   Text document and text document store state.
//
// Surface contract:
//   - Primary responsibility: document text document/store state examples.
//   - Allowed setup data: document IDs, roles, titles, text content, metadata,
//     dirty flags, revisions, and active document IDs.
//   - Call direction: test code calls document and store helpers.
//   - Mutation authority: tests mutate local text fixtures.
//   - Unit convention: whole-document text values, no edit ranges here.
//   - Identity policy: stable document IDs.
//   - Lifetime: each scenario owns its store/document fixture.
//   - Composition boundary: focused on document state shape.
//   - Promotion path: extend into ASCII document state when that contract lands.
