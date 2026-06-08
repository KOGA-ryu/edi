// drafting_commands_tests.cpp
//
// Contract family:
//   Drafting command validation and mutation sequencing.
//
// Intended coverage:
//   - Create/delete/move/update command acceptance.
//   - Command rejection diagnostics.
//   - Selection-affecting commands.
//   - Revision changes on accepted mutation.
//   - Failed commands preserving prior document state.
//
// Should not test here:
//   - Raw geometry math in isolation.
//   - Widget click paths.
//   - Binary/text format parsing.
//
// Later role:
//   These tests should become executable documentation for C++ as the mutation
//   gate used by UI and scripting.
