# Handoff — ui-20260617-master-integration

**Department:** edi-ui (owns `src/widgets`, the master integration line, the chrome bucket).
**Session:** edi-ui-planner. **Gate:** builder (ongoing integration + chrome).

## STANDING INTEGRATION CADENCE (hub-delegated 2026-06-17)
edi-ui autonomously merges verified dept branches (drafting / dungeon-map / blender-lab)
to master as green tasks accumulate — **no hub routing for routine merges**. Only
cross-dept KEYSTONES come via the hub. Per-merge protocol:
1. Inspect dept branch tip: `git -C ~/edi-<dept> log --oneline`; only merge CLOSED tasks.
2. `git diff --stat <merge-base> <tip>` — confirm no shared-file collisions
   (`EdiShellWindow*`, `CMakeLists.txt`, `app/main.cpp`); if a shared file is touched
   that's a hub escalation, not a silent merge.
3. `git merge --no-ff <tip>` into master with a teaching commit body.
4. **Full green gate** (`cmake --build build && ctest` 100% + scan) — gate on FULL ctest
   now (H1 `-E` exclusion dropped). If red, investigate / back the merge out; master
   stays green so depts can rebase on it.
5. Update LEDGER + this handoff; brief heartbeat to hub so depts know master advanced.

## What this thread is
edi-ui owns three standing duties (HUB kick `~/dept-bus/edi-ui/briefs/000-kick-edi-ui.md`):
1. **Master integration line.** When a domain dept lands a verified op, the hub routes its
   merge here; edi-ui merges to master and keeps master green so other depts rebase down.
2. **The chrome bucket.** Wire each feature's UI surface per `docs/ui-surface/` as the
   domain op lands. Each surface is a new instance of an existing mechanism (belt cell,
   tool-option input, inspector field, picker, panel). Match the existing design exactly.
3. **Shell goldens** (H1 + DM-11) — re-bless the machine-local goldens on this box.

## RESUMED 2026-06-17 — progress since the pause
- ✅ **REBALANCE merges landed:** blender-lab full batch @`88452bb`, drafting DR-11/12 @`f8bed78`.
  edi-gate green both. bus-hub'd.
- ✅ **Chrome batch-1 reviewer audit:** ALL THREE (DR-10/DM-10/DM-14) **ACCEPT, no MUST-FIX**.
  Signal-safety (seed-before-connect, no write-back loop), widget lifetimes, data-oriented
  discipline all clean. One cross-dept NIT forwarded to drafting: `setRotateCopiesTotalAngle`
  ignores `|angle|<1` while the spin allows 0 → drafting OWNS it (queued after DR-13).
- ✅ **H1 RESOLVED:** re-blessed `tests/golden/default_shell_1100x760.png` ON THE BOX (captures
  the DM-14 placement spins now visible in the default shell). Full edi-gate now **102/102
  incl edi_shell_window_tests**; **dropped the `EXCLUDE` in `~/dept-bus/bin/edi-gate`** (full
  suite runs). Re-exclude only if the golden drifts on a NEW box.

### Chrome batches — BOTH DONE + AUDITED ACCEPT
- **Batch-1** DR-10 toggle / DM-10 fill button / DM-14 spins (`caafe9f`/`0acba08`/`a6bbf4b`) —
  reviewer ACCEPT no MUST-FIX. H1 golden re-blessed to capture the DM-14 spins.
- **Batch-2** DM-11 map-browser plugs/connections (`125c449`) + DM-01 view-auto-fit (`00a1ff4`)
  — reviewer ACCEPT no MUST-FIX. Map workspace renders framed + full graph readout
  (`/tmp/cb2_map.png`); default drafting golden untouched (map workspace has no committed
  golden — nothing to re-bless). Optional nits (deferred): browser shows qualified `room.plug`
  name (kept — clearer in a flat list); trivial `findRoom` DRY into DraftingMapQuery (cross-dept).

### Still open
1. **DM-15 chrome — BLOCKED, escalated to hub:** needs core projection plumbing edi-ui must NOT
   add — a `has_block_instance_selection` bool + the selected object's `instance_id` projected in
   `src/core/DrawingDocumentProjection.cpp` (+ `activeObjectProjection` in `ShellWidgetHelpers.cpp`).
   dungeon-map dept is CLOSED, so the hub must route it (drafting owns src/core, or re-open a
   dungeon-map sliver). Then the DM-15 inspector verb is a quick edi-ui follow-up.
2. **DR-13 angular-dim PAINTER seam:** edi-ui's, rides the eventual Angular-tool chrome (combo
   entry + belt cell + arc painter).
3. **Remaining drafting surfaces to audit/wire:** DR-11 kaleidoscope, DR-07/08/09 modify verbs,
   DR-14 (in flight) — assess the chrome backlog next.
2. **DM-15 cross-dept GAP (blocks the DM-15 chrome slice):** needs core plumbing edi-ui must NOT
   add — drafting/dungeon-map owe, in `DrawingDocumentProjection.cpp` (+ `activeObjectProjection`
   in `ShellWidgetHelpers.cpp`): a `has_block_instance_selection` bool AND the selected object's
   `instance_id` projected. Then DM-15 widget is a quick edi-ui follow-up. ESCALATE to hub
   (dungeon-map dept closed/idle).

Untracked `docs/ui-surface/{blender-lab,dungeon-map}-batch2/` are ui-integration's (stale-base
batch-2 specs) — NOT edi-ui's; left untouched.

