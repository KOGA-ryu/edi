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

### Reviewer boundary gate (interactive authoring design) — 2026-06-17 — SETTLED YES
- Reply: `~/dept-bus/edi-dungeon-map/replies/019-reviewer-interactive-authoring-design.md`
- **Design settled.** The whole loop reuses existing single-pick capture + graph/
  corridor/door pieces — **NO new command arm, NO codec change, NO new struct, NO
  subclassing.** 2 new `PointCaptureIntent` values (DATA) + thin verbs + pure helpers.
  - **Plug tool:** snap-pick a wall point (snap runs before `resolvePointCapture`, free)
    → mint Point marker + `CreatePlugCommand` (`beginPlugPick`/`placePlugAtPoint`).
  - **Connection tool:** reuse the `m_pendingBlockId` idiom — ONE intent `PlugConnect`
    + ONE member `m_pendingConnectionPlugA` (first click stores A + re-arms; second
    resolves B → `connectPlugs`). Click→plug via `hitTestDocument` + NEW pure helper
    `plugAtAnchorObject(doc, objectId)` in `DraftingGraphOps`. On-demand corridor from
    existing `corridorWalls(routeCorridorCenterline(...))`.
  - **Editable corridor:** INDEPENDENT (confirmed, = hub default). Corridor↔connection
    tie = a NEUTRAL provenance tag `connection:<connId>` on each corridor wall (open-
    vocab breadcrumb like `feature:<type>` — no new field/codec). Edit/delete/re-route
    filter by the tag.
  - **Door type:** `setPlugType(plugId,type)` → re-mint the `plug:<plugId>`-tagged Wall
    leaf via the M1.3 `WallType` render mapping. Neutral.
  - **Delete (the flagged trap):** rendered objects (marker/leaf/corridor walls) are
    NOT graph records → gather-then-delete explicitly. `deletePlug`/`deleteConnection`
    one bracket each; mind the `pruneGraphForRemovedObject`-vs-`DeletePlugCommand`
    double-prune ordering (B2-4 brief calls it out).
- Neutral + data-oriented + H2 boundary all confirmed. edi-ui owns the tool buttons.

### PLANNER RULINGS on the 3 parked items (2026-06-17)
1. **Auto-re-route-on-plug-move → PARK** (accept). v1 = a manual `rerouteConnection`
   verb (B2-5); auto-sync joins the already-parked `plug.anchor`-staleness TODO. A
   future `syncGraphForMovedObject` serves both. Consistent with the codebase's
   parking discipline.
2. **Wall-opening carve at the doorway → PARK** (accept abutting-but-solid v1). The
   authored path carves a gap in the room wall; interactive v1 emits the corridor +
   door leaf but leaves the room wall solid behind the leaf. The door LEAF already
   provides the door representation (a door reads as a band within a wall), so
   abutting-but-solid is visually acceptable for v1; carving = wall-segment splitting
   (materially heavier), parked. **Flagged to hub/edi-ui** as a v1 visual choice (the
   look is theirs) — record, not a blocker.
3. **Merged/Vazgriz corridors → OUT OF SCOPE** (independent ratified). No action.
   Also v1: `deleteConnection` keeps the plug's door leaf (not reverted to solid wall)
   — accepted v1 choice (a plug can be reconnected); noted.

### Ratified surface model (DM2-surfaces, hub 2026-06-17) — bake into ops
- **plug pick = free canvas click** (snap runs before `resolvePointCapture`, free).
- **connection create = two-click capture** (click plug A → click plug B).
- **connection SELECT (for edit) = Map-browser ROW click** (not a canvas pick) — so a
  connection is picked via the browser; my deliverable is `selectConnection(connId)` +
  the relation-aware inspector context.
- **door = auto-from-plug-type — NO separate door tool** (setPlugType drives the leaf).
- **⚠ STRUCTURAL (my ops):** a connection is a RELATION (not a selectable object);
  the inspector context keys on relation MEMBERSHIP → **`contextForKind`
  (`src/drafting/DraftingInspectorPlan.cpp:59`, used `:98` on `selectedKind`) must
  WIDEN from kind-only to relation-aware** (is the selected object a plug anchor? is a
  connection selected?). This is its own slice **B2-CTX** + `selectConnection(connId)`.
- (DM2-surfaces.md not yet on my worktree; decisions taken from the hub message.)

