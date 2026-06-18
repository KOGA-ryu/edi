# UI-surface spec — edi-dungeon-map BATCH-2 (interactive corridor + door authoring)

> Designer: `edi-ui-integration-dungeon-map`. GATE deliverable: surface the **interactive
> authoring of the plug / connection / corridor / door graph** in the live tool, on
> EXISTING infra (M# from `docs/ui-surface/INFRA.md`). Read + write docs only; no code.
>
> **STATUS: FINAL — all decisions ratified by the hub 2026-06-17; batch-2 gate complete,
> bucket released.** The "Ratified decisions" section below froze the 4 forks (with the
> designer's recommendations) plus the relation-aware `contextForKind` requirement.
>
> Verified against live source on 2026-06-17:
> belt table `kDraftingTools[]` (`DraftingFeaturePanels.cpp:55`), `PointCaptureIntent` +
> the `begin*Pick`/`apply*AtPoint` capture idiom (`DrawingCore.h:39`, `RegionFill`/
> `BlockInstance` siblings already shipped), `m_wallType` `makeDataCombo` →
> `setSelectedWallType` (`DraftingFeaturePanels.cpp:764`), the graph ops
> `addPlug`/`declareConnection`/`removePlug`/`undeclareConnection` (`DraftingGraphOps.h`),
> corridor routing `routeCorridorCenterline`/`corridorWalls`/`planCorridor`
> (`DraftingCorridor.h`), and the `.map.toml` authoring precedent
> `createMapFromSpec` (`DrawingDocumentController.cpp:~2369-2540`, where connections →
> corridors + `wallTypeForPlugType(plug.type)` door leaves are emitted today).

---

## Enumerated feature set + scope divergence from the docs

The engine side is DONE (`tool-backlog.md` Phase A corridors-v2 + Phase B doors; graph
work-order S0–S6) — but **only from a `.map.toml` batch parse** (`createMapFromSpec`).
**There is NO interactive controller verb today** for placing a single plug, declaring a
single connection, or routing one corridor on the canvas. The graph COMMANDS exist
(`addPlug`/`declareConnection` + their `DraftingCommand` arms, S1/S2); the corridor/door
GEOMETRY builders exist (`routeCorridorCenterline`/`corridorWalls`,
`wallTypeForPlugType`). Batch-2 = **wire those existing commands + builders to
interactive capture verbs + chrome.** This matches the brief's spine. Enumerated:

| # | Feature | Verdict |
|---|---------|---------|
| **B2-1** | Plug authoring — place + name + type a plug on an opening | surface now (controller verb) + edi-ui chrome |
| **B2-2** | Connection authoring — pick plug A ↔ plug B → route an independent corridor | surface now (controller verb) + edi-ui chrome |
| **B2-3** | Corridor display/edit + re-route on plug move | mostly falls out (M5/canvas) + 1 controller verb (re-route) |
| **B2-4** | Door authoring on a plug — neutral render type (WallType) | surface now — **reuses `wallTypeCombo` exactly** |
| **B2-5** | Plug / connection type + flags tagging | surface now — M4 combos + flags field (DM-04 reuse) |
| **B2-6** | Corridor/door polish — corridor width/type tag | surface now — M2 tool-option + M3 combo |

**Two structural realities the "belt tools" framing glosses (both ratified below, both
real work — not trivial "add a row"):**
1. **A connection is NOT a document object** — it is a relation (`DraftingDeclaredConnection`
   on the document, referenced by id). The inspector (M3) keys context off the *selected
   object kind*, and the canvas selects *objects*. So **selecting a connection to edit/
   re-route/delete it has no existing surface.** → resolved by Decision 1 (Map-browser
   row click sets a "selected connection" on the controller).
2. **The inspector context resolution must become RELATION-AWARE.** Today
   `contextForKind(DraftingShapeKind)` (`DraftingInspectorPlan.cpp:59`) switches **purely on
   geometry kind**. The new `object_plug` and `object_connection` contexts key on
   **relation membership**, NOT kind: "this selected `Point` is a plug *anchor*" and
   "a connection is *selected* (no object selected at all)". So the context-resolution input
   must widen from a bare `DraftingShapeKind` to carry *is-this-object-a-plug-anchor* and
   *is-a-connection-selected* — a real (if on-pattern) extension of `planDraftingInspector`'s
   input, **more than adding a `contextTable()` row**. Builders: this is the load-bearing
   structural change behind B2-1/2-4/2-5; size it as such.

---

## Ratified decisions (hub, 2026-06-17 — froze the 4 forks + the relation-aware note)

1. **Connection selection = Map-browser row click (option 1B, RATIFIED).** Clicking a
   `⟷ A ↔ B` row in `mapBrowserList` sets a "selected connection" on the controller; the
   inspector then shows the `object_connection` group. (Option 1A — tagging the corridor's
   wall objects with the connection id — was rejected: it drags connection identity onto
   wall geometry and is fragile when a corridor is edited/split.) The Map browser already
   lists connections (batch-1 DM-11) and is the natural graph-navigation surface; clickable
   rows mirror `m_objectList` click-to-select. **This decides that B2-3/B2-5 connection
   edits land in the inspector via a browser-driven selection**, and requires the
   relation-aware `contextForKind` widening above.
2. **Plug placement pick = free canvas click for v1 (RATIFIED).** The author eyeballs the
   gap and may nudge later; the `.map.toml` author already supplies `at`. Snap-to-opening
   (snap the pick to the nearest wall-opening/midpoint) is a recorded FOLLOW-UP, not v1.
3. **Connection capture = two-click-arm-once (RATIFIED).** Click the Connect tool once to
   arm; the first canvas click resolves to plug A, the second to plug B, then the corridor
   routes — **neither plug need be pre-selected**. (The select-then-one-click Fillet shape
   was rejected.) Implemented as a held first-plug field across a two-stage capture.
4. **No standalone door tool (RATIFIED).** A door LEAF + doorway opening auto-emit for a
   *connected* plug from the plug's `type` (`wallTypeForPlugType`); "door authoring" =
   setting the plug `type` via `plugTypeCombo`. This matches the engine path exactly; an
   unconnected secret plug stays flush.

**NEW infra: none.** Three belt-tool rows (M1), two-three `PointCaptureIntent` values +
held-plug state (M6), the relation-aware `object_plug`/`object_connection` inspector
contexts (M3 — see the structural note), reused `makeDataCombo`/`QLineEdit`/`QDoubleSpinBox`
(M2/M4), and one Map-browser row-click (M8) cover everything.

---

## B2-1 · Plug authoring (place + name + type a plug on an opening)
- **Mechanism:** **M1 belt tool** (`plug_tool` cell) **+ M6 single-point capture** (place
  the marker + mint the plug) **+ M3 inspector** (name field + type combo, see B2-5).
- **Interaction:**
  1. User clicks the **Plug** belt cell → arms `PointCaptureIntent::PlugPlacement`
     (prompt: "click an opening to place a plug").
  2. Next canvas click (free point per Decision 2) → controller emits a `Point` marker at
     the click (provenance `"plug"`, tag `plug:<name>`) **and** mints + `addPlug`s a
     `DraftingPlug` anchored to that marker id, in one edit bracket; the marker auto-selects.
  3. The selected plug marker shows, in the inspector, a **plug name field**
     (`plugNameField`, a `QLineEdit`) and a **plug type combo** (B2-5). The author types a
     name / picks a type; the new plug appears in the Map browser plugs section
     (batch-1 DM-11) automatically on `modelChanged`.
- **Reuses:** the belt table `kDraftingTools[]` (one row `{ "plug_tool", "Plug", "Pl", <row> }`
  + a `draftingToolFace` arm); the `RegionFill`/`BlockInstance` capture idiom
  (`begin…Pick`/`apply…AtPoint`, `DrawingCore.h:349`) mirrored as
  `beginPlugPlacementPick()`/`applyPlugAtPoint(Point2D)`; the `addPlug` graph op +
  `CreatePlugCommand` arm; the `emitMarker`/`buildDraftingObject(... Point ...)` path that
  `createMapFromSpec` uses for plug markers; `makeActionButton`/`QLineEdit` for the name
  field; the plug type combo from B2-5.
- **NEW infra?** No. New belt row (M1) + new `PointCaptureIntent::PlugPlacement` +
  `begin/apply` pair (M6) + the relation-aware `object_plug` inspector context (M3 — keyed
  on "selected Point is a plug anchor", see structural note). **Verdict: controller verb
  (ours) = `beginPlugPlacementPick`/`applyPlugAtPoint`; edi-ui chrome = belt cell + face,
  plug inspector group, name field.**

## B2-2 · Connection authoring (pick plug A ↔ plug B → route a corridor)
- **Mechanism:** **M1 belt tool** (`connect_tool` cell) **+ M6 two-target capture**
  (pick plug A's marker, then plug B's marker) → `declareConnection` + route an
  **independent editable corridor** (the ratified v2: one corridor per connection, no merge).
- **Interaction:**
  1. User clicks the **Connect** belt cell → arms `PointCaptureIntent::ConnectionPlugA`
     (prompt: "pick the first plug").
  2. First canvas click resolves to the nearest plug marker → held as plug A; prompt
     advances to "pick the second plug" (capture transitions to `ConnectionPlugB`).
  3. Second click resolves to plug B → controller `declareConnection(A,B)` AND routes the
     corridor: build a `CorridorSpec` from the two plugs' anchors + edges, call
     `routeCorridorCenterline` (around other rooms as obstacles) → `corridorWalls`, carve
     the doorway openings + emit the door leaves (`wallTypeForPlugType(plug.type)`), all in
     ONE edit bracket (one undo). The corridor wall objects + door leaves are independent
     editable geometry; the connection appears in the Map browser connections section.
  4. Picking the same plug twice, or a click that resolves to no plug, is a no-op refusal
     surfaced via `finishEdit`.
- **Reuses:** the belt table (one row `{ "connect_tool", "Connect", "Cn", <row> }` + face);
  the capture idiom extended to two stages (Decision 3) — a `ConnectionPlugA`/`ConnectionPlugB`
  pair (or one intent + a held-`m_pendingPlugA` field), mirroring how `Fillet` resolves a
  click to an *object* then acts; `declareConnection` op + `DeclareConnectionCommand` arm;
  the corridor geometry path **verbatim from `createMapFromSpec`** (`CorridorSpec`,
  `routeCorridorCenterline`, `corridorWalls`, `wallTypeForPlugType`,
  `DrawingDocumentController.cpp:2369-2540`) — factored into a controller helper both the
  batch path and the interactive verb call; nearest-plug-marker resolution reuses the
  click→object resolution that Trim/Fillet already do.
- **NEW infra?** No. New belt row (M1) + new two-stage `PointCaptureIntent` + held-plug
  state (M6, established plumbing). **Verdict: controller verb (ours) =
  `beginConnectionPick`/`applyConnectionAtPoint` (two-stage) + the corridor-routing helper;
  edi-ui chrome = belt cell + face + the two-step prompt.**

## B2-3 · Corridor display / edit + re-route on plug move
- **Mechanism:** **mostly falls out** of existing object editing (**M5 object list +
  canvas**) for display/move; **M3 inspector action** for an explicit re-route verb.
- **Interaction:**
  - The routed corridor is ordinary `WallGeometry` objects (provenance `"corridor"`) +
    door leaves — they appear in `m_objectList`, select/move/delete like any wall. No new
    surface for display/edit.
  - **Re-route:** when the author moves a plug (its anchor marker) or a room, the corridor
    does not auto-follow (the objects are independent geometry). Surface an explicit
    **`rerouteConnectionButton`** inspector action on the selected connection (Decision 1)
    that re-runs `routeCorridorCenterline`/`corridorWalls` for that connection, replacing
    its old corridor objects in one undo step. (Auto-reroute-on-move is OUT — it would
    couple independent geometry; the explicit verb matches the v2 "author moves each
    corridor separately" fork.)
- **Reuses:** `m_objectList` projection (M5); the canvas selection/move path (no new code);
  `makeActionButton` + a `rerouteConnection(connectionId)` controller verb reusing the
  same corridor helper as B2-2; the connection-selection surface from Decision 1 (Map
  browser row click).
- **NEW infra?** No. **Verdict: display/move falls out (no UI); re-route = controller verb
  (ours) `rerouteConnection` + edi-ui chrome inspector button (lands in the
  `object_connection` group from Decision 1).**

## B2-4 · Door authoring on a plug (neutral render type)
- **Mechanism:** **M4 picker / M3 inspector combo** — the door's render type is the plug's
  neutral `type`, set through a combo that **reuses `wallTypeCombo` exactly**. There is NO
  separate door tool (Decision 4): a connected plug auto-emits its door leaf from the type.
- **Interaction:**
  1. Author selects a plug (its marker) → the plug inspector group (B2-1) shows a **plug
     type combo** (`plugTypeCombo`).
  2. Picking `Door` / `Window` / `Secret` / `Portal` sets the plug's `type`; on the next
     model rebuild the connected plug's door leaf renders with the matching `WallType`
     (`wallTypeForPlugType`). An UNCONNECTED secret plug stays flush (no leaf) — automatic,
     correct.
  3. Directly editing a door-leaf wall object also still exposes the existing
     **`wallTypeCombo`** in `object_shape` (a leaf is a `WallGeometry`) — so a one-off
     render-type override on the leaf itself is already surfaced today, no new control.
- **Reuses:** `m_wallType = makeDataCombo("wallTypeCombo", { solid/door/window/secret })` →
  `setSelectedWallType` (`DraftingFeaturePanels.cpp:764`) as the EXACT precedent for the new
  `plugTypeCombo`; `wallTypeForPlugType` (`DrawingDocumentController.cpp:2370`) which already
  maps plug type → door render; a `setSelectedPlugType(typeId)` controller verb (sibling of
  `setSelectedWallType`).
- **NEW infra?** No. `plugTypeCombo` is a `makeDataCombo` clone; `setSelectedPlugType` is a
  `setSelectedWallType` sibling. **Verdict: surface now — controller verb (ours)
  `setSelectedPlugType`; edi-ui chrome = `plugTypeCombo` in the plug inspector group.**

## B2-5 · Plug / connection type + flags tagging
- **Mechanism:** **M4 `makeDataCombo`** (plug type, connection type) **+ M3 fields** (plug
  name, plug flags — DM-04 reuse). Reuses the dimension-kind / wall-type combo precedent.
- **Interaction:**
  - **Plug:** select the plug marker → inspector plug group shows `plugNameField`
    (`QLineEdit` → `setSelectedPlugName`), `plugTypeCombo` (B2-4), and a **plug flags
    field** (`plugFlagsField`, a comma-split `QLineEdit` → `setSelectedPlugFlags`) reusing
    the DM-04 open-vocabulary `flags` data already parsed/persisted/exported. Flags show in
    the Map browser plug line (batch-1 DM-11) and the TOON export (DM-06) automatically.
  - **Connection:** select a connection (Decision 1 — Map-browser row click) → inspector
    connection group shows a **`connectionTypeCombo`** (or a free `connectionTypeField`) →
    `setSelectedConnectionType` (neutral role tag "corridor"/…), and a **delete**
    (`deleteConnectionButton` → `undeclareConnection`).
- **Reuses:** `makeDataCombo` (the `wallTypeCombo`/`dimensionKindCombo` pattern); `QLineEdit`
  name/flags fields (the `objectMaterialField`/`blockNameField` pattern); the DM-04
  `DraftingPlug.flags` vector + its parse/persist/export (no new data); `makeActionButton`
  for delete; new controller verbs `setSelectedPlugName`/`setSelectedPlugFlags`/
  `setSelectedConnectionType`/`deleteConnection` (siblings of `setSelectedWallType`).
- **NEW infra?** No. New relation-aware M3 plug + connection inspector contexts/groups (see
  the structural note) + M4 combos. **Verdict: surface now — controller verbs (ours) for the
  setters; edi-ui chrome = the plug + connection inspector groups + combos/fields.
  Connection editing rides Decision 1.**

## B2-6 · Corridor / door polish (corridor width + type tag)
- **Mechanism:** **M2 tool-options** (corridor width spin, beside the Connect tool) **+ M3
  combo** (corridor/connection type tag — folded into B2-5's `connectionTypeCombo`).
- **Interaction:**
  - **Corridor width:** when the Connect tool is armed, a tool-options **`corridorWidthSpin`**
    (a `QDoubleSpinBox`) sets the walkable gap used by the next routed corridor (default
    `0.045`, the `kCorridorWidth` constant). Cloned from the fillet-radius-spin pattern.
  - **Corridor/connection type tag:** the neutral `type` on the connection (B2-5
    `connectionTypeCombo`) — "corridor" is the default; the author may tag "stairs"/"bridge"/…
    as a neutral token the engine interprets. No separate surface.
- **Reuses:** the fillet-radius-spin pattern (`DraftingFeaturePanels.cpp:1056`: labelled
  `QDoubleSpinBox` bound to a controller setter) → `corridorWidthSpin` +
  `setCorridorWidth`/`m_corridorWidth` (mirror `setFilletRadius`/`m_filletRadius`); the
  `toolOptionsTable()` + `beginInspectorGroup("tool_connect")` M2 plumbing;
  `kCorridorWidth` (`DrawingDocumentController.cpp:2369`) as the default; B2-5's connection
  type combo.
- **NEW infra?** No. M2 tool-options group (one row + a builder) + reused M3 combo.
  **Verdict: surface now — controller verb (ours) `setCorridorWidth`; edi-ui chrome =
  `corridorWidthSpin` tool-option + the connection type combo (B2-5).**

---

## Hand-off summary

- **Controller verbs (dungeon-map owns):** `beginPlugPlacementPick`/`applyPlugAtPoint`
  (B2-1), `beginConnectionPick`/`applyConnectionAtPoint` two-stage + the corridor-routing
  helper factored out of `createMapFromSpec` (B2-2), `rerouteConnection` (B2-3),
  `setSelectedPlugType` (B2-4), `setSelectedPlugName`/`setSelectedPlugFlags`/
  `setSelectedConnectionType`/`undeclareConnection`-wrapper (B2-5), `setCorridorWidth` (B2-6).
  All mirror existing `setSelected*`/`begin*Pick` siblings and call existing graph ops +
  corridor builders.
- **edi-ui chrome wiring:** three belt rows + `draftingToolFace` arms (`plug_tool`,
  `connect_tool`; door is NOT a tool per Decision 4); the two/three new `PointCaptureIntent`
  values; the **relation-aware inspector contexts** `object_plug` + `object_connection`
  (name field, type combos, flags field, delete + re-route buttons) — requires widening
  `planDraftingInspector`'s context input beyond bare `DraftingShapeKind`; the
  `corridorWidthSpin` tool-option; the Map browser connection-row click-to-select
  (Decision 1); the `map`-golden re-bless.
- **No UI / falls out:** corridor + door geometry display/move (ordinary objects via M5 +
  canvas); plug/connection appearance in the Map browser (batch-1 DM-11, automatic on
  `modelChanged`); plug flags display + TOON export (DM-04/06, automatic).
- **NEW infra needed:** none. Belt rows (M1), capture intents + held-plug state (M6),
  relation-aware inspector contexts (M3 — more than a kind row, see structural note),
  `makeDataCombo`/`QLineEdit`/`QDoubleSpinBox` (M2/M4), and one Map-browser row-click (M8)
  are all established patterns.
- **Ratified decisions (hub 2026-06-17):** (1) connection selection = Map-browser row click
  (1B); (2) plug pick = free click v1 (snap-to-opening deferred); (3) connection capture =
  two-click-arm-once; (4) no standalone door tool (door render from plug type). Plus the
  relation-aware `contextForKind` widening is a recognized structural requirement, not a
  trivial table row.
</content>