## State as of 2026-06-17

### Done
- **Integration cadence merges into master (verified green each time, 97/97 + scan):**
  - `b4f2eed` — drafting DR-01 transformGeometry keystone (`167768e`).
  - `bd3d99d` — drafting DR-02/03: quadrant/nearest-on-curve (`e405d89`) + tangent/
    perpendicular snaps (`d9c7aca` + on-segment fix `d6b1738`).
  - `0041783` — blender-lab BL-01/03/04: extrude spine + AddPrism carrier + Python prism
    (isolated to src/recipe + tools/blender, no shared-file collision).
  - `c6e98e3` — dungeon-map DM-02..08: interior features + plug flags + Seam C. Branch was
    based on OLD master (`f87bc1b`) → duplicate-history 3-way conflicts resolved:
    DraftingMapTypes.h + MapToonExport.cpp took THEIRS (DM-04/06 flags features),
    DraftingCommands.cpp took OURS (cosmetic static_assert wording), docs took OURS.
    **Unblocks DM-14/15** (transformGeometry consumers).
- **Coordination is edi-ui's (no hub relay):** after each merge I bus the dept planner
  directly the merged tip + rebase instruction. Sent dungeon-map (`c6e98e3`) + blender-lab
  (`0041783`) reconciliation replies. Depts rebase on master; LEDGER conflicts always
  resolve take-master.
- **Master promoted** to `9967f0b` (cartography integration of all 3 depts), then
  **merged `dept/drafting` transformGeometry** (DR-01 keystone `167768e`) → master at
  `b4f2eed`. Clean merge (drafting core + 1 test + 1 CMakeLists line; no shell conflicts).
  Verified **97/97 ctest green + scan clean** on this Linux box. Reported to hub.
  → unblocks dungeon-map DM-14/15 (they rebase master).
- **UI-Integration dept + surface specs** committed to master (`6282a93`):
  `.claude/agents/edi-ui-integration/`, `docs/departments/ui-integration.md`,
  `docs/ui-surface/{INFRA.md, drafting/, dungeon-map/, blender-lab/}`.
- **H1 golden re-blessed** on this box (`e103097`): `tests/golden/default_shell_1100x760.png`.
  Was 6508px over a 4180 budget — pure font-AA drift (golden authored on the Mac).
  Verified render correct vs reference before blessing. `edi_shell_window_tests` now 100%
  green here. **Hub told to drop the `-E edi_shell_window_tests` exclusion** from the gate.

### Settled chrome-sliver interface (DM-01 / DM-11) — FINAL SIGNATURES (slivers built)
Built on dept/dungeon-map (`32b297b`, `1b8dacb`), 101/101, golden unchanged. Lands on master
in dungeon-map's FULL FINAL TIP (after a scale-overflow hardening commit) — merge that, then
wire chrome:
- **DM-01 auto-fit:** `std::optional<edi::drafting::Bounds2D>`
  `DrawingDocumentController::computeDocumentBounds() const` (`nullopt` on empty doc).
  edi-ui side: `computeFitView` consuming it via `viewportFitRect` / `clampViewportZoom`. No
  control surface (view behavior only).
- **DM-11 map browser:** `#include "drafting/DraftingMapQuery.h"` (no-Qt core), exposing:
  - `deriveEdge(const DraftingMapRoom&, Point2D) -> "N"/"E"/"S"/"W"`
  - `plugIsConnected(const std::vector<DraftingDeclaredConnection>&, const DraftingPlugId&)`
  `MapToonExport` already switched to it (golden UNCHANGED). edi-ui wires the
  `buildMapBrowserPanel` (EdiShellWindowIo.cpp:700) plugs/connections sections from the SAME
  helpers (flags = `·`-joined run, Fork 2 ratified) and **co-blesses the `map` workspace
  golden** in that same change.

### Pending / waiting on upstream
- **Chrome bucket is idle by design** — no surface-bearing op has landed yet.
  DR-01 transformGeometry has **NO OWN SURFACE** (primitive; surfaces only via the future
  DR-10/11/12 transform actions — see `docs/ui-surface/drafting/DR-surfaces.md:50`). Next
  chrome work pipelines behind: DR-10/11/12 (drafting transforms), DM-03/10/11/14/15
  (dungeon-map), blender-lab's 2 File-menu actions.
- **DM-11 map-browser golden co-bless** — FUTURE coordinated event. When dungeon-map adds
  plugs+connections to `buildMapBrowserPanel` (M8, edi-ui-owned host/file, dungeon-map
  content), the `map` workspace golden changes and edi-ui must re-bless in that same change.
  edi-ui is READY. (`docs/ui-surface/dungeon-map/DM-surfaces.md:41`.)

## How a fresh session resumes
1. `git -C ~/edi log --oneline -5` — confirm master tip; `git status -sb` clean.
2. Watch the bus inbox for the next route-merge or "op landed, wire chrome" doorbell.
3. For a chrome slice: read the feature's `docs/ui-surface/<domain>/*-surfaces.md` entry,
   write a self-contained brief to `~/dept-bus/edi-ui/briefs/NNN-*.md`, hand to
   edi-ui-builder, green-gate (build + ctest + scan + a `--snapshot` look check), then
   hand the diff to edi-ui-reviewer (signal-safety / widget lifetimes are the risky joints).