### Slice plan + batch order (reviewer B2-1..B2-5 + the surface-spec B2-CTX)
- **Builder batch-1** = B2-1 (plug tool, free-click) + B2-2 (connection two-click +
  on-demand corridor). Brief `020` WRITTEN + READY. ⏸ **NOT dispatched (hub pause).**
- batch-2 = **B2-CTX** (relation-aware `contextForKind` widening + `selectConnection`)
  + B2-3 (door auto-from-type: `setPlugType`).
- batch-3 = B2-4 (interactive delete + cascade cleanup — the double-prune-ordering
  trap) + B2-5 (manual re-route).
- Each carries the reviewer's baked acceptance. ui-integration wires chrome later.

## ▶ RESUMED (HUB, 2026-06-17) — full fleet live, new toolbelt + tiering
Re-read the updated `~/dept-bus/PROTOCOL.md`: green gate = `edi-gate`; reply via
`bus-reply`; number via `bus-next`; context hygiene via `bus-ctx`/`dept-cycle`;
model tiering (planner+reviewer=Opus, builder+researcher=Sonnet); merges → `edi-ui`,
surfaces ← `edi-ui-integration-*`. **At each worker reply boundary: `dept-status` +
`dept-cycle` any of MY workers >500k OR still on opus.**

- **Batch-1 (B2-1+B2-2) DISPATCHED.** `dept-cycle`'d the builder opus→sonnet (was
  412.8k on opus; rolled + shed context) and sent brief `020`. Builder working now.
- **Cross-dept slice (hub-routed) — brief `021` WRITTEN, QUEUED behind 020:** add
  `has_block_instance_selection` + `instance_id` to `DrawingDocumentProjection.cpp`
  (block instances are our domain) for edi-ui's DM-15 inspector. **bus-hub the hub
  when it lands so it routes the green tip to edi-ui.** (This is the optional
  projection key the DM-15 builder noted but left.)
- Reviewer is on opus/255k (target opus) — OK, no cycle.

### Updated batch order
- batch-1 = B2-1 + B2-2 (`020`) — ✅ DONE `f4b6ac1`.
- batch-1b = block-instance projection keys (`021`, cross-dept) — ▶ IN FLIGHT.
- batch-2 = B2-CTX (relation-aware `contextForKind`) + B2-3 (setPlugType door-type).
- batch-3 = B2-4 (delete + cascade cleanup) + B2-5 (manual re-route).

### Builder batch-1 (B2-1+B2-2, plug+connection tools) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/020-builder-plug-connection-tools.md`
- Commit `f4b6ac1` (B2-1+B2-2 COMBINED — builder judgment: shared enums/switch/state;
  flagged). edi-gate GREEN 102/102, scan clean, authored path untouched. Plug tool
  (free-click → marker+plug, one undo), connection tool (two-click → DeclareConnection +
  `connection:<id>`-tagged corridor walls, one undo), `plugAtAnchorObject` helper. New
  surface for edi-ui: `beginPlugPick()` / `beginConnectionPick()` (+ `pointCapturePrompt`).
- Builder edge-case calls (→ audit 022 checks): first-click-miss = lenient re-arm; plug
  outside all rooms → `deriveEdge` fallback `RoomEdge::North`; door leaf NOT minted on
  interactive plugs (B2-3 `setPlugType` mints it). Context OK (sonnet, below-nudge).

### Reviewer diff-audit of 020 — DISPATCHED (parallel with 021)
- Brief: `~/dept-bus/edi-dungeon-map/briefs/022-reviewer-020-audit.md`
- Focus: the two-click capture state machine (stale plug-A leaks, A-deleted-mid-pick),
  neutral corridor tags, one-undo brackets, the edge-fallback judgment calls.

### Builder slice 021 (block-instance projection keys, cross-dept) — DISPATCHED
- Brief: `~/dept-bus/edi-dungeon-map/briefs/021-builder-block-instance-projection-keys.md`
- `has_block_instance_selection` + `instance_id` in `DrawingDocumentProjection.cpp`.
  **On land → bus-hub so the hub routes the tip to edi-ui (DM-15).**

## Next
- Reviewer settles the interactive-authoring design → I spec the builder batches →
  build ops (edi-ui wires chrome when the surface spec lands). bus-hub at ~3-task
  marks + closeout when the bucket is done.
