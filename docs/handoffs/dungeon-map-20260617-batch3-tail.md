# Handoff — dungeon-map-20260617-batch3-tail

> The short tail after the corridors+doors closeout: the ratified `active_plug_type`
> chrome-support key + an honest backlog-exhaustion assessment (the hub's NOTHING-LEFT
> off-ramp). Builds on the closed corridors-doors batch (tip `980cc67`).

- **Campaign**: dungeon-map-20260617-batch3-tail
- **Department**: edi-dungeon-map
- **Hub directive**: continue autonomously with the batch-3/parked backlog; if the
  tool-first backlog is GENUINELY EXHAUSTED → `bus-hub NOTHING-LEFT` (the hub stands the
  dept down or fetches the user's next direction).
- **Guard**: rebase ONLY onto LOCAL master (planner-only); builders no git remote.

## Slices
### `037` — `active_plug_type` projection key (RATIFIED) — DISPATCHED
- edi-ui's door-type picker pre-selects the current type (hub-ratified UX) → add the key
  (mirror `active_object_is_plug`). Small, clearly-needed. ▶ builder.

### `038` — reviewer backlog-exhaustion ASSESSMENT — 2026-06-17 — DONE: **EXHAUSTED, NOTHING-LEFT**
- Reply: `~/dept-bus/edi-dungeon-map/replies/038-reviewer-backlog-exhaustion-assessment.md`
- **Verdict: tool-first backlog GENUINELY EXHAUSTED — recommend NOTHING-LEFT / stand down.**
  No BUILD-now correctness gap; the loop is complete + correct; the 3 gate-caught bugs all
  fixed in-tree. Item calls:
  - `syncGraphForMovedObject` → **PARK** (headline reactivation item): genuine UX-smoothing
    (auto-follow a moved plug; fixes the `plug.anchor` staleness + the export-edge asymmetry)
    BUT the loop is correct without it (manual `rerouteConnection` covers it; staleness only
    degrades a cosmetic export edge label), perf-sensitive (hooks the move path), design-gated.
    Building it now to stay busy = scraping.
  - Interactive plug **name/flags editing** → **PARK** (secondary): interactive plugs export
    opaque ids as names; genuine export-readability polish, functional today, needs edi-ui chrome.
  - `m_activeConnectionId`→snapshot → **DROP** (the reconcile patch IS the correct permanent
    answer — view-state must not persist). Locked-layer NIT → **DROP** (rare, graph-consistent,
    any fix is drafting-wide).
- **REACTIVATION AGENDA (if the user later wants UX-polish):** (1) `syncGraphForMovedObject`
  (design-gate first), (2) interactive plug name/flags. Otherwise the dept is DONE.

## ⏸ POWER-DOWN CHECKPOINT (user call 2026-06-17) — RESUME RECIPE

### DONE
- Three campaigns delivered + closed: cartography (`docs/closeouts/h2-src-drafting-map-boundary.md`),
  feature batch DM-01..15 (`…feature-batch.md`), interactive authoring corridors+doors
  (`…corridors-doors.md`). Full in-app map-authoring loop ships; mandate intact.
- Final green code line: **`980cc67`** (corridors+doors closeout) — last full **edi-gate GREEN**,
  handed to edi-ui to merge. Batch3-tail dispatched (037 + 038).

### IN FLIGHT at power-down (workers checkpoint their own slices)
- **`037` active_plug_type** (RATIFIED) — builder was MID-EDIT (uncommitted
  `DrawingDocumentProjection.cpp` + test in the shared worktree). The builder session
  checkpoints it (commits green, or stops safe). NOT verified green by the planner.
- **`038` backlog-exhaustion assessment** — reviewer in flight (read-only; re-runnable).

### CURRENT DEPT TIP + GREEN STATUS
- Planner checkpoint tip: this handoff commit on top of `6f5e55a`, on the green `980cc67`
  code line. The committed code is edi-gate GREEN; 037's uncommitted edits are unverified.

### NEXT (on resume — do in order)
1. **Re-read** charter (`docs/departments/edi-dungeon-map.md`) + `CLAUDE.md` +
   `~/dept-bus/PROTOCOL.md`. Rebase onto LOCAL master if needed (planner-only; guard:
   never origin).
2. **Check `~/dept-bus/edi-dungeon-map/replies/` for `037-*` and `038-*`:**
   - `037` present (or its commit on the branch) → verify edi-gate GREEN; confirm
     `active_plug_type` to **edi-ui**. If only uncommitted edits remain (no commit/reply)
     → re-dispatch brief `037` (it's durable).
   - `038` is DONE: **EXHAUSTED → the dept STANDS DOWN.** No BUILD work. (Already bus-hub'd
     NOTHING-LEFT at power-down.) Do NOT scrape — the 2 PARK items wait for an explicit
     user UX-polish request.
3. **So on resume the ONLY action is: land `037` (the last ratified slice) if it didn't
   complete, confirm `active_plug_type` to edi-ui, then the dept is DONE — idle/stood down
   until the hub/user gives a new direction.** The tool-first program is complete + exhausted.
