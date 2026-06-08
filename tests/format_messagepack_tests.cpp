// format_messagepack_tests.cpp
//
// Contract family:
//   MessagePack machine-state adapters and inspection tooling.
//
// Surface contract:
//   - Primary responsibility: document MessagePack adapter and inspector
//     examples.
//   - Allowed setup data: byte buffers, schema/version fields, typed documents,
//     snapshots, fixture summaries, and FormatResult diagnostics.
//   - Call direction: test code calls inspector, reader, and writer APIs.
//   - Mutation authority: translation/inspection fixtures only.
//   - Unit convention: typed units before encode and after decode.
//   - Identity policy: stable IDs preserved through binary representation.
//   - Lifetime: byte buffers and decoded values are local fixtures.
//   - Composition boundary: focused on binary adapter shape.
//   - Promotion path: add golden fixture examples after inspector tooling exists.
