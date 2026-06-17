# UI-surfacing infrastructure — the mechanism menu (reuse FIRST)

> The GATE's foundation doc. Every surface decision for the 45 features must name a
> mechanism from THIS menu. New infra is proposed only when nothing here fits, and
> then as the MINIMAL on-pattern addition. Verified against live `src/widgets` +
> three offscreen snapshots (drafting / map / blender) on 2026-06-17.
>
> `file:line` anchors drift — trust the symbol name and re-grep.

## How to SEE the live UI (the snapshot harness)

```
QT_QPA_PLATFORM=offscreen ./build/edi --workspace drafting --snapshot /tmp/d.png
QT_QPA_PLATFORM=offscreen ./build/edi --workspace map --map-file tests/data/dungeon.map.toml --snapshot /tmp/m.png
QT_QPA_PLATFORM=offscreen ./build/edi --workspace blender --ops-file samples/doric_column/doric_column_ops.toml --snapshot /tmp/b.png
```
Add `--probe x,y` to read a pixel (paint verification). Window settles via an 800 ms
`QTimer` (`app/main.cpp`); device-pixel-ratio 1 so probe coords == image coords.
Workspaces: `drafting`, `map`, `blender` (text/project/planning resolve to drafting;
`settings` is UI-only, not snapshottable).

---

## The shell skeleton (where surfaces can land)

Four slots (`ShellSlot::Main/Left/Right/Bottom`) host **features**
(`FeatureDescriptor`, `shell_architecture.md`). A `WorkspaceLayout` binds features to
slots; layout is **data** (TOML), never code. The drafting feature
(`DraftingFeature`, `src/widgets/DraftingFeature.h`) fills all four slots today.

**Two top-left regions, never merged:**
- **Activity Rail** (far-left vertical strip: `D T P R B M S`, then `+` `?`) — the
  **workspace switcher**. Swaps the whole `WorkspaceLayout`. NOT a feature launcher,
  NOT where a new feature's button goes.
- **Top Chrome** (42 px bar) — traffic lights → left-collapse → back/forward
  (rabbit-hole workspace history, NOT undo) → File / Edit / Settings menus →
  drag region → **Snap** popup → bottom("terminal")-collapse → right-collapse.

**Status bar** (bottom edge): feature publishes one line
(`mode | N selected | N objects | zoom%` + title). `DraftingFeature::publishStatus`.

---

## The mechanism menu

### M1 — The Belt / carousel (the TOOL surface)
- **What:** `BeltState` (pure ops, `src/widgets/BeltState.h`) + `BeltCrossWidget`
  (the painted carousel). It is a `FloatingPalette` ("tool_belt"). Model (user
  direction): **one ROW per tool; the row's cells are that tool's sub-features**.
  Carousel renders the active row full-size; previous/next non-empty rows **peek** as
  half-cells; wheel = one row per 120-unit notch; navigation skips empty cells.
  **Row pinning** via a `+` nub freezes a row into an always-clickable quick bar
  (`×` to kill); pins are not persisted.
- **The tool table is data:** `kDraftingTools[]` in
  `src/widgets/DraftingFeaturePanels.cpp` — `{id, label (tooltip), glyph, beltRow}`.
  Belt rendering AND the default arrangement both read it (one source of truth). The
  drawn cell face is `draftingToolFace(toolId)` returning a `BeltFace` of unit-space
  polylines/dots/ellipses — a tiny diagram of what the tool MAKES.
- **Tool inventory + F6 checklist:** `DraftingFeature::toolInventory()` (id+label),
  `beltLayoutForTools(enabledIds)`, `defaultBeltLayout()`. The settings "Tool Belt"
  page edits which tools are on the belt; `refreshBelt` re-dresses live.
