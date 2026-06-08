// format_messagepack_tests.cpp
//
// Contract family:
//   MessagePack machine-state adapters and inspection tooling.
//
// Intended coverage:
//   - Schema/version metadata.
//   - Inspector summaries before load.
//   - Reader conversion into typed C++ contracts.
//   - Writer output that remains inspectable.
//   - Rejection of unsafe or unsupported binary payloads.
//
// Should not test here:
//   - TOML static config.
//   - TOON AI handoff exports.
//   - UI behavior.
//   - Direct command mutation.
//
// Later role:
//   These tests should become executable documentation that compact binary state
//   is accepted only through typed readers and inspection tools.
