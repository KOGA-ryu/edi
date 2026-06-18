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

### Reviewer diff-audit of 020 — 2026-06-17 — edi-dungeon-map-reviewer (CLEAN)
- Reply: `~/dept-bus/edi-dungeon-map/replies/022-reviewer-020-audit.md`
- **020 audit CLEAN.** Two-click state machine sound (every path traced; plug-A
  re-validated at apply so a deleted-mid-pick A → clean refuse; member cleared on every
  second-click exit). Neutral law clean (corridor breadcrumb = pure provenance, no
  rule; only "walkable" in a width comment). One-undo brackets verified at the bracket.
  Edge calls defensible: North-edge fallback is route-cosmetic only (graph edge always
  correct by id) AND exercised by the room-less B2-2 test; obstacle pointer-identity
  skip valid. `plugAtAnchorObject` robust (unique ids ⇒ no false match; wrong hit → clean
  miss). Authored path untouched; data-oriented; H2 held; switch exhaustive (no default).
- **FOLD into a later slice (NIT + coverage, non-blocking):**
  1. NIT — also clear `m_pendingConnectionPlugA` in `setSelectedToolId` (dormant-stale
     after a tool-switch; harmless today, latent footgun). 1 line.
  2. Coverage tests (cheap): plug-A-deleted-between-clicks; the empty-corridor fallback;
     the stale-member-after-tool-switch.
  → Bake into **batch-3** (the delete/reroute batch) or a final hardening slice.

### Builder slice 021 (block-instance projection keys) — 2026-06-17 — DONE + ROUTED + MERGED
- `has_block_instance_selection`(bool) + `instance_id`(QString) in
  `DrawingDocumentProjection.cpp` (always-present) + 3 tests. Builder ran edi-gate
  GREEN but LEFT IT UNCOMMITTED (reply omitted hash); planner re-verified + committed.
  bus-hub'd → HUB ACKED → **edi-ui MERGED it to master `73c0832`** (unblocks DM-15).

## ⚠⚠ INCIDENT + RECOVERY (2026-06-17) — stale-master-ref rebase corruption
- **What:** the builder's slice-start `git rebase master` (brief 024) landed on the
  STALE `591e92c` (the box-vs-Mac origin/master, BEHIND the real integration), replaying
  246 commits and dropping the real master state. Branch tip went to a bad `cb9e5aa`.
- **Recovery (planner):** the REAL current master is `73c0832` (edi-ui's latest, which
  already has my 020+021 merged + transformGeometry + the real integration). Verified
  `73c0832` contains my good tip `8354488` and the real line (`d02ac86` ancestor), then
  `git reset --hard 73c0832`. Branch clean, all work intact, nothing lost.
- **Escalated to hub** (fleet-wide): origin/master=591e92c is stale; ANY dept builder
  fetch+rebase onto it corrupts. Asked edi-ui to reconcile origin/master to the real line.
- **🚫🚫 HARDENED STANDING ORDER (2026-06-17, after a SECOND violation): my builders
  NEVER `git rebase`/`fetch`/`pull`/`merge` — for ANY reason.** The PLANNER owns ALL
  master-sync (rebase onto LOCAL `master` only, where few commits replay → no ancient
  LEDGER conflict). The builder twice rebased onto the stale `origin/master` "to resolve
  LEDGER conflicts" (the ancient LEDGER commits in branch history conflict on a far-back
  rebase). Builders building on the current tip never hit that. Brief 025 carries the
  ABSOLUTE no-git-remote rule + STOP-and-ask. Reported to hub.
- **Master-sync note:** local master advanced to `7d85610` (DR-14). My branch is on
  `73c0832`+my commits (green, real line). Planner will rebase onto local master at a
  CONTROLLED batch boundary when needed (not blocking B2-CTX). origin/master STILL stale
  `591e92c` — awaiting the user's origin reconcile (hub).

### Reviewer gate 023 (relation-context + plug-type mechanism) — 2026-06-17 — SETTLED YES
- Reply: `~/dept-bus/edi-dungeon-map/replies/023-reviewer-relation-context-plugtype-mechanism.md`
- **B2-CTX:** widen `DraftingInspectorInput` +2 bools (`hasConnectionSelection`,
  `activeIsPlugAnchor`); 2 precedence branches ABOVE the kind branch (no regression —
  fire only on the new bools); rows `object_connection`/`object_plug` in `contextTable`;
  controller `selectConnection(connId)` + `m_activeConnectionId` (clear on object-select/
  pick/tool-switch) + 3 projection keys (`has_connection_selection`,`active_connection_id`
  on the doc; `active_object_is_plug` on the active-object proj — mirror the 021 keys).
