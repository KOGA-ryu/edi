// drafting_commands_tests.cpp
//
// Contract family:
//   Drafting command validation and mutation sequencing.
//
// Surface contract:
//   - Primary responsibility: document command contract scenarios.
//   - Allowed setup data: local documents, command values, object IDs, movement
//     payloads, handle payloads, and selection payloads.
//   - Call direction: test code calls public command execution APIs.
//   - Mutation authority: tests mutate local fixtures through commands.
//   - Unit convention: use command-declared units.
//   - Identity policy: commands target stable IDs.
//   - Lifetime: command values can be reused for replay-style scenarios.
//   - Composition boundary: focuses on command orchestration over store details.
//   - Promotion path: add replay/undo examples once command history exists.
