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

### Phase D — Seam B export (M4) · compute: exporter pattern + design-light
- **◻ D1 — TOON map-graph projection.** Project rooms/plugs/connections/neutral tags
  to TOON via `ToonExport`. *Accept:* the dungeon exports a stable TOON doc carrying
  every neutral field the engine needs (golden test).
- **◻ D2 — export action** (`--export-map` CLI + UI hook). *Accept:* file written; loop
  closed. → **STOP (tool-first complete).**

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