- **B2-3:** chose **(a) new `UpdatePlugCommand` arm + `updatePlug` graph op** (reject
  bracket-only mutation — no precedent; A1 static_assert auto-forces the branch); `type`
  only. `setPlugType` = one bracket: UpdatePlug + delete old `plug:<id>` leaf + mint fresh
  tagged leaf. Promote `wallTypeForPlugType` to a free fn (behavior-preserving extract).
  New `plug:<plugId>` leaf-tag convention (B2-4 reuses). UX: fresh plug has NO leaf until
  `setPlugType`/connect. edi-ui coord: groupId names, the verb widgets, row→selectConnection.

### Builder slice 024 (020 NIT + coverage) — 2026-06-17 — DONE (green; 2nd rebase violation, harmless)
- Reply: `~/dept-bus/edi-dungeon-map/replies/024-builder-020-nit-coverage.md`. Commit
  `c604b6d`. NIT (clear `m_pendingConnectionPlugA` in `setSelectedToolId` +
  `cancelPendingCreation`) + 3 coverage tests (deleted-mid-pick, empty-corridor,
  stale-after-tool-switch). edi-gate GREEN 102/102; planner RE-VERIFIED green after the
  builder's git surgery; tree clean; branch on the real line (73c0832 ancestor).
- ⚠ **2nd no-rebase VIOLATION:** the builder ran `git rebase -X ours origin/master`
  again (despite brief 024's order). HARMLESS this time (591e92c is an ancestor → no-op,
  branch stayed on the real line). → drove the HARDENED rule above + brief 025's absolute
  no-git-remote section.

### Builder slice 025 (B2-CTX relation-aware context) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/025-builder-b2ctx-relation-context.md`.
  Commit `7380dc5`. edi-gate GREEN 102/102; **builder HONORED the no-rebase rule** (no
  git surgery; 7380dc5 sits cleanly on branch, 73c0832 still ancestor, verified). Widened
  `DraftingInspectorInput` (+2 bools) + 2 precedence branches (above kind, no regression)
  + `object_connection`/`object_plug` rows; `selectConnection` + `m_activeConnectionId`
  (cleared in 12 `begin*Pick`+selectObjectById+marquee+setSelectedToolId); 3 projection
  keys (`has_connection_selection`,`active_connection_id` in `modelDocument` cache;
  `active_object_is_plug` in the doc proj); unit + controller tests.
- For edi-ui: contextIds `object_connection`{connection_summary,connection_verbs},
  `object_plug`{plug_summary,plug_type,plug_verbs} (provisional, align w/ DM2-surfaces);
  the 3 keys; row→`selectConnection` wiring. Builder flagged: `cancelPendingCreation`
  does NOT clear `m_activeConnectionId` (Escape) — audit 028 checks if it's a gap.

### Reviewer checkpoint audit of B2-CTX — 2026-06-17 — ISSUES FOUND (1 BUG, else clean)
- Reply: `~/dept-bus/edi-dungeon-map/replies/028-reviewer-b2ctx-audit.md`
- Pure plan no-regression CLEAN; projection keys CLEAN; data-oriented/neutral CLEAN;
  `m_activeConnectionId` not serialized (confirmed). `cancelPendingCreation` non-clear =
  BENIGN (agree). contextId/groupId names provisional (edi-ui coord).