- **Use it for:** any feature the user ARMS by clicking a tool — a new draw tool, or
  a new sub-feature/variant of an existing tool (adds a CELL to that tool's row).
  Rows have 6 columns; e.g. dimensions are one row (`beltRow 9`) with 5 cells today.
- **To add a tool/variant:** one row in `kDraftingTools[]` (+ a `draftingToolFace`
  arm) — the belt and default layout pick it up. **No new infra.**

### M2 — Tool-options (the per-tool PARAMETER surface)
- **What:** `DraftingInspectorPlan` (`src/drafting/DraftingInspectorPlan.cpp`) — two
  data tables. `toolOptionsTable()` maps `toolId -> option group ids`;
  `contextTable()` maps a selection context -> group ids. The Right panel is a
  **contextual stack of group containers, toggled by visibility, never rebuilt**
  (`planDraftingInspector` returns the ordered groupIds; the widget keys off ids).
- Tool options ride along WITH a selection too (so a param control doesn't vanish
  after the first draw). Groups built once via `beginInspectorGroup(groupId)` in
  `DraftingFeaturePanels.cpp`; examples: `tool_polygon` (sides spin), `tool_radius`,
  `tool_rectangle`.
- **Use it for:** any numeric/option a tool needs BEFORE/while drawing — setback,
  axis count, segment count, a kind toggle. **To add:** one row in
  `toolOptionsTable()` + a `beginInspectorGroup("tool_<x>")` builder. **No new infra.**

### M3 — The contextual Inspector (the SELECTION surface)
- **What:** Right panel, `DraftingFeature` + `DraftingFeatureInspector.cpp` +
  `DraftingFeaturePanels.cpp`. `contextTable()` keys groups off the selected kind:
  `object_shape` -> {selection_summary, style, geometry, transform, object_guides};
  `object_dimension` -> {…, dimension}; `object_guide`, `object_construction`;
  `document` (select tool, nothing selected) -> {layers, guides, calibration, …}.
- Group widgets are built once and shown/hidden; the **geometry editor** is the one
  exception — it is rebuilt per selection (`rebuildGeometryEditor`, retires spins
  with `deleteLater()` — flush `DeferredDelete` before widget lookups in tests).
- **Controls available as data-driven helpers:**
  - `makeActionButton(objectName, label, action)` — a verb button.
  - `makeConditionalButton(objectName, label, enableKey, action)` — enabled state
    driven by a projection bool key (e.g. `has_selection`), so build+refresh can't
    drift.
  - `makeToggle`, `makeDataCombo(items, onData)`, `makeGeometryFieldSpin(spec)`.
- **Use it for:** editing a SELECTED object's new field; a verb that acts on the
  selection; a readout. **To add:** a group id in `contextTable()` + a builder, or a
  button/combo in an existing group. **No new infra.**

### M4 — Pickers (color / fill / enumerated choice)
- **Color:** a `QLineEdit` color field (`styleColorField`, `styleFillColorField`,
  built in `DraftingFeaturePanels.cpp`) bound to the projection keys
  `own_stroke_color` / `own_fill_color`; the canonical **picker dialog** is
  `QColorDialog::getColor(...)` (live in `SettingsFeature.cpp:142`). Fill **opacity**
  is `m_styleFillOpacitySpin` -> `own_fill_opacity`.
- **Enumerated choice:** `makeDataCombo` (id->label, emits the data string) — the
  pattern for line style, dimension kind, wall type, layer, role, etc.
- **Use it for:** authoring a color/opacity (e.g. fill), or any closed vocabulary.
  The fill style group already exists in `object_shape` -> `style`; waking a fill is
  setting `own_fill_opacity>0` through that group. **No new infra.**

### M5 — Object list & block palette (the Left-panel NAVIGATION surface)
- **What:** `m_objectList` (QListWidget) follows `modelChanged`, click selects via the
  controller; `m_blockList` (block palette) stamps a block by id on row-click (arms a
  placement capture). `refreshBlockPalette` repopulates from `document.blocks`.
- **Use it for:** browsing/selecting objects; a click-to-place library. **No new infra.**

### M6 — PointCaptureIntent (the multi-PICK gesture surface)
- **What:** the controller-side arming mechanism. A tool/verb calls
  `begin<X>()` which arms a `PointCaptureIntent` (`DrawingCore.h`); the next canvas
  click(s) feed points to `apply<X>AtPoint(Point2D)`. Used today by Fillet, Trim,
  Radial-array center, Block-instance placement. Rejections surface via `finishEdit`.
- **Use it for:** any verb that needs the user to PICK points/objects on the canvas
  (chamfer, extend, break, region-fill seed, place-rotated, transform-instance). The
  ARMING is a belt cell (M1) or an inspector action button (M3); the PICK is this
  intent. **To add:** a new `PointCaptureIntent` value + `begin/apply` pair, mirroring
  an existing one. **No new infra** (this is established controller plumbing).

### M7 — The projection `QVariantMap` (the DATA-to-UI bridge)
- **What:** `DrawingDocumentProjection` turns the `DraftingDocument` into the
  `QVariantMap` the canvas painter + inspector read (keys like `own_fill_color`,
  `dimension_kind`, `dimension_angle_deg`, `has_selection`, `editable`). The inspector
  refresh + conditional-button enable-state read these keys.
- **Use it for:** surfacing a NEW object field to the inspector or canvas — it must
  appear as a projection key before a control can bind to it. Owned by the controller
  layer (drafting), coordinated with edi-ui. **No new infra** — add a key.

### M8 — Map browser (the Right-panel map-GRAPH surface, `map` workspace)
- **What:** `buildMapBrowserPanel` (`EdiShellWindowIo.cpp` ~700-772), read-only live
  re-projection on every `modelChanged`. Today: header
  (`N rooms · N connections · N plugs`), room footprints in authored feet
  (`document.canvasPerAuthoredUnit`), and the connection list. Drive by `objectName`
  for tests.
- **Use it for:** displaying map graph state (rooms / plugs / connections / blocks).
  **Note:** this panel host + the file edit are **edi-ui-owned** shell files — content
  changes here are *coordinated* edits, not local dept edits.

### M9 — Recipe lab panels (the `blender` workspace surfaces)
- **Right panel — `Palette / Render / Compiled` tabs.** "Add Step" lists the
  one-click op palette (`recipePaletteOpTypes` / `makeRecipeOp`: Box / Cylinder /
  Sphere / Ring) + the **Craftsmen** list (scanned from `tools/blender/craftsmen/`,
  surfaced by `--list-craftsmen`; a craftsman row seeds a `ScriptOp` with manifest
  defaults). Clicking appends an op to `m_opsStream`.
- **Bottom panel — `Steps / Editor / ASCII Proof` tabs.** Steps = the op list with
  `Remove / ↑ / ↓` reorder; clicking a step opens its **field editor** (schema
  extras drive the editable fields — `RecipeOpSchema`). Editor = the TOML op-stream
  text. ASCII Proof = the rendered silhouette.
- **The authoring fork (§9.2 of the lab arch):** ops that need a **reference** (a
  drafted profile id, a path id, a target op name) are **NOT** one-click palette
  entries — they are **authored** (pick a profile + type params, like the lathe),
  edited through the Steps field editor / Editor TOML. One-click palette is only for
  reference-free primitives.
- **Use it for:** a new recipe op surfaces as (a) a palette entry IF reference-free,
  else (b) an authored op edited via Steps/Editor; a new craftsman is a pure-Python
  drop that the palette picks up automatically; a param surfaces as a Steps schema
  field. The lab panel CONTENT is blender-lab's; the panel HOST is edi-ui's.

### M10 — Floating palettes & chrome popups (registry seams)
- **What:** `DraftingFeature::buildPalettes()` (chromeless movable panels over Main,
  positions persist as `palette.N.*` in workspace.toml — the belt is one) and
  `buildChromePanels()` (top-chrome popups — the **Snap** settings popup is one).
- **Use it for:** a cluster of controls that should float over the canvas, or a
  chrome-level popup. Prefer M1/M2/M3 first; reach for this only when a control group
  genuinely wants to float. **To add:** a `FeaturePaletteSpec` / `FeatureChromePanelSpec`.

---

## Decision order for any feature (apply in this sequence)

1. Is it a **new tool / tool variant** the user arms by clicking? → **M1** belt row/cell
   (+ **M6** capture if it needs canvas picks, + **M2** if it needs params).
2. Is it a **parameter for a tool**? → **M2** tool-options group.
3. Is it a **verb / edit / field on a SELECTED object**? → **M3** inspector
   (action button / combo / spin) (+ **M6** if it needs picks, + **M7** for a new field).
4. Is it a **color / fill / enumerated choice**? → **M4** picker.
5. Is it **map graph display**? → **M8** map browser (edi-ui-coordinated).
6. Is it a **recipe op / craftsman / op param**? → **M9** lab panels
   (palette if reference-free, else authored).
7. None fit? → propose the **minimal** on-pattern new infra and FLAG it to the hub.

## Ownership reminder (the surface spec is a hand-off, not an edit)
- The belt table, inspector groups, tool-options tables, pickers, projection keys live
  in **edi-ui** (`src/widgets`) + the **drafting controller** (`src/core`). The Map
  browser + recipe panels HOSTS are **edi-ui** shell files. UI-Integration writes the
  SPEC; the builders (edi-ui / the domain dept) wire it. Name the exact mechanism so no
  UX decision is left open.
</content>
</invoke>
