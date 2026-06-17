# Handoff — dungeon-map-20260617-corridors-doors (BATCH-2)

> Per-campaign state. **Status: SCOPE CONFIRMED (interactive authoring) — design gate open.**

## ✅ SCOPE RESOLVED (hub, 2026-06-17)
Interpretation #2 confirmed: **BATCH-2 = the INTERACTIVE authoring loop in the LIVE
tool** — plug tool, connection tool (editable corridor via two-plug pick), door
authoring/type on plugs, corridor edit. NOT rebuilding the geometry (Phase A/B render
from `.map.toml` stays). Build OPS; **edi-ui gates the surfaces** (no corridor/door
surface spec exists yet — build ops now, wire chrome when the spec lands, per batch-1).
Mandate holds: neutral (door type is a neutral tag; no passable/weight/direction), NO
generation.

## Planned task set (the interactive loop — to be refined by the design gate)
- **Plug tool** — place a plug interactively (anchor a Point marker on a wall/object →
  `CreatePlugCommand`). Reuses the `PointCaptureIntent` pick + `DraftingGraphOps`.
- **Connection tool** — TWO-plug pick (plug A → plug B) → `DeclareConnectionCommand` +
  route corridor geometry ON DEMAND (`routeCorridorCenterline`+`corridorWalls`). The
  two-stage pick is the key NEW interaction (existing intents are single-pick).
- **Door authoring / type on plugs** — set a plug's neutral type (door/window/secret/
  portal) interactively → re-render the door leaf (the M1.3 WallType painter exists).
- **Corridor edit** — independent-corridor model (charter fork: each connection emits
  its OWN editable corridor); re-route on plug move / connection change.
  **HUB-RATIFIED DEFAULT (2026-06-17):** corridors are INDEPENDENT/editable (per the
  corridor-routing research rec) — the reviewer (gate 019) may override ONLY with
  strong cause; otherwise independent is the settled default. (Gate 019 already asks
  this; I apply this default-framing when integrating its verdict — not interrupting
  the in-flight one-shot reviewer.)
- **Delete** plug/connection interactively (cascades exist: `pruneGraphForRemovedObject`,
  `removePlug`/`removeConnection`).

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

### Reviewer boundary gate (interactive authoring design) — 2026-06-17 — OPEN
- Brief: `~/dept-bus/edi-dungeon-map/briefs/019-reviewer-interactive-authoring-design.md`
- Settle: the plug-anchor model, the TWO-plug connection-pick mechanism (new vs
  reuse), on-demand corridor routing + the editable-corridor (independent) model,
  neutral door-type authoring, where verbs live, delete cascades, and the slice
  breakdown. Neutral-boundary + data-oriented check. NO code.

## Next
- Reviewer settles the interactive-authoring design → I spec the builder batches →
  build ops (edi-ui wires chrome when the surface spec lands). bus-hub at ~3-task
  marks + closeout when the bucket is done.
