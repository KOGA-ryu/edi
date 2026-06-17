# UI-surface spec — edi-blender-lab (BL-01..15)

> The surface-design GATE deliverable for the recipe-lab batch. Each feature names the
> EXACT existing mechanism (from `docs/ui-surface/INFRA.md`), the interaction, what it
> reuses (cite symbol), and whether NEW infra is needed. Verified 2026-06-17 against live
> `src/widgets/EdiShellWindowIo.cpp` + `EdiShellWindowPanels.cpp` and
> `docs/architecture/edi-blender-lab.md`. `file:line` anchors drift — trust the symbol.
>
> **All 15 are M9 (recipe-lab panels).** The lab's authoring doctrine (§9.2): *recipe is
> truth, ASCII is proof, Blender is execution. The human composes by CLICKING; the AI
> edits the same `m_opsStream` as TOML; both mutate one stream.* The spine of these specs
> is the §9.2 authoring fork — **palette one-click** (reference-free primitive) vs
> **authored op** (needs a reference; typed into the Editor TOML, params tuned in the
> Steps field editor, like the lathe `AddRevolvedProfile`).

## The live M9 sub-surfaces (the menu these specs draw from)

| sub-surface | builder symbol | objectName | role |
|---|---|---|---|
| **Palette / Add Step** | `buildStepPalettePanel` (EdiShellWindowIo.cpp:1574) | `stepPalettePanel`, `addStep_<Type>` | one button per `recipePaletteOpTypes()`; click → `appendRecipeOp(type)` |
| **Palette / Craftsmen** | same panel, `m_craftsmen` loop (:1601) | `craftsmanPaletteTitle`, `addCraftsman_<id>` | one button per scanned manifest; click → `appendScriptStep(id)` |
| **Steps list** | `buildOpStepsPanel` (:1131) | `opStepsList`, `removeStep`/`moveStepUp`/`moveStepDown` | the op list + reorder; click a row → opens its field editor |
| **Steps field editor** | same panel, `rebuildFields` (:1315) | `opStepsFields`, `opField_<key>` | per-op scalar editor; widget kind driven by `RecipeOpSchema` (`opEditableScalars` → `RecipeFieldKind` {Number/Integer/Boolean/Choice/Text}) |
| **Bind affordance** | Number-field context menu (:1368, `showOpBindMenu`) | (on `opField_<key>`) | right-click a Number field → bind/unbind to a drafted measurement (`RecipeFieldBinding`); bound = read-only, label shows `← objectId.field` |
| **Editor (TOML)** | text tab, `recipeOpsToToml`/`recipeOpsFromToml` (:280/:307) | — | the AI-facing op-stream text; Apply re-parses into `m_opsStream` |
| **ASCII Proof** | `buildAsciiPreviewPanel` (:846), `renderOpsProjection` | `asciiPreviewPanel` | front/side/top silhouette; display-only, re-renders on `opsStreamChanged` |
| **Compiled** | `buildCompiledRecipePanel` (:895), `renderCompiledRecipeText` | `compiledRecipePanel` | the resolve+compile output the proof/Build consume; display-only |
| **Render** | `buildBlenderPreviewPanel` (:677) | `blenderPreview` | the Blender-built PNG |
| **File menu — recipe cluster** | `EdiShellWindowPanels.cpp:153-156` | `fileMenu` | Open / Save Ops Recipe · Export Resolved · Export Ops Previews |

**The authoring-fork verdict for this batch:** of the 15, **none** is a new one-click
palette entry (every new op needs a reference). The new ops (BL-01 extrude, BL-08 sweep,
BL-11 boolean) are **authored** — typed in the Editor TOML, tuned in the Steps field
editor — exactly as the lathe is. The depth verbs (BL-05/06/07/09/10) are **schema params**
on an existing arm → Steps field editor fields. The craftsmen (BL-12/13) are **pure-Python
drops** the Palette Craftsmen list picks up automatically. BL-02/03/04 are **display-only**
(proof tiers, no control). BL-14/15 ride/extend the **File-menu recipe cluster**.

---

## Open UX forks (escalate to the planner → hub/user)

1. **BL-02 ASCII draw-vs-refuse (batch open question, UX/scope).** Does the extruded
   profile render a real silhouette in the **ASCII Proof** tab (BL-02 earns its slice) or
   is it refused like the lathe/Script (OBJ-only proof, BL-02 collapses)? This is a *what
   does the proof tab show* fork, not a control choice — surface-design has no control to
   place either way (the ASCII Proof tab is display-only). **FLAG.** If refused, BL-02
   frees a slot (candidate: a Bevel param → a Steps field, or a 3D Array op → authored).

