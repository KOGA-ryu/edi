# EDI Format Strategy

Formats are boundary tools. They should not leak raw structures into app logic. C++ owns truth, validation, commands, and durable state transitions.

## Ownership Rule

Runtime flow should be:

1. Read bytes from a boundary format.
2. Parse into a typed C++ contract.
3. Validate the typed contract.
4. Mutate state only through C++ commands.
5. Export by projecting typed state back through a format adapter.

No format should directly own behavior or canonical in-memory state.

## TOML

Use TOML for human-authored static configuration.

Good fits:

- App settings
- Theme
- Workspace layout
- Tool presets
- Project profiles
- Declarative export presets

TOML documents should be readable in diffs, understandable without tooling, and limited to static configuration. If a file needs behavior, composition, or command sequencing, it should not be forced into TOML.

## TOON

Use TOON for AI-facing handoff and context exports.

Good fits:

- Planning packets
- Summaries
- Review packets
- Agent handoff bundles
- Compact document/context exports

TOON is a communication format. It should not become app truth, project storage, or a runtime command source.

## MessagePack

Use MessagePack for compact machine-owned state.

Good fits:

- Drawing documents
- Canvas object snapshots
- Undo/replay fixtures
- Telemetry-like compact records if needed
- Binary golden fixtures

Rule: no canonical MessagePack without inspect/unpack tooling. If a binary file cannot be inspected by a project tool, it is not acceptable as a durable project artifact.

## Lua

Use Lua for authored behavior and composition.

Good fits:

- Procedural recipes
- Export recipes
- Batch tool chains
- Build-plan generation recipes

Lua can request commands. Lua cannot directly mutate app state or own raw object storage. C++ validates every command request before mutation.

## Format Boundaries By Feature

| Feature Area | Preferred Format Direction | Reason |
| --- | --- | --- |
| App/project settings | TOML | Human-authored, static, diffable |
| Workspace layout | TOML | Human-readable layout configuration |
| Drafting documents | MessagePack | Compact typed machine state |
| Drafting replay fixtures | MessagePack | Stable compact test fixtures with inspector tooling |
| Text documents | Native text plus typed metadata contract | Text should remain text; metadata needs a typed boundary |
| AI handoff packets | TOON | Communication-focused context export |
| Automation recipes | Lua | Authored behavior and composition |

## Implementation Order

1. Define the typed C++ contract for the feature.
2. Define validation and command ownership.
3. Add tests for the typed contract.
4. Add the format adapter.
5. Add inspection tooling for binary formats.
6. Wire the adapter into UI or automation.
