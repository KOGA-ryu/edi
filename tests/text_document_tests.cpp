// text_document_tests.cpp
//
// Contract family:
//   Text document and text document store state.
//
// Intended coverage:
//   - Document construction.
//   - Role handling.
//   - Dirty/revision behavior.
//   - Store ID uniqueness.
//   - Active document consistency.
//
// Should not test here:
//   - Editor command range mutation.
//   - UI text widgets.
//   - TOON/TOML/MessagePack adapters.
//
// Later role:
//   These tests should become executable documentation for text state ownership
//   before editor behavior is layered on top.