2. **BL-14 chaining affordance (NEW File-menu action, minimal).** Save/load of a *named*
   recipe ALREADY has a surface: **File ▸ Save Ops Recipe… / Open Ops Recipe…**
   (`EdiShellWindowPanels.cpp:153-154`, `recipeOpsToToml`/`recipeOpsFromToml`). The only
   missing verb is **append/chain** (`appendRecipe`) — Open *replaces* `m_opsStream`, it
   does not splice. Minimal on-pattern addition: one File-menu entry **"Append Ops
   Recipe…"** beside the two existing ones, calling a `promptAppendOpsRecipe()` that
   parses the file and runs `appendRecipe(m_opsStream, parsed)`. **FLAG** — do not build a
   heavy "Recipes" panel; the File-menu cluster is the established home.

3. **BL-15 export entry (NEW File-menu action, minimal).** No TOON-export surface exists
   for recipes today (the File cluster has *Export Resolved…* = TOML, *Export Ops
   Previews…* = ASCII). Minimal on-pattern addition: one File-menu entry **"Export Recipe
   (TOON)…"** beside *Export Resolved…*, calling a `promptExportRecipeToon()` over
   `exportRecipeStreamToToon`. **FLAG** the entry placement (File menu vs a Compiled-tab
   button); recommend the File menu, matching the other recipe exports.

4. **No profile/path picker exists (known limitation, NOT new infra here).** Authored ops
   (BL-01 extrude, BL-08 sweep) reference a drafted object by **string id**, set by typing
   in the Editor TOML — same as the lathe today (it has no profile picker either). The
   reference field renders **disabled** in the Steps field editor (read-only Text). A
   future "pick a drafted profile on the canvas" affordance (an M6 PointCaptureIntent +
   an inspector pick) would be the natural upgrade, but it is **out of this batch** and
   matches the lathe's current UX. Noted so it is not mistaken for missing work.

---

## BL-01 · AddExtrudedProfile op arm

1. **Mechanism** — M9, **authored op (NOT palette)**. Surfaces via the **Editor (TOML)**
   tab for op creation + the **Steps field editor** for its scalars. Same path as the
   lathe (`AddRevolvedProfile`), which is deliberately absent from the one-click palette.
2. **Interaction** — User types an `[[op]] type = "AddExtrudedProfile"` block in the
   Editor tab (setting `profile`, `height`, `baseZ`, `x`, `y`, `material`) → **Apply** →
   the op appears in the **Steps list** (`opStepsList`). Clicking the step opens the field
   editor: `height`/`baseZ`/`x`/`y` as Number spins, `material` as a Choice combo,
   `profile` as a disabled (read-only) reference field. The op is refused-before-build, so
   the ASCII Proof / Compiled / Render show the refusal until BL-03 lowers it.
3. **Reuses** — `RecipeOpSchema` `appendExtras`/`setExtra` (drives `opEditableScalars` →
   the `opField_<key>` widgets); `recipeOpsFromToml` (Editor Apply); the
   `AddRevolvedProfileOp` arm as the template at every site. **NOT** in
   `recipePaletteOpTypes()`/`makeRecipeOp` (needs a profile reference).
