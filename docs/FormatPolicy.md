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

## Inventory Commands

Inventory is read-only and does not convert, delete, or reject JSON by default.

```bash
build/edi_format_inventory --repo .
build/edi_format_inventory --repo . --summary
build/edi_format_inventory --repo . --families
build/edi_format_inventory --repo . --families --target MessagePack --sample-limit 2
build/edi_format_inventory --repo . --families --repo-state tracked
build/edi_format_inventory --repo . --families --scope disposable_artifact
build/edi_format_inventory --repo . --target MessagePack
build/edi_format_inventory --repo . --category internal_authored_json --summary
```

Use `--families` as the default planning view. It groups by category, data family, target format, and migration priority with file counts, byte totals, and bounded sample paths.

Use `repository_state` and `migration_scope` to keep tracked product contracts separate from ignored generated artifacts. Tracked files are migration candidates. Ignored files are disposable artifacts unless a later work order promotes them into tracked fixtures. Untracked files are local audit items.

Use failure modes only as explicit audits:

```bash
build/edi_format_inventory --repo . --fail-unknown
build/edi_format_inventory --repo . --fail-blocked
```

Do not register those failure modes as hard migration guards until the repo has a committed conversion plan for each data family.
