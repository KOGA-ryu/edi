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

### Phase A — Corridors v2 (obstacle-aware routing) · compute: direct build (research done, [[edi-corridor-routing-research]])
- **◻ A1 — A\* grid pathfinder (pure).** `DraftingPathGrid` + A* over a coarse grid:
  Manhattan heuristic, turn penalty, weighted cost (empty +5, room-cell high/+10).
  *Accept:* tests — straight path when clear; routes AROUND an obstacle rect; turn
  penalty cuts corner count. Pure, Qt-free, in `src/drafting/`.
- **◻ A2 — route corridors with A\*.** Per connection: build the obstacle grid from
  room rects (inflated by half-width + clearance), A* door→door, simplify the cell
  path to an orthogonal polyline, feed through the EXISTING planCorridor offset +
  doorway machinery. Use A* when the straight L/Z would cross a room; else keep L/Z
  (cheaper, identical when clear). *Accept:* re-snapshot dungeon — no corridor crosses
  an unrelated room; full ctest green. Independent corridors (no merge).

### Phase B — Doors (M2.2) · compute: direct build
- **◻ B1 — door render at openings.** A connected opening reads as a DOOR (a leaf /
  marker driven by the plug's neutral type; secret stays flush). Reuses the opening +
  marker; no new geometry kind. *Accept:* door markers appear at connected openings,
  secret doors stay solid; test + snapshot.

### Phase C — Block / symbol library (M3, the "flash sheet") · compute: design + adversarial-critique workflow, then builds
- **◻ C0 — design pass.** Workflow: block definition + instance model in edi DOD,
  grounded in the §M seam (copy/paste + array + createObjectsAndSelect). Output: slice
  plan C1–C3 with acceptance tests.
- **◻ C1 — block definition** (save a named group). *Accept:* round-trip a named block.
- **◻ C2 — block instances** (transformed placement referencing one definition).
- **◻ C3 — palette UI + tag/set taxonomy** (widget layer).

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