4. **NEW infra?** No. Rides existing authoring. (Profile chosen by typing the id — see
   fork #4.)

## BL-02 · Extrude ASCII framing + projection silhouette

1. **Mechanism** — M9, **display-only via the ASCII Proof tab**. No control.
2. **Interaction** — Once a lowered extrude exists in `m_opsStream`, the **ASCII Proof**
   tab re-renders on `opsStreamChanged` and (if BL-02 lands as DRAWN) shows the prism's
   front/side/top silhouette, framed by the bounds estimator. User does nothing but read
   it; the same pop-out button as the other proof panes applies.
3. **Reuses** — `buildAsciiPreviewPanel` + `renderOpsProjection`; the `BoundsEstimator`
   (RecipeOpsAscii.cpp) for framing; the `AddMoulding` ProjectionDrawer arm as the
   silhouette template.
4. **NEW infra?** No. **But see fork #1** — whether this proof is DRAWN or REFUSED is an
   open UX/scope decision; surface-design places no control either way.

## BL-03 · Resolve-lowering (profile ref → concrete extrudable points)

1. **Mechanism** — M9, **display-only via the Compiled tab** (and indirectly the ASCII
   Proof, which renders post-lowering). Pure resolve-time transform; no authoring control.
2. **Interaction** — User reads the **Compiled** tab to see the extrude lowered to its
   concrete carrier (the AddPrism-style op, per the BL-03 fork). A deleted/degenerate/open
   profile surfaces as a named refusal in the Compiled output and blocks the Render.
3. **Reuses** — `buildCompiledRecipePanel` / `renderCompiledRecipeText` (calls
   `resolveRecipeOps` + `compileRecipeOps`); the lathe lowering loop + the four shared
   profile-refusal wordings as the template.
4. **NEW infra?** No. (The lowering-carrier fork — Option A AddPrismOp — is a *core* dept
   decision, not a surface one.)

## BL-04 · Python build + OBJ proof

1. **Mechanism** — M9, **Render tab** (the Blender build) for the in-app surface; the OBJ
   golden is a CLI/test proof, not UI chrome.
2. **Interaction** — User triggers the existing Build/Render path → the lowered extrude is
   handed to `edi_craft.py` → the resulting PNG appears in the **Render** tab
   (`blenderPreview`). (The `--obj-out` OBJ proof is a developer/test artifact via the CLI,
   not a panel.)
3. **Reuses** — `buildBlenderPreviewPanel` / `showRenderImage`; the existing
   `planBlenderRender` → `ProcessRunStore` chain (adjacent, edi-ui-hosted — not edited).
4. **NEW infra?** No.

## BL-05 · Push/Pull height (one-input depth verb)

1. **Mechanism** — M9, **Steps field editor schema param** + the **bind affordance**.
   The `height` field is a `RecipeFieldKind::Number` spin that is *bindable*.
2. **Interaction** — Click the extrude step → the field editor shows a `height` spin. Type
   a value (negative = push/pull CUT; zero is refused-by-name at validate and surfaces in
   the Compiled/Render error). **Right-click** the `height` spin → the bind menu lets the
   user bind it to a drafted line's length; bound, the spin goes read-only and the row
   label shows `← objectId.length`. Unbind via the same right-click menu.
3. **Reuses** — `RecipeOpSchema` extras (the `height` Number scalar); the Number-field
   `showOpBindMenu` context-menu + `RecipeFieldBinding` (already wired for lathe/box
   numbers); `applyOpScalarEdit` for the typed commit.
4. **NEW infra?** No. The bind affordance and signed-double range already exist; a
   negative `height` is just an unconstrained spin value.

## BL-06 · Partial revolve (sweepDegrees)

1. **Mechanism** — M9, **Steps field editor schema param** on `AddRevolvedProfile`.
2. **Interaction** — Click the lathe step → field editor shows a `sweepDegrees` Number
   spin (default 360). Type a value in (0,360]; out-of-range is refused at validate and
   shows in the Compiled/Render error.
3. **Reuses** — `RecipeOpSchema` extras (the new Number scalar, threaded onto AddMoulding
   so it survives lowering); the `entasis_ratio` field-add as the template.
4. **NEW infra?** No.

## BL-07 · Screw / helix (screwRise + screwTurns)

1. **Mechanism** — M9, **Steps field editor schema params** on `AddRevolvedProfile`.
2. **Interaction** — Click the lathe step → field editor shows `screwRise` (Number,
   default 0 = no helix) and `screwTurns` (Number/Integer, default 1). Type values; 0 is
   behavior-preserving.
3. **Reuses** — `RecipeOpSchema` extras; the BL-06 field-add plumbing as the template
   (lands after BL-06, shares one validate block).
4. **NEW infra?** No.

## BL-08 · Follow-Me / sweep op

1. **Mechanism** — M9, **authored op (NOT palette)**, same path as BL-01. Surfaces via the
   **Editor (TOML)** tab for creation + the **Steps field editor** for scalars.
2. **Interaction** — User types `[[op]] type = "AddSweepProfile"` (setting `profile`,
   `path`, `material`, `baseZ`) → **Apply** → the op enters the **Steps list**. Field
   editor: `baseZ` Number spin, `material` Choice combo, and **two disabled reference
   fields** (`profile`, `path`) — both set by typing the drafted ids in the Editor TOML.
3. **Reuses** — `RecipeOpSchema` extras; `recipeOpsFromToml`; the `AddExtrudedProfileOp`
   arm (BL-01) as the template; the read-only-reference render path used for the lathe's
   `profile`.
4. **NEW infra?** No. (Two references typed in TOML — see fork #4; a path-picker is a
   future upgrade, out of batch.)

## BL-09 · Taper-along-sweep (taperEnd)

1. **Mechanism** — M9, **Steps field editor schema param** on `AddSweepProfile`.
2. **Interaction** — Click the sweep step → field editor shows a `taperEnd` Number spin
   (default 1.0 = no taper). Type a value > 0; ≤0 refused at validate.
3. **Reuses** — `RecipeOpSchema` extras; the BL-06/07/08 field-add pattern.
4. **NEW infra?** No.

## BL-10 · Inset + normal-offset (depth params)

1. **Mechanism** — M9, **Steps field editor schema params** on the extrude/prism carrier.
2. **Interaction** — Click the extrude step → field editor shows `inset` (Number, default
   0) and `normalOffset` (Number, default 0) spins. Type values; an oversized inset
   (self-intersecting) is refused-by-name at validate and shows in the error.
3. **Reuses** — `RecipeOpSchema` extras; the prism mesh builder from BL-04 (downstream).
4. **NEW infra?** No.

## BL-11 · Solid boolean op (union / subtract / intersect)

1. **Mechanism** — M9, **authored op (NOT palette)** — references two earlier ops BY NAME.
   Surfaces via the **Editor (TOML)** tab + the **Steps field editor**.
2. **Interaction** — User types `[[op]] type = "AddBoolean"` (setting `a`, `b` = earlier op
   names, `op` = Union/Subtract/Intersect) → **Apply** → step enters the **Steps list**.
   Field editor: `a` and `b` as Text fields (op-name refs, like CutFlutes' `target`) and
   `op` as a **Choice combo** (the `BooleanKind` enum). Naming a later/absent op is refused
   at validate and shows in the Compiled/Render error. The proof tier emits the two
   operands as named objects (no CSG); Render does the real boolean.
3. **Reuses** — `RecipeOpSchema` extras; the `RecipeFieldKind::Choice` combo path (same as
   the existing `z_mode`/`axis` enums); `CutFlutes` as the targets-by-name template
   (validator + ordering reused).
4. **NEW infra?** No. (Op-name refs are plain Text scalars; no picker needed since targets
   are by name, not canvas pick.)

## BL-12 · Craftsman: radial-petal bloom

1. **Mechanism** — M9, **Palette Craftsmen list entry** (pure-Python drop, auto-scanned).
2. **Interaction** — Drop `tools/blender/craftsmen/radial_petal.py` → on next launch the
   **Palette** tab shows an "Add Petal…" button under the **Craftsmen** heading. Click →
   `appendScriptStep("radial_petal")` seeds a `ScriptOp` with manifest defaults. Click the
   step → the field editor renders each manifest param by its declared type (petals =
   Integer spin, petalLength/petalWidth/centerRadius/z-rise = Number spins, material =
   material combo) in MANIFEST order.
3. **Reuses** — `load_craftsmen` scan + `--list-craftsmen` → `m_craftsmen`;
   `appendScriptStep` / `makeScriptOp`; the per-param typed widgets in `buildParamWidget`
   (EdiShellWindowIo.cpp:1258); `twisted_column.py` as the three-part template.
4. **NEW infra?** No — **no C++/widget change**; the panel picks it up automatically.

## BL-13 · Craftsman: n-fold star {n/k}

1. **Mechanism** — M9, **Palette Craftsmen list entry** (pure-Python drop), identical
   pattern to BL-12.
2. **Interaction** — Drop `tools/blender/craftsmen/nfold_star.py` → **Palette ▸ Craftsmen**
   shows its button → `appendScriptStep("nfold_star")`. Field editor: points/skip as
   Integer spins, outerRadius/innerRadius/height as Number spins, material as a combo
   (MANIFEST order). Degenerate k is clamped in the craftsman.
3. **Reuses** — same as BL-12 (`load_craftsmen`, `appendScriptStep`/`makeScriptOp`,
   `buildParamWidget`); BL-12's structure as the sibling template.
4. **NEW infra?** No — no C++/widget change.

## BL-14 · Named-recipe library + chaining

1. **Mechanism** — M9, **File-menu recipe cluster**. Save/load a named recipe rides the
   **existing** *File ▸ Save Ops Recipe… / Open Ops Recipe…*; **chaining (append) needs ONE
   NEW File-menu action** (minimal on-pattern). — **FLAGGED, fork #2.**
2. **Interaction** — Save a named recipe: *File ▸ Save Ops Recipe…* (writes the current
   `m_opsStream` as TOML, carrying its `id`/`name`). Load: *File ▸ Open Ops Recipe…*
   (replaces `m_opsStream`). **Chain/append:** a new *File ▸ Append Ops Recipe…* entry
   prompts for a recipe file and splices its ops onto the end of `m_opsStream` (with
   binding-index re-offset + namespaced op names), then emits `opsStreamChanged` so the
   Steps list grows.
3. **Reuses** — `promptSaveOpsRecipe`/`promptOpenOpsRecipe` (EdiShellWindowPanels.cpp:153)
   and `recipeOpsToToml`/`recipeOpsFromToml` for save/load; the new append entry wires a
   `promptAppendOpsRecipe()` over the `appendRecipe(target, source)` free function (BL-14
   core).
4. **NEW infra?** **YES — minimal:** one File-menu `QAction` "Append Ops Recipe…" +
   `promptAppendOpsRecipe()` (edi-ui hand). Mirrors the existing Open/Save Ops Recipe
   entries; **no new panel**. FLAGGED.

## BL-15 · TOON handoff (export resolved op stream)

1. **Mechanism** — M9, **File-menu recipe cluster** — **ONE NEW File-menu action**
   (minimal on-pattern, beside *Export Resolved…*). — **FLAGGED, fork #3.**
2. **Interaction** — *File ▸ Export Recipe (TOON)…* → file dialog → writes the **resolved**
   stream as a TOON packet. An unresolved stream (live bindings / un-lowered lathe-or-
   extrude) is refused by name, surfaced via the same warning dialog the other recipe
   exports use (`QMessageBox::warning`).
3. **Reuses** — the export-prompt pattern of `promptExportResolvedOps`
   (EdiShellWindowPanels.cpp:155) + its warning surfacing; `exportRecipeStreamToToon`
   (BL-15 core) over the existing `exportToonPacket` / `ToonPacket`; the `op.N` key scheme
   `recipeOpsToToml` already writes (so TOON and TOML speak the same addresses).
4. **NEW infra?** **YES — minimal:** one File-menu `QAction` "Export Recipe (TOON)…" +
   `promptExportRecipeToon()` (edi-ui hand). Sits beside the existing recipe exports; **no
   new panel**. FLAGGED (entry placement — recommend File menu over a Compiled-tab button).

---

## Summary table

| BL | feature | mark | mechanism | NEW infra |
|----|---------|------|-----------|-----------|
| 01 | AddExtrudedProfile arm | authored via Editor TOML + Steps | M9 | no |
| 02 | extrude ASCII framing | display-only (ASCII Proof) | M9 | no (fork #1) |
| 03 | extrude lowering | display-only (Compiled) | M9 | no |
| 04 | Python build + OBJ | display-only (Render) | M9 | no |
| 05 | push/pull height | schema param + bind affordance | M9 | no |
| 06 | partial revolve (sweepDegrees) | schema param | M9 | no |
| 07 | screw/helix | schema param | M9 | no |
| 08 | follow-me sweep | authored via Editor TOML + Steps | M9 | no |
| 09 | taper-along-sweep | schema param | M9 | no |
| 10 | inset + normal-offset | schema param | M9 | no |
| 11 | solid boolean | authored via Editor TOML + Steps (enum combo) | M9 | no |
| 12 | radial-petal craftsman | craftsman drop | M9 | no |
| 13 | n-fold star craftsman | craftsman drop | M9 | no |
| 14 | recipe library/chaining | File-menu cluster | M9 | **YES — 1 File-menu action (append)** |
| 15 | TOON handoff | File-menu cluster | M9 | **YES — 1 File-menu action (export)** |

**Surfaced: 15/15.** NEW-infra needs: 2, both single File-menu `QAction` additions on the
existing recipe cluster (BL-14 append, BL-15 TOON export) — no new panels. Open UX forks
to escalate: 4 (BL-02 draw-vs-refuse, BL-14 append entry, BL-15 export entry, the
no-profile/path-picker limitation shared by BL-01/08).
