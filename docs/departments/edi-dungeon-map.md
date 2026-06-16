# Department charter — edi-dungeon-map

D&D-style dungeon-map authoring on the drafting bench: a wall-segment graph,
rooms, plugs and connections, and corridor routing — exported as a NEUTRAL map
document for the user's own game engine. **Layered law: edi does drafting only
(geometry + neutral tags, NO game rules); the engine owns rules.**

## Scope (what this department owns)
- The map graph: `plugs` and `declared_connections` as document-level vectors on
  `DraftingDocument` (a plug is a RELATION, not a geometry variant), neutral-only
  (no passable/weight/direction), their ops, `DraftingCommand` arms, and the
  additive/tolerant MessagePack persistence.
- Wall primitives, rooms, and corridor routing (door↔door connections → corridor
  geometry).
- The Map workspace: `WorkspaceMode::Map`, `mapWorkspaceLayout`, the live map
  graph browser (footprints in authored feet) — its CONTENT (the host/chrome is
  edi-ui's).
- Authoring + export (Seam B/C): `parseMapSpecToml` / `createMapFromSpec`,
  `io/RoomSpecStore`, `--map-file`, and `exportMapToToon` / `io/MapToonExport` /
  `--export-map` (rooms + plugs + connections + placed `blocks[]` in authored
  feet).

Builds ON the drafting core (geometry) — coordinate with **edi-drafting**. The
shell host/chrome is **edi-ui**'s.

## Architecture (the rules to obey)
THREE SEAMS: Seam A = AI control (code/prompt); the document = geometry + neutral
tags only; **Seam B = the neutral map doc → the user's game engine, which owns
the rules.** edi records, it does not simulate. plugs/connections stay neutral.
Persistence: a new document-level vector + free-function ops + a `DraftingCommand`
arm + additive/tolerant MessagePack (missing key ⇒ default, NO version bump —
like `wall_visual`). Data-oriented; no subclassing.

TOOL-FIRST STOP-LINE (mandate): corridors → doors → blocks → Seam B export, then
STOP. Generation (WFC/procedural) is OUT of scope — edi is the authoring bench,
not the generator. The map doc is neutral; rules live downstream.

## Read first
- Memory: `edi-dungeon-map-research` (the verified plan + current status),
  `edi-corridor-routing-research`, `edi-tool-first-mandate`.
- `docs/dungeon-map-graph-work-order.md`, `docs/dungeon-map-roadmap.md`,
  `docs/dungeon-map-seams.md`, `docs/dungeon-map-tool-backlog.md`,
  `docs/map-authoring-format.md`.
- The reference dungeon: `tests/data/dungeon.map.toml` (10 rooms + junction +
  entrance + 12 corridors).

## Verify (the green gate for this department)
```
cmake --build build && ctest --test-dir build --output-on-failure   # incl. the map tests
```
plus the scan. Render the reference dungeon offscreen:
`QT_QPA_PLATFORM=offscreen ./build/edi --workspace map --map-file tests/data/dungeon.map.toml --snapshot /tmp/map.png`.
Seam C export carries placed blocks:
`./build/edi --map-file <saved>.edidraw --export-map out.toon`.

## Backlog
`docs/dungeon-map-tool-backlog.md` (the tool-first backlog). Next candidate: DRAW
declared connections as corridor geometry (see `edi-corridor-routing-research` —
L/Z v1, then weighted grid A* v2; the merged-vs-independent-corridors fork is the
editable one). Asset-Dex / CSV is SHELVED (a passing remark, not a goal).
