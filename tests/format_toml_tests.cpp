// format_toml_tests.cpp
//
// Contract family:
//   TOML static-configuration adapters.
//
// Intended coverage:
//   - Reader diagnostics for malformed config.
//   - Reader conversion into typed C++ contracts.
//   - Writer stable output for static settings/presets.
//   - No raw TOML object leakage into app/domain logic.
//
// Should not test here:
//   - Drafting document binary persistence.
//   - Lua recipe execution.
//   - UI settings pages.
//
// Later role:
//   These tests should become executable documentation that TOML is a
//   human-authored config format, not durable app truth for active documents.