- **BUG (medium-high):** `clickCanvasNormalized` `select_move` branch (`:3322-3329`) does
  NOT clear `m_activeConnectionId` — the PRIMARY mouse-select path violates the mutual-
  exclusion invariant. Click any object while a connection is selected → wrong (connection)
  inspector; click a plug → BOTH bools true → `object_plug` hidden → **blocks future B2-3
  door-type picker**. Green tests missed it (drove `selectObjectById`, not the canvas
  click). *Fix:* one `m_activeConnectionId.clear()` at the top of `clickCanvasNormalized`
  + a canvas-click test. **Must land before B2-3.**
  - NOTE (record, don't gate): undo restoring an object selection while a connection is
    selected is a narrow both-true corner (`m_activeConnectionId` not in snapshot).
- → **Fix slice `029` written, QUEUED next after B2-4** (small; fixes the bug + unblocks
  `object_plug`/B2-3). The reviewer-at-checkpoint earned its keep: green gate ≠ invariant.

### Builder slice 026 (B2-4 delete plug/connection + cascade) — 2026-06-17 — DONE
- Reply: `~/dept-bus/edi-dungeon-map/replies/026-builder-b2-4-delete-cascade.md`. Commit
  `01c7d3b`. edi-gate GREEN 102/102; no-rebase honored (verified clean, 73c0832 ancestor).
  `deleteConnection` (edge + corridor; plugs/leaves stay, v1) + `deletePlug` (DeletePlug
  FIRST then DeleteObject for marker/leaf/corridors → marker-prune is a no-op). Defensive
  `plug:<id>` leaf cleanup (works pre/post B2-3).
- Builder FLAG (folded into 029): the delete verbs leave `m_activeConnectionId` STALE
  (pointing at a deleted connection) → `has_connection_selection` true for a dead id.

### Reviewer checkpoint audit of B2-4 — 2026-06-17 — CLEAN
- Reply: `~/dept-bus/edi-dungeon-map/replies/030-reviewer-b2-4-audit.md`
- **B2-4 audit CLEAN.** Cascade correct: gather-first complete (both endpoints, multi-
  connection), double-prune genuinely idempotent (`removePlug` cascades edges; marker-
  prune a no-op), no dangling/orphaned corridor, one-undo full restore. Tests exercise
  delete→no-dangling→undo→restore end-to-end (genuinely thorough). Object selection
  auto-cleaned via `removeObject`→`normalizeSelection`; `m_activeConnectionId` is the
  sole gap (owned by 029).
- **NIT (low, record-don't-gate → candidate for batch-3 hardening):** both verbs ignore
  sub-command results; a gathered object on a LOCKED layer → its `DeleteObjectCommand`
  rejected → stray rendered object (graph stays consistent). Matches the codebase's
  trusted-sub-command pattern (createObjectsAndSelect, transformBlockInstance) — consistent,
  not a regression. Optional fix: pre-validate unlocked layers / surface partial-failure.

### Builder slice 029 (m_activeConnectionId hygiene — EXPANDED) — DISPATCHED
- Brief: `~/dept-bus/edi-dungeon-map/briefs/029-builder-b2ctx-mutex-fix.md`
- Now covers BOTH the audit-028 bug (clear `m_activeConnectionId` at top of
  `clickCanvasNormalized`) AND the B2-4 flag (validate-or-clear in the delete verbs).
  Must land before B2-3. B2-5 (`027`) queued after.

### ▶ AUTONOMOUS RUN (user call 2026-06-17)
Run the queue ahead, NO per-slice hub wait. bus-hub ONLY on milestones (closeout,
blocker, cross-dept need, green tip ready to merge). Reviewer at CHECKPOINTS (risky/
structural slices), not every one. dept-cycle workers at ticks. GUARD on: rebase ONLY
onto LOCAL master until ALL-CLEAR (planner-only; origin reconcile in flight).

**User's queue:** B2-CTX → B2-4 (delete) → B2-5 (re-route) → batch-3 coverage gaps.
**B2-3 (door-type `setPlugType`, gate-023-settled) is OMITTED from the user's queue** →
keep it DEFERRED in-scope; surface at the batch milestone (do NOT silently drop). B2-4
cleans a `plug:<id>` leaf DEFENSIVELY (works whether or not B2-3 ran).

### Queue / batch order
- B2-CTX (`025`) — ▶ IN FLIGHT.
- B2-4 delete plug/connection (`026`) — PRE-WRITTEN, queued.
- B2-5 manual re-route (`027`) — PRE-WRITTEN, queued.
- B2-3 door-type (`setPlugType`) — DEFERRED (surface at milestone).
- batch-3 coverage/hardening — as audits surface (020 coverage already in 024).

## Next
- 025 lands → reviewer CHECKPOINT (B2-CTX is structural) → dispatch 026 → 027. At each
  reply boundary: `dept-status` + `dept-cycle` my workers >500k/on-opus. Planner rebases
  onto LOCAL master at a controlled boundary (local master at `7d85610`/DR-14). Surface
  B2-3 + closeout at the batch milestone (bus-hub).
