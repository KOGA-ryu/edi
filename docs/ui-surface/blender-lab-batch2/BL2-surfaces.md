# UI-surface spec — edi-blender-lab BATCH-2 (the canonical 8 deferred polish items)

> Batch-2 of the surface-design GATE. The **canonical 8 deferred polish items**, taken
> verbatim from the batch-1 closeout `docs/closeouts/blender-lab-feature-batch.md`
> ("Deferred polish", on `dept/blender-lab`). **Supersedes the candidate list in brief 005**
> (Solidify / Bisect / Section-plane / 3D-Array / 3D-Mirror were *never-in-scope* menu ops,
> not deferred — that earlier enumeration was wrong and is dropped).
>
> **STATUS: FINAL — both decisions ratified by the hub 2026-06-17; batch-2 gate complete,
> bucket released.**
>
> Same precedent as batch-1 (`docs/ui-surface/blender-lab/BL-surfaces.md`): recipe-lab
> features are **M9**, and the §9.2 authoring fork is the spine. These 8 are
> **mesh-quality / validate-refinement** items, mostly Python-build-side — so the honest
> verdict for most is **"no UI"** or **"display-only"**; I say so and cite WHY rather than
> invent controls. Verified 2026-06-17 against the closeout + live `src/recipe/*` and
> `src/widgets/EdiShellWindowIo.cpp`. Read + write docs only. `file:line` anchors drift —
> trust the symbol.

## Enumeration — the canonical 8 (verbatim from the closeout) + verdict

| # | item | verdict | mechanism |
|---|------|---------|-----------|
| 1 | **Watertight helix** (end caps + closure) — BL-07 v1 helix is an open ribbon | **no UI** (internal mesh) | — |
| 2 | **`sweep_degrees` × `screw_rise` silent-override validate-WARNING** — BL-07 ignores partial sweep in v1 | **display-only** | M9 Compiled/Render warning text |
| 3 | **Edge-incidence manifold assert in the smoke** (lock watertightness) — BL-06 note | **no UI** (smoke assert) | — |
| 4 | **`bounds_of` arc/helix/sweep tightness** — loose-but-correct (±max_radius) today | **no UI** (internal mesh framing) | — |
| 5 | **Boolean proof-duplication** — BL-11 emits operands standalone AND tagged; suppress the consumed standalone to mirror bpy | **display-only** | M9 ASCII/OBJ proof + golden re-bless |
| 6 | **Sweep miter at path corners** — BL-08 v1 is straight-segment (sharp corners self-intersect) | **no UI** (internal mesh) | — |
| 7 | **Per-axis / non-linear taper** — BL-09 taper is a uniform centroid scale | **schema param** (the ONE real surface) | M9 Steps field editor |
| 8 | **Straight-skeleton inset for non-convex loops** — BL-10 v1 is edge-normal + a conservative collapse guard | **no UI** (internal mesh; guard already surfaces) | — |

**Match against the source: 8/8, exact.** The closeout's "Deferred polish" list is the
canonical 8 and these are it verbatim. The closeout's separate **"edi-ui dependencies"**
section (canvas profile/path pickers, the File-menu Append / TOON-export entries, the
`--recipe-toon` CLI) is **batch-1 chrome already surfaced in `BL-surfaces.md`** — NOT
batch-2, not re-surfaced here (per the correction).

---

## Ratified decisions (hub, 2026-06-17 — froze the 2 forks)

1. **Item #7 taper-param vocabulary = `taperStart` + per-axis `taperX` / `taperY`
   (RATIFIED).** Four bindable Number schema params on the `AddPrism` carrier (with the
   existing `taperEnd`): `taperStart` (non-linear along-length) and `taperX` / `taperY`
   (non-uniform cross-section). **All default 1.0 (identity)** so a recipe without them is
   byte-preserving and the BL-09 golden stays unchanged. Threaded exactly like BL-09's
   `taperEnd` (writer/reader/validate/schema both languages + the carrier mesh); ≤0 refused
   at validate.
2. **Item #5 boolean golden = co-bless with edi-ui (RATIFIED).** Suppressing the consumed
   operand's standalone emission changes the boolean ASCII/OBJ golden; the builder
   re-blesses those goldens in the same change (the established proof-golden co-bless
   pattern). No control — a test-artifact dependency, recorded.

