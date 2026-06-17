# UI-surface spec — edi-dungeon-map (DM-01 .. DM-15)

> Designer: `edi-ui-integration-dungeon-map`. The GATE deliverable: per feature, the
> EXISTING mechanism (M# from `docs/ui-surface/INFRA.md`), the precise interaction, the
> widget/symbol it reuses, and whether any NEW infra is needed. A builder wires from this
> with no UX decision left open.
>
> Verified against live `src/widgets` + `src/core` on 2026-06-17:
> `buildMapBrowserPanel` (`EdiShellWindowIo.cpp:700`), `m_blockList`/`refreshBlockPalette`
> (`DraftingFeaturePanels.cpp:335/353`), `beginBlockInstancePick`/`placeBlockInstance`
> (`DrawingCore.h:296/292`), `PointCaptureIntent` (`DrawingCore.h:39`), the fillet
> radius spin pattern (`DraftingFeaturePanels.cpp:1056` + `setFilletRadius`).

## Surfacing verdict at a glance

| Feature | Mechanism | Verdict |
|---|---|---|
| DM-01 view-auto-fit | M7-adjacent framing | edi-ui-coordinated shell edit (no control surface) |
| DM-02 features data/parse | — | no UI (data/parse only) |
| DM-03 features → Point markers | M5 object list + M3 tags | surface now (falls out of existing Point + tags) |
| DM-04 plug flags data/parse | — | no UI (data/parse only) |
| DM-05 persist plug flags | — | no UI (persistence only) |
| DM-06 plug flags → TOON | M8 (display) | no UI / display-only via M8 |
| DM-07 Seam-C edited round-trip | — | no UI (verify-and-close) |
| DM-08 Seam-C regression test | — | no UI (test only) |
| DM-09 region-fill boundary trace | — | no UI (pure plan; surfaces via DM-10) |
| DM-10 region-fill → filled Polygon | M1/M3 arm + M6 capture + M4 fill | surface now (arm) + edi-ui-coordinated (widget) |
| DM-11 Map browser content | M8 | **edi-ui-coordinated shell edit (headline UI)** |
| DM-12 block rotation/scale fields | — | no UI (data/persist only) |
| DM-13 export reads rotation/scale | — | no UI (export only) |
| DM-14 place rotated/scaled block | M2 tool-options + M5 palette | surface now (rotation/scale spins beside block palette) |
| DM-15 transform placed instance | M3 inspector action + M6 capture | surface now (inspector verb) |

**Net: 5 features carry a real interactive surface (DM-03, DM-10, DM-11, DM-14, DM-15);
10 are data/parse/persist/export/test with no control surface (DM-06 displays via M8).**

---

## Open UX forks (escalate to planner → hub/user)

1. **Map browser golden co-bless (DM-11).** Adding plugs + connections sections to
   `buildMapBrowserPanel` changes the `map` workspace default-shell snapshot golden. The
   panel host + file are **edi-ui-owned**; the content is ours. edi-ui must co-bless the
   re-blessed golden in the same change. FLAG: confirm edi-ui is ready to re-bless.
2. **Plug-flags display shape (DM-06/DM-11).** Flags are an open `std::vector<std::string>`
   (DM-04). For the Map browser plugs section (DM-11) and the TOON column (DM-06), display
   as a single space/`·`-joined token run appended to the plug line — NOT a fixed
   light/sight/lockable sub-column. This keeps the open-vocabulary mandate. Confirm the
   browser line shape (proposed below) reads acceptably vs. a dedicated flags column.
3. **Region-fill arming target (DM-10).** The controller arms a
   `PointCaptureIntent::RegionFill` seeded by a canvas click. WHERE the arming control
   lives is the only open UX choice: **(A)** an inspector action button in the `document`
   context (no selection needed — fill is a canvas verb), or **(B)** a belt cell on a new
   "fill" tool row. RECOMMEND **(A) inspector `document`-context action button**
   (`fillRegionButton`) — region-fill needs no selected object, mirrors how Radial/Fillet
   verbs already live as inspector buttons, and avoids a belt-row add. Confirm A vs B.
4. **DM-14 rotation/scale field placement (Left vs Right panel).** The block palette lives
   in the **Left** panel (`m_blockList`), but tool-option params (M2) canonically live in
   the **Right** inspector. RECOMMEND placing the rotation/scale spins **directly under the
   block list in the Left "Blocks" palette panel** (same panel, immediately above the
   click-to-stamp list) so the author dials transform then clicks a block — co-located with
   the action. This is the fillet-radius-spin pattern relocated. Confirm Left-panel
   placement vs. a Right-panel `tool_block` options group.
5. **NEW infra:** none required. Every DM feature maps onto an existing mechanism. The two
   new `PointCaptureIntent` values (RegionFill, TransformInstance) are established
   controller plumbing per M6, not new UI infra.

---

## DM-01 · view-auto-fit
- **Mechanism:** **M7-adjacent framing behavior** — no widget chrome. `computeFitView`
  wired into the canvas pan/zoom apply + the `--snapshot` settle.
- **Interaction:** None authored. On map load (after `createMapFromSpec`) and before the
  `--snapshot` capture, the canvas applies `computeFitView(documentBounds, w, h, padding)`
  once, framing the whole map. The user sees a framed map instead of a clipped one; no
  click, no field.
- **Reuses:** `computeFitView` (new, in `DrawingCanvasViewport.{h,cpp}`) layered on
  `clampViewportZoom`/`viewportFitRect`; `computeDocumentBounds()` controller getter;
  the `--snapshot` `QTimer` settle in `app/main.cpp:~395`; the `DrawingCanvasWidget`
  pan/zoom apply.
- **NEW infra?** No. **Verdict: edi-ui-coordinated shell edit** (touches `main.cpp` +
  `DrawingCanvasWidget`, both edi-ui-owned). No control surface.

## DM-02 · interior features data model + parse
- **Mechanism:** None — pure data/parse (`RoomSpec.features`, `RoomSpecStore.cpp`).
- **Interaction:** Author edits `room.feature.<i>.{x,y,type,name}` in the `.map.toml`. No
  interactive surface.
- **Reuses:** `parseRoomPlugs` contiguous-indexed dialect; `configDouble`/`configString`/
  `hasKey`. The geometry it feeds surfaces only via DM-03.
- **NEW infra?** No. **Verdict: no UI (data/parse only).**

## DM-03 · interior features as Point markers
- **Mechanism:** **M5 object list** (markers appear as ordinary Point objects) **+ M3
  inspector tags field** (the `feature:<type>` tag is shown/edited there). No new control.
- **Interaction:** After `createMapFromSpec`, each `RoomFeature` is an editable Point
  object on the canvas; it appears as a row in `m_objectList` and is selectable/movable
  like any Point. Selecting it shows its `feature:<type>` token in the existing inspector
  **tags** field (the documented open-vocabulary precedent). The author moves/deletes it
  with the normal selection tools.
- **Reuses:** `buildDraftingObject(... DraftingShapeKind::Point, PointGeometry{at})` (same
  minting idiom as plug markers / door leaves); `m_objectList` projection on `modelChanged`;
  the `metadata.tags` → inspector tags field path (M3); the existing Point object inspector
  context (`object_shape`).
- **NEW infra?** No. **Verdict: surface now** — falls out of existing Point + tags display;
  zero new widget, zero new control.

## DM-04 · plug flags data model + parse
- **Mechanism:** None — pure data/parse (`DraftingPlug.flags` / `RoomPlugSpec`,
  `RoomSpecStore.cpp`).
- **Interaction:** Author edits `room.plug.<i>.flags` (comma-split string) in the
  `.map.toml`. No interactive surface.
- **Reuses:** `configString` comma-split; the `plug.type` thread-through at
  `createMapFromSpec` ~2111.
- **NEW infra?** No. **Verdict: no UI (data/parse only).** (Display of the parsed flags is
  DM-11/DM-06.)

## DM-05 · persist plug flags
- **Mechanism:** None — persistence only (`plugValue`/`readPlug`, `DraftingSerialize.cpp`).
- **Interaction:** None. Flags round-trip through `.edidraw` invisibly.
- **Reuses:** the additive-tolerant codec array read/write helpers (wall_visual/asset_ref
  precedent); no version bump.
- **NEW infra?** No. **Verdict: no UI (persistence only).**

## DM-06 · plug flags → TOON export
- **Mechanism:** **M8 (display surface, indirectly)** — the export is non-UI, but the same
  flags are what the Map browser plugs section (DM-11) renders. Record as **display-only
  via M8** for the live UI; the export itself is no-UI.
- **Interaction:** None in the export. In the Map browser (DM-11), flags show as a
  trailing token run on the plug line (see DM-11). In the exported TOON, flags are a new
  `flags` column on the `plugs[]` array.
- **Reuses:** `writePlugRow` (extended signature, both overloads); the header tuple
  becomes `{room,name,edge,type,connected,flags}`. The browser side reuses DM-11's plug
  re-projection.
- **NEW infra?** No. **Verdict: no UI / display-only via M8.**

## DM-07 · Seam-C edited-doc round-trip
- **Mechanism:** None — controller/ops wiring (verify `document.rooms` populated via
  `CreateMapRoomCommand`).
- **Interaction:** None. The result is visible only indirectly: the Map browser (DM-11)
  reads `document.rooms` for its footprints, so a correctly populated `document.rooms` is
  what makes the browser's rooms section non-empty for an edited doc.
- **Reuses:** the existing `CreateMapRoomCommand` arm + `applyCommandAndEmit` batch; the
  Map browser already reads `doc.rooms` (`EdiShellWindowIo.cpp:742`).
- **NEW infra?** No. **Verdict: no UI (verify-and-close).**

## DM-08 · Seam-C regression test
- **Mechanism:** None — test only.
- **Interaction:** None.
- **Reuses:** `exportMapToToon`; `createMapFromSpec` fixture path.
- **NEW infra?** No. **Verdict: no UI (test only).**

## DM-09 · region-fill boundary trace
- **Mechanism:** None — pure plan (`planRegionFill`, `DraftingRegionFill.cpp`). Surfaces
  only through DM-10's command.
- **Interaction:** None directly; the ring it returns becomes a filled Polygon via DM-10,
  seeded by the canvas click that DM-10's capture intent owns.
- **Reuses:** `computeBounds`/`boundsContainsPoint`/`includeBounds`.
- **NEW infra?** No. **Verdict: no UI (pure plan).**

## DM-10 · region-fill emits a filled Polygon
- **Mechanism:** **M1/M3 arming + M6 PointCaptureIntent::RegionFill (the canvas pick) +
  M4 fill (color/opacity).** Arming control: per Fork 3, an **inspector action button**
  in the `document` context (`fillRegionButton`) — RECOMMENDED — mirroring how Radial /
  Fillet verbs are inspector buttons.
- **Interaction:**
  1. User clicks the **`fillRegionButton`** inspector action (no object selection needed).
  2. The controller arms `PointCaptureIntent::RegionFill`; the status/prompt shows
     "click inside a room to fill".
  3. The next canvas click seeds `fillEnclosedRegion(Point2D)`; on a valid enclosed room
     a new closed `Polygon` object is created with a non-zero `FillStyle`
     (`own_fill_opacity > 0`, default floor color) and auto-selected.
  4. A click in open/non-enclosed space is a no-op refusal surfaced via `finishEdit`.
  5. The new filled Polygon is then editable through the normal **M4 fill picker** in the
     `object_shape → style` group (`own_fill_color` field, `m_styleFillOpacitySpin`).
- **Reuses:** the `BlockInstance` capture idiom (`beginBlockInstancePick` /
  `PendingPointCapture`, `DrawingCore.h:43`, controller ~1953) mirrored as a new
  `PointCaptureIntent::RegionFill` + `begin…`/`apply…` pair; `applyCommandAndEmit` +
  `buildDraftingObject`; the existing fill style group (`styleFillColorField`,
  `m_styleFillOpacitySpin` → `own_fill_color`/`own_fill_opacity`); `makeActionButton`
  for the arming button. The arming button itself is an **edi-ui** widget wire-up.
- **NEW infra?** No (new `PointCaptureIntent` value is established M6 plumbing). **Verdict:
  surface now (arming = `fillRegionButton` inspector action) + edi-ui-coordinated (the
  button widget lives in `DraftingFeaturePanels.cpp`).**

## DM-11 · Map browser content (rooms + plugs + connections)  ← headline UI
- **Mechanism:** **M8 Map browser** (`buildMapBrowserPanel`). Add a **plugs section** and a
  **connections section** to the read-only live re-projection. **edi-ui-coordinated** (the
  panel host + file are shell-owned; the content is ours).
- **Interaction:** Read-only. On every `modelChanged` the panel re-projects. The user sees
  three labelled runs in the existing `mapBrowserList` (`QListWidget`), in this order:
  rooms (already present), then plugs, then connections. No clicks mutate; it is a graph
  readout. Footprints stay in **authored feet** (`doc.canvasPerAuthoredUnit`, the
  `:741` precedent).
- **Exact section layout + objectNames a test drives** (extend `buildMapBrowserPanel`,
  drive by `objectName` from `edi_shell_window_tests`):
  - Keep `mapBrowserTitle`, `mapBrowserSummary` (`N rooms · N connections · N plugs`),
    `mapBrowserList`.
  - **Rooms** (unchanged): rows `▸ <name>   <w> × <h>` in authored feet.
  - **Plugs section** — insert a section header item `── Plugs ──`, then one row per
    `doc.plugs`:
    `◦ <plug name>   <type> · <edge> · <connected?>` followed by the flags token run when
    non-empty, e.g. `◦ north_doorway   door · N · linked   [window · passes_light]`.
    - `name` from `plug.name` (fall back to opaque `plug.id` when empty — reuse the
      existing `plugLabel` fallback at `:750`).
    - `edge` is **derived read-only** (the `DraftingPlug` struct has NO stored edge —
      `MapToonExport.cpp deriveEdge()` picks the room side nearest `plug.anchor`). Per the
      spec, factor a tiny shared neutral helper `deriveEdge(plug, room)` (or recompute
      read-only) so the browser and the TOON export agree. Token shape: `N/E/S/W/?`
      (`edgeName`).
    - `connected` y/n derived the same way `MapToonExport.cpp:127-132` does — a plug is
      `linked` iff some `doc.connections` names it; otherwise `unlinked`.
    - `flags` (DM-04/06) appended as a `·`-joined token run in `[...]`, omitted when empty
      (Fork 2 shape).
  - **Connections section** — insert a section header item `── Connections ──`, then one
    row per `doc.connections`: `⟷ <plugA name> ↔ <plugB name>   <type>` (reuse the existing
    `plugLabel` resolver at `:750`; show `type` when non-empty, else nothing). This
    replaces/augments the current bare `⟷ A ↔ B` rows at `:759`.
  - **Test objectNames:** the section headers and rows live in `mapBrowserList`; the test
    asserts over the list's item texts (no per-row objectName today). If finer assertion is
    wanted, split into three `QListWidget`s (`mapBrowserRoomsList`, `mapBrowserPlugsList`,
    `mapBrowserConnectionsList`) — RECOMMEND keeping the single `mapBrowserList` and
    asserting by item-text prefixes (`◦`, `⟷`, `──`) to minimize golden churn. Builder's
    call; note it for edi-ui.
- **Reuses:** `buildMapBrowserPanel` refresh lambda (`EdiShellWindowIo.cpp:722`);
  `doc.plugs`/`doc.connections`/`doc.rooms`; the existing `plugLabel` resolver and
  `countLabel` pluralizer; `MapToonExport.cpp`'s `edgeName`/`deriveEdge`/`connected`
  derivation (factored into a shared neutral read-only helper).
- **NEW infra?** No. **Verdict: edi-ui-coordinated shell edit (headline UI).** Default-shell
  golden re-bless required → Fork 1.

## DM-12 · block rotation/scale fields (data + persist)
- **Mechanism:** None — data/persist only (`BlockPlacementMetadata.rotationDeg/scale`,
  `DraftingSerialize.cpp`).
- **Interaction:** None. Identity-valued (`rotationDeg = 0`, `scale = 1`) until DM-14
  writes real values from the placement spins.
- **Reuses:** additive `blockValue`/`readBlock` codec; emit-when-non-default, read-tolerant;
  no version bump.
- **NEW infra?** No. **Verdict: no UI (data/persist only).**

## DM-13 · export reads rotation/scale
- **Mechanism:** None — export only (`MapToonExport.cpp` blocks[] accumulator).
- **Interaction:** None. The real per-instance `rotationDeg`/`scale` replace the literal
  `1`/`0` placeholders in the TOON `blocks[]` array.
- **Reuses:** the grouped placement's first stamped object's `BlockPlacementMetadata`
  (FLATTEN stamps share it).
- **NEW infra?** No. **Verdict: no UI (export only).**

## DM-14 · place a rotated/scaled block
- **Mechanism:** **M2 tool-options (rotation + scale spins) + M5 block palette.** Two
  `QDoubleSpinBox` fields are the placement parameters; the M5 palette row is the arm.
- **Interaction:**
  1. In the **Left "Blocks" palette panel**, under `m_blockList`, the author sets
     **`blockRotationSpin`** (degrees) and **`blockScaleSpin`** (uniform scale) — per Fork 4,
     RECOMMENDED placement is directly below the block list, co-located with the click.
  2. The author clicks a block row in `m_blockList` → arms `beginBlockInstancePick(blockId)`.
  3. The next canvas click stamps the block at the click point, now **rotated/scaled** by
     the two spin values about the placement center (`placeBlockInstance(blockId, x, y,
     rotationDeg, scale)` consuming `transformGeometry`).
  4. Defaults (0° / 1.0) preserve today's identity placement exactly.
- **Reuses:** the **fillet-radius-spin pattern** verbatim
  (`DraftingFeaturePanels.cpp:1056`: a labelled `QDoubleSpinBox` bound to a controller
  setter) — add `blockRotationSpin` (range e.g. -360..360, step 1, decimals 1) and
  `blockScaleSpin` (range e.g. 0.01..100, step 0.1, decimals 3) with controller
  setters `setBlockPlacementRotation`/`setBlockPlacementScale` (mirror
  `setFilletRadius`/`m_filletRadius`); `m_blockList`/`refreshBlockPalette`;
  `beginBlockInstancePick`; `placeBlockInstance` (extended signature). The
  controller-side `transformGeometry` consumption is dungeon-map's; the two spins are an
  **edi-ui** widget wire-up.
- **NEW infra?** No. **Verdict: surface now (rotation/scale spins beside the block palette,
  fillet-radius-spin pattern).**

## DM-15 · transform a placed block instance
- **Mechanism:** **M3 inspector action button + M6 capture/selection.** A verb on a
  selected placed-instance object.
- **Interaction:**
  1. The author selects a stamped placed-instance object on the canvas (it carries
     `BlockPlacementMetadata.instanceId`).
  2. In the inspector (the `object_shape` context, a small **"Block instance" section**
     gated to objects with a non-empty `instanceId`), the author sets a delta-rotation
     spin (**`instanceRotationSpin`**, degrees) and a scale-factor spin
     (**`instanceScaleSpin`**), then clicks **`transformInstanceButton`**.
  3. The controller calls `transformBlockInstance(instanceId, deltaRotationDeg,
     scaleFactor)`, re-transforming every object of the FLATTEN group about the group's
     bounds center in one undo step, and updating each copy's `rotationDeg`/`scale`
     metadata.
  - Alternative arming (if the planner prefers a canvas pick over a selection-driven
    button): a `PointCaptureIntent::TransformInstance` seeded by clicking the instance —
    but RECOMMEND the selection-driven inspector verb (the object is already selectable;
    no extra pick needed), mirroring Radial/Fillet inspector verbs.
- **Reuses:** `makeActionButton` / `makeConditionalButton` (enable on a projection bool
  like `has_block_instance_selection`); the inspector group plumbing
  (`beginInspectorGroup` + `contextTable()`); the fillet-radius-spin pattern for the two
  delta spins; `transformBlockInstance` controller verb (+ `transformGeometry`). A new
  projection key (M7) `has_block_instance_selection` (or reuse `instanceId` presence)
  gates the button. The inspector group + button are **edi-ui** widget wire-ups.
- **NEW infra?** No (a new projection bool key is M7, "add a key"; an optional new
  `PointCaptureIntent` is M6 plumbing). **Verdict: surface now (inspector "Block instance"
  verb + delta spins).**

---

## Hand-off summary

- **Surface now (dungeon-map / drafting-controller side):** DM-03 (tags display falls out),
  DM-10 arm (`fillRegionButton` + `RegionFill` intent), DM-14 (placement spins + extended
  `placeBlockInstance`), DM-15 (inspector verb + `transformBlockInstance`).
- **edi-ui-coordinated shell edits:** DM-01 (`main.cpp` + `DrawingCanvasWidget`), DM-11
  (`buildMapBrowserPanel`, golden re-bless), and the widget wire-ups for DM-10/14/15
  (buttons + spins in `DraftingFeaturePanels.cpp`).
- **No UI (data/parse/persist/export/test):** DM-02, DM-04, DM-05, DM-06 (display-only via
  M8), DM-07, DM-08, DM-09, DM-12, DM-13.
- **NEW infra needed:** none. Two new `PointCaptureIntent` values (RegionFill, optional
  TransformInstance) and one projection bool key are established M6/M7 plumbing, not new
  UI infrastructure.
- **Forks to escalate:** (1) Map browser golden co-bless, (2) plug-flags display shape,
  (3) region-fill arming target A/B, (4) DM-14 spin placement Left vs Right.
