# Format Policy

This project treats file formats as boundaries, not app logic.

C++ owns truth, validation, parsing, commands, and durable state transitions. UI code and scripts can request work, but they do not own canonical data contracts.

## Format Roles

- TOML is for human-authored static configuration: themes, project profiles, shell layout, surface maps, and design rules.
- Lua is for authored composition and behavior recipes. Lua must never directly own app state.
- MessagePack is for compact machine state, snapshots, replay data, telemetry streams, drawing documents, and binary fixtures.
- Every canonical MessagePack fixture requires inspect and unpack tooling before acceptance.
- TOON is for AI handoff and context exports only: summaries, review packets, and context bundles.
- JSON and JSONL are temporary compatibility formats or quarantined external data only.

## Boundary Rules

No raw format objects should leak into app logic. Format readers convert bytes into typed C++ contracts. Format writers project typed C++ contracts back into the selected boundary format.

No blind conversion is allowed. Migration must be category-aware, covered by inventory, and backed by validation or inspection tooling for the target format.

No canonical JSON should remain long-term. Existing JSON and JSONL files are inventory inputs until each data family has an explicit migration plan.
