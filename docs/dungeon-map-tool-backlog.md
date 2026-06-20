# Dungeon-map tool backlog (the living plan)

The forward plan the planner keeps AHEAD of the builder. Mandate (2026-06-14):
**tool-first** stop-line · **run ahead autonomously** · **planner drives the forks,
with recommendations**. So this file is the source of truth for what's next: each
slice carries its own acceptance test and the compute shape it wants, so a builder
agent or an autonomous loop can pick it up and self-verify without re-deriving it.

Companion to `docs/dungeon-map-roadmap.md` (the long vision) and the shipped work
orders. Status keys: ✅ done · ▶ in progress · ◻ queued · ⏸ deferred.

## Stop-line (tool-first)

Ship a usable dungeon-authoring tool: a complete **author → export** loop. Finish
**corridors → doors → block library → Seam B export**, then STOP. Generation
(BSP/WFC) is vision-first and OUT of this scope.

## Forks — driven (planner's call, vetoable)

- **Corridor merging (v2):** INDEPENDENT corridors — each connection emits its own
  editable corridor; no Vazgriz reuse-merge. *Why:* edi emits editable geometry; the
  author moves each corridor separately. Merging is a baked-dungeon nicety.
- **Seam B export format:** TOON for the neutral map-graph handoff (readable,
  AI-friendly, reuses `src/formats/ToonExport.*`); the `.edidraw` MessagePack stays
  the lossless document. NOT UVTT (JSON; and the user runs their OWN engine, not a
  VTT). *Why:* `format_strategy.md` assigns TOON to handoff; the engine reads a clean
  neutral projection of rooms/plugs/connections/tags.
- **BSP vs WFC generation:** N/A — generation is out of tool-first scope; revisit
  only if the stop-line moves to vision-first.

## The queue

### Phase A — Corridors v2 (obstacle-aware routing) ✅ DONE (0c64fd9 + 021b4c1)
- **✅ A1 — A\* grid pathfinder (pure).** `DraftingPathfind` — weighted grid, Manhattan
  heuristic, turn penalty, (cell, incoming-direction) state. Tested.
- **✅ A2 — route corridors with A\*.** Refactored DraftingCorridor into
  corridorCenterline + corridorWalls; `routeCorridorCenterline` returns the direct L/Z
  when clear, else A* around the room obstacles (door endpoints snapped with an
  orthogonal elbow). Controller passes all-rooms-except-the-two-joined as obstacles.
  No-op on the reference dungeon (rooms well separated); detour proven by unit test.

### Phase B — Doors (M2.2) ✅ DONE (e49b9ef)
- **✅ B1 — door render at openings.** A connected opening gets a door LEAF — a thin
  WallGeometry band spanning the doorway (provenance "door"), render type from the
  plug's neutral type via the M1.3 WallType painter. Unconnected secret plugs stay
  flush. Dungeon 146 → 170 objects; controller test asserts two Door-type leaves.

### Phase C — Block / symbol library (M3, the "flash sheet") · compute: design + adversarial-critique workflow, then builds
- **✅ C0 — design pass.** Ran `block-library-design` workflow (3 grounding explorers →
  3 approaches + 3-judge panel → adversarial critique). **Fork resolved → FLATTEN-on-place**
  (unanimous 8.6/9/9): a placed instance is independent first-class objects, NOT a live
  reference (which scored 6.2/5/5 — it would create a second derived-at-render object space
  breaking the ~16 sites walking `document.objects`, and needs a `transformGeometry` over all
  14 kinds that mirror/array deliberately refuse). Critique verdict **sound**; grafts folded in:
  (1) **no parallel serial counter** — mint `block_NNNN` off the one `m_nextObjectSerial`,
  and `highestDocumentIdSerial` scans objects+plugs+connections+blocks (closed a pre-existing
  plug/conn recovery hole too); (2) **readBlock recomputes bounds** (derived, never trusted);
  (3) **provenance honesty** — block-origin tag goes in `metadata.source` (NOT `toolProvenance`,
  which drives a calibration branch), commented as pure provenance, or omitted (C2 call).
  Deferred (named, separable): `transformGeometry` rotate/scale over all 14 kinds, backing a
  rotate/scale TOOL first — that is what unlocks per-instance rotation/scale; the FLATTEN MVP
  is translate-only because `translateGeometry` is the only geometry transform that exists.
- **✅ C1 — block definition** (ccd14c6). `DraftingBlock` + `DraftingBlockOps`
  (`buildBlockFromObjects` normalize-to-origin, add/removeBlock) + Create/DeleteBlockCommand +
  additive `blocks` MessagePack (tolerant, no version bump) + the serial-recovery fix.
  *Accept (met):* `drafting_block_ops_tests` — normalize, validation, additive round-trip +
  forward-compat, serial regression. 94 green.
- **✅ C2 — block instances** (FLATTEN placement) (9ac319d). `placeBlockInstance(blockId, x, y)`
  centres the definition on the point and stamps independent copies via `planDraftingPaste` +
  `createObjectsAndSelect` (one undo step, auto-select, translate-only, NO provenance breadcrumb);
  `defineBlockFromSelection(name)` gathers the selection, mints `block_NNNN` off the one serial,
  applies `CreateBlockCommand` through `applyCommandAndEmit` (undoable), refuses empty selection.
  *Accept (met):* controller test — define one-undo-step, stamp two `instance_` objects centred on
  the point, definition byte-unchanged (independence), one undo step. 94 green.
