// format_toml_tests.cpp
//
// Contract family:
//   TOML static-configuration adapters.
//
// Surface contract:
//   - Primary responsibility: document TOML adapter examples.
//   - Allowed setup data: TOML text, typed config values, source labels, and
//     FormatResult diagnostics.
//   - Call direction: test code calls TOML reader/writer APIs.
//   - Mutation authority: translation-only fixtures.
//   - Unit convention: config units declared in typed config values.
//   - Identity policy: typed IDs round-trip as stable text fields.
//   - Lifetime: parsed data is local to a scenario.
//   - Composition boundary: focused on adapter shape.
//   - Promotion path: split schema-specific TOML examples as configs grow.