The other six items carry no UX decision (mesh-quality or smoke-assert refinements with no
control to place). **NEW infra across all 8: NONE.**

---

## 1 · Watertight helix (end caps + closure)

1. **Mechanism** — **no UI; internal mesh correctness.** BL-07's helix is an open ribbon
   in v1; the fix adds end caps + seam closure in the Python loft.
2. **Interaction** — None. Surfaces only as a *better OBJ mesh / Render-tab geometry* (a
   closed, watertight spiral) and a green manifold smoke assert (item #3). No control, no
   new field — the same `screwRise`/`screwTurns` author the helix; this only improves how
   it is built.
3. **Reuses** — `_moulding_world` (the Python ring loft, `tools/blender/edi_craft.py`); the
   existing screw params from BL-07. No widget.
4. **NEW infra?** No — **no surface at all.**

## 2 · `sweep_degrees` × `screw_rise` silent-override validate-WARNING

1. **Mechanism** — M9, **display-only via the Compiled / Render error text.** A new
   validate **Warning**, no control.
2. **Interaction** — When a lathe op sets both a partial `sweepDegrees` (<360) and a
   `screwRise` (helix), v1 silently ignores the partial sweep. The fix emits a named
   validate Warning so the user *sees* the override in the **Compiled** tab
   (`renderCompiledRecipeText`) and the Render error — exactly like the existing
   `negative_*_base_z` / `moulding_profile_not_monotonic` warnings. The user reads it; the
   build still proceeds (Warning, not Error).
3. **Reuses** — the validate finding idiom `add(findings, Severity::Warning, "<name>", …)`
   (`RecipeOpsValidate.cpp` — e.g. `negative_prism_base_z` at :180); the Compiled-tab
   warning surfacing (`buildCompiledRecipePanel`). No widget.
4. **NEW infra?** No — **no control**; a named warning on an existing surface.

## 3 · Edge-incidence manifold assert in the smoke

1. **Mechanism** — **no UI; smoke-test assert.** Locks watertightness in the cross-language
   smoke.
2. **Interaction** — None. The assert (every edge shared by exactly two faces) runs in
   `tests/edi_craft_smoke.py`; its only user-visible effect is a green/red gate, not a
   panel. It guards items #1/#6 from regressing.
3. **Reuses** — `tests/edi_craft_smoke.py` + the existing `--obj-out` OBJ proof it parses.
   No widget.
4. **NEW infra?** No — **no surface.**

## 4 · `bounds_of` arc/helix/sweep tightness

1. **Mechanism** — **no UI; internal mesh framing.** Tightens the Python OBJ bounds for
   arc/helix/sweep shapes (today a loose-but-correct ±max_radius box).
2. **Interaction** — None. Surfaces only as tighter framing of those shapes in the OBJ /
   Render output (the ASCII Proof framing is the *separate* C++ `BoundsEstimator`, untouched
   here). No control, no field.
3. **Reuses** — `bounds_of` (`tools/blender/edi_craft.py`). No widget.
4. **NEW infra?** No — **no surface.**

## 5 · Boolean proof-duplication (suppress the consumed standalone operand)

1. **Mechanism** — M9, **display-only.** Changes what the ASCII Proof / OBJ proof *shows*
   (one fewer standalone operand); no control.
2. **Interaction** — None to author. BL-11 today emits an operand both standalone AND tagged
   under the boolean; the fix suppresses the consumed operand's standalone emission so the
   proof mirrors what bpy actually keeps. The user simply sees a cleaner **ASCII Proof** /
   OBJ proof for a boolean step. **Requires a boolean-golden re-bless co-blessed with
   edi-ui** (ratified Decision 2).
3. **Reuses** — `renderOpsProjection` / `buildAsciiPreviewPanel` (ASCII); the
   `obj_objects` boolean proof emission (Python); the existing proof-golden co-bless
   pattern. No widget.
4. **NEW infra?** No — **no control**; proof-tab appearance + golden update.

## 6 · Sweep miter at path corners

1. **Mechanism** — **no UI; internal mesh correctness.** BL-08 v1 sweeps straight segments
   (sharp corners self-intersect); the fix miters the profile at path corners.
2. **Interaction** — None. Surfaces only as a *better OBJ mesh / Render geometry* (clean
   corners) for an existing `AddSweepProfile` op — same `profile`/`path` references author
   it; this only improves the loft. No control, no field.
3. **Reuses** — the sweep lowering + the Python sweep mesh helper
   (`tools/blender/edi_craft.py`). No widget.
4. **NEW infra?** No — **no surface.**

## 7 · Per-axis / non-linear taper  *(the one real authoring surface)*

1. **Mechanism** — M9, **Steps field-editor schema params** on the prism/sweep carrier
   (`AddPrism` — the carrier BL-09's `taperEnd` already rides). `RecipeOpSchema` extras.
2. **Interaction** — Click the prism/sweep step → the field editor shows the new taper
   spins beside the existing `taperEnd` (ratified Decision 1): **`taperStart`** (Number,
   default 1.0) for non-linear along-length, and per-axis **`taperX` / `taperY`** (Number,
   default 1.0) for non-uniform cross-section. Each is a bindable Number — right-click to
   bind to a drafted measurement. All default to identity (1.0) so a recipe without them is
   **byte-preserving** (the closeout's invariant); out-of-range (≤0) is refused at validate
   and shows in the Compiled/Render error.
3. **Reuses** — `RecipeOpSchema` `appendExtras`/`setExtra` (drives `opEditableScalars` →
   the `opField_<key>` Number spins in `rebuildFields`, `EdiShellWindowIo.cpp:1315`); the
   Number-field bind affordance `showOpBindMenu` (:1368); the **BL-09 `taperEnd`
   field-add as the exact template** (default-identity numeric param threaded
   writer/reader/validate/schema both languages + the carrier mesh).
4. **NEW infra?** No — identical pattern to BL-09's `taperEnd`; the Steps field editor
   already renders bindable Number params.

## 8 · Straight-skeleton inset for non-convex loops

1. **Mechanism** — **no UI; internal mesh correctness.** BL-10's `inset` is edge-normal +
   a conservative collapse guard in v1; the fix is a proper straight-skeleton inset for
   non-convex loops.
2. **Interaction** — None new to author. The same `inset` Number field (BL-10, already in
   the Steps field editor) drives it; this only improves the geometry it produces, letting
   larger insets on non-convex profiles succeed instead of hitting the conservative guard.
   The existing oversized-inset refusal (the collapse guard) stays as a named refusal in
   the Compiled/Render error. No control, no new field.
3. **Reuses** — the `inset` schema param from BL-10; the prism mesh builder
   (`tools/blender/edi_craft.py`); the existing collapse-guard validate refusal. No widget.
4. **NEW infra?** No — **no surface** (the control already exists from BL-10).

---

## Summary

| # | item | verdict | NEW infra |
|---|------|---------|-----------|
| 1 | Watertight helix | no UI (internal mesh) | no |
| 2 | sweep_degrees × screw_rise warning | display-only (Compiled/Render warning) | no |
| 3 | manifold smoke assert | no UI (smoke assert) | no |
| 4 | bounds_of tightness | no UI (internal mesh) | no |
| 5 | boolean proof-duplication | display-only (proof + golden re-bless, co-bless edi-ui) | no |
| 6 | sweep miter | no UI (internal mesh) | no |
| 7 | per-axis / non-linear taper | **schema param (Steps field editor)** | no |
| 8 | straight-skeleton inset | no UI (control already exists from BL-10) | no |

**Surfaced: 8/8** (canonical closeout list, exact match). **Verdict spread:** 5 no-UI
(#1/#3/#4/#6/#8), 2 display-only (#2/#5), 1 schema-param (#7). **NEW-infra needs: NONE** —
#7 reuses the BL-09 `taperEnd` field-add pattern; everything else is mesh-quality,
smoke-assert, or a named warning on an existing surface. **Ratified decisions (hub
2026-06-17):** item-7 taper set = `taperStart` + `taperX`/`taperY` (default 1.0); item-5
boolean golden co-blessed with edi-ui. The earlier brief-005 candidate list
(Solidify/Bisect/3D-Array/3D-Mirror) is dropped as never-in-scope.
</content>