- **✅ C3 — palette UI** (widget layer) (dd12fad). `PointCaptureIntent::BlockInstance` +
  `beginBlockInstancePick` + a `block_palette` FeaturePaletteSpec (shell frames it in a
  FloatingPalette like the tool belt). Name field + "Save selection as block" button →
  `defineBlockFromSelection`; block list rows carry the id, click → arm point capture → next
  canvas click stamps; `refreshBlockPalette` keeps the list live. *Accept (met):*
  `edi_shell_window_tests` drives it end-to-end through the real widgets; default-shell golden
  re-blessed for the new palette. 94 green. **Minimal/unstyled by design — the user owns the
  look (placement + visual polish to restyle); tag/set taxonomy deferred (open `tags` is additive).**

**Phase C COMPLETE** (C0–C3). Block library ships: define a selection as a named block →
stamp independent editable instances → browse/stamp from the palette → round-trips through
`.edidraw`. Translate-only (rotation/scale await the deferred `transformGeometry` slice).
**Next: Phase D (Seam B export), then STOP.**

### Phase D — Seam B export (M4) ✅ DONE (d19f685) · compute: exporter pattern + design-light
- **✅ D1 — TOON map projection.** `exportMapToToon` (src/io/MapToonExport, pure) projects
  the typed **MapSpec** (NOT a lossy document reconstruction — the document drops room
  footprints/edges/names; the MapSpec carries them) to three flat tabular TOON arrays:
  rooms{name,origin,size,material}, plugs{room,name,edge,type,connected}, connections{from,to,type}.
  Rooms keyed by name; `connected` derived; empty plug type → "door". *Accept (met):*
  `map_toon_export_tests` pins the exact wire format (golden contract).
- **✅ D2 — export action** (`--export-map <toon>` CLI). Headless: parse `--map-file` at
  canvasPerUnit=1.0 (authored feet), project, write, exit. *Accept (met):* the reference
  dungeon exports 12 rooms / 26 plugs / 12 connections; loop closed.

**Phase D COMPLETE → TOOL-FIRST PROGRAM COMPLETE (STOP-LINE REACHED).** The full
author → export loop ships: `.map.toml` → rooms/walls/corridors/doors/blocks in edi →
neutral TOON map across Seam B to the game engine. Generation (BSP/WFC) is out of scope
by mandate. Optional past the line: the deferred `transformGeometry` (block rotate/scale),
and the polish items below. A future generalization (if needed): export edi-edited
*documents* (not just authored `.map.toml`), which would first store rooms/edges in the
document — the heavier "real Seam B from the live doc" path.

### Campaign — proving-ground neutral marker layer ✅ DONE (2026-06-19, M1–M4) · past the stop-line
A NEUTRAL "first-playable proving-ground" map for the user's game-engine testing + its Seam B TOON
export. Adds, ADDITIVELY (no version bump): markers = `RoomFeature` + `id` + `metadata`; a map-level
`MapPatrolPath {id,waypoints,closed}`; a generic lock tag (`locked`+`key_id` typed on the connection,
k/v in a chest marker's metadata). The Seam B (MapSpec) TOON gains conditional `markers[]`/`patrols[]`
+ connection lock columns; reference golden stays byte-identical. Authored `tests/data/provingground.map.toml`
(spawn→two-turn corridor→dead-end branch→gated key alcove(gold_key)→npc+patrol→goal behind a locked
door; ONE gold_key gates the goal door AND a chest). SHAs M1 `8f240b5` / M2 `68e3b94` / M3 `1f93288`
/ M4 `a5a4f13`. Closeout + engine manifest: `docs/closeouts/dungeon-map-proving-ground.md`. Prior art:
`docs/research/marker-patrol-lock-prior-art.md`. **Deferred:** lock on `DraftingDeclaredConnection` +
conditional `.edidraw` codec (only when a live-document export needs it). **Triage (pre-existing, NOT
this campaign):** drafting-core has a 7-test Release-build SEGFAULT (Debug clean) + `--export-map`
aborts on CLI teardown after writing the file — both predate `e34e773`; flagged to the hub.

## Cross-cutting polish (fold in opportunistically)
- **◻ P1 — view auto-fit** for authored maps (frame the whole map to the viewport;
  the snapshots are currently un-framed). High usability; do early, likely beside A2.
- **◻ P2 — interior features** (`map.room.<i>.feature.<j>.{x,y,type}` → free Point
  markers) — the center rubble plugs couldn't place (edge-locked).
- **◻ P3 — region / bucket fill** — colour an enclosed room without tracing.

## Planner-ahead protocol
Keep the next 2–3 slices fully specced (acceptance baked) ahead of the builder.
Run research/design workflows at PHASE starts (not per slice). Independent slices may
run in parallel git worktrees. Surface to the user only at phase boundaries or a real
new fork. Update status here as slices land; this file outlives any single context.
