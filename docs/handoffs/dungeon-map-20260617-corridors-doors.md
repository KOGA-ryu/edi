# Handoff — dungeon-map-20260617-corridors-doors (BATCH-2)

> Per-campaign state. **Status: SCOPE FLAGGED TO HUB — not building until confirmed.**

- **Campaign**: dungeon-map-20260617-corridors-doors
- **Department**: edi-dungeon-map
- **Dispatch**: "Arc to the mandate stop-line: CORRIDORS + DOORS — route declared
  door↔door connections into corridor GEOMETRY (L/Z + grid-A*) + door geometry on
  plugs. Plan from the backlog (remaining tool-first items); neutral, NO generation.
  Rebase on master first. ui-integration gates surfaces in parallel."
- **Base**: rebased on master `d02ac86` (transformGeometry + batch-1 DM-01..15 all
  present; corridor + region-fill + map-query infra present).

## ⚠ SCOPE DISCREPANCY (flagged to hub, awaiting ratification)
The dispatched corridors+doors work appears **ALREADY COMPLETE on master**. Verified
against live source (not the backlog's word):

| Dispatched item | State on master | Evidence |
| --- | --- | --- |
| Route declared connections → corridor GEOMETRY (L/Z + A*) | **DONE** | `createMapFromSpec` calls `corridorWalls(routeCorridorCenterline(corridor, obstacles), …)` (`DrawingDocumentController.cpp:2545-2546`); `DraftingCorridor` has `corridorCenterline`/`routeCorridorCenterline`(A* w/ obstacles)/`corridorWalls`/`planCorridor`; `DraftingPathfind` is the grid-A*. Commits `0c64fd9`/`021b4c1` in HEAD history. |
| Door geometry on plugs | **DONE** | carved doorway opening + door LEAF (`WallType::Door`) on CONNECTED plugs, secret stays flush, perpendicular corridor exit (`:2369-2399`). Commit `e49b9ef` in HEAD history. |
| Backlog "remaining tool-first items" | **NONE** | backlog marks Phase A/B/C/D ✅ "TOOL-FIRST PROGRAM COMPLETE (STOP-LINE REACHED)". |
| Cross-cutting polish P1/P2/P3 | **DONE in batch-1** | DM-01 auto-fit, DM-02/03 features, DM-09/10 region fill. |

**The ONLY plausible genuinely-remaining gap:** *interactive* corridor/connection
authoring. Corridors/doors only materialize during authored-map (`.map.toml`) LOAD —
there is NO `PointCaptureIntent` / controller verb to connect two plugs in-app and
route a corridor on demand (the enum has RadialArray/RotateCopies/Trim/Extend/Fillet/
Chamfer/Break/BlockInstance/RegionFill — nothing for connect-plugs). The graph ops
(`CreatePlug`/`DeclareConnection`) exist but are only applied during map build.

## Recommended interpretations (for the hub to pick)
1. **(MOST LIKELY) Already done — dispatch was stale.** Confirm the corridors+doors
   arc is complete; close BATCH-2 as already-satisfied, or redirect to a different
   bucket.
2. **Interactive corridor authoring (real new work).** Add a connect-two-plugs verb
   (a `PointCaptureIntent`) → `DeclareConnectionCommand` → route+emit corridor geometry
   on demand. This is the genuine gap and stays neutral/tool-first. (My pick if the
   hub wants corridors+doors *work*.)
3. **An enhancement** — e.g. the charter's "merged-vs-independent-corridors fork" (the
   editable one), or interactive door-type editing.

## Gate log
### Verification — 2026-06-17 — edi-dungeon-map-planner
- Rebased on master; verified corridors+doors already implemented (table above).
- Flagged the scope discrepancy to the hub via bus-hub. NOT building until confirmed
  (do not rebuild done work).

## Next
- Await hub scope ratification. On confirm of interpretation 2/3, open the campaign
  (reviewer boundary if needed → builder); on interpretation 1, close BATCH-2.
