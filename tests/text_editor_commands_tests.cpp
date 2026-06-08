// text_editor_commands_tests.cpp
//
// Contract family:
//   Text editor command validation and mutation.
//
// Surface contract:
//   - Primary responsibility: document text command examples.
//   - Allowed setup data: text stores, document IDs, command values, ranges,
//     text payloads, roles, and titles.
//   - Call direction: test code calls text command execution APIs.
//   - Mutation authority: tests mutate local fixtures through commands.
//   - Unit convention: TextSelection range units.
//   - Identity policy: commands target stable document IDs.
//   - Lifetime: command values are local/replayable.
//   - Composition boundary: focused on editing commands, not UI key handling.
//   - Promotion path: add undo/coalescing examples when history exists.
