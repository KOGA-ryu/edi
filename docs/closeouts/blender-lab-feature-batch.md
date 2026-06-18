# Closeout — blender-lab-20260617-feature-batch

> Freezes the recipe-lab feature batch so future work does not re-litigate it.
> Campaign: **build the canvas-geometry→bpy depth seam + the recipe-lab feature
> bucket** (15 tasks BL-01..15). Complete 2026-06-17 on `dept/blender-lab` (NOT
> merged — edi-ui owns integration/merge). Handoff:
> `docs/handoffs/blender-lab-20260617-feature-batch.md`. Architecture map:
> `docs/architecture/edi-blender-lab.md` (re-trued at this close).

## What shipped (all 15, each verified; commit hashes are pre-rebase ids)

| Task | What | Commit |
|---|---|---|
| BL-01 | `AddExtrudedProfile` op arm (→11), refused-before-build | `cd646ab` |
| BL-03 | `AddPrism` carrier + extrude→prism lowering (→12) | `e166709` |
| BL-04 | Python prism build + OBJ golden — **spine complete** | `914a473` |
| BL-05 | Push/pull authoring proved (height bindable, neg=cut) | `7d0a73d` |
| BL-06 | Lathe `sweepDegrees` (partial revolve, watertight) | `6a62c8f` |
| BL-07 | Lathe `screwRise`/`screwTurns` (helix) | `2e251c7` |
| BL-12 | `radial_petal` craftsman (pure Python) | `0a2ab5b` |
| BL-13 | `nfold_star` {n/k} craftsman (coprime-guarded) | `789f7bc` |
| BL-14 | Named-recipe library + `appendRecipe` chaining | `834daed` |
| BL-08 | Follow-Me sweep arm (→13) + generalized prism `path` | `2b54a9c` |
| BL-09 | Taper-along-sweep (`taperEnd`, centroid scale) + BL-08 fold-in | `159d77b` |
| BL-11 | Solid boolean op (→14) + `remapRecipeOpNameRefs` hardening | `12df814` |
| BL-10 | `inset`/`normalOffset` depth verbs on the prism | `46f0715` |
| BL-15 | `exportRecipeStreamToToon` (resolved-stream TOON handoff) | `9283d37` |

(BL-02 was COLLAPSED per the ratified fork — extrude is OBJ-only proof — and its slot
spent on the carrier generalization / depth verbs.)

## Frozen design decisions (do not re-derive)

- **The depth seam is closed end-to-end:** a drafted closed figure →
  `AddExtrudedProfile` → (resolve) `AddPrism` → (key-for-key store/`parse_ops`) OBJ +
  bpy build. The lathe + extrude + sweep are the three canvas→bpy bridges.
- **`AddPrism` is THE generalized buildable carrier** (Option A, ratified): one arm
  carries straight extrude (empty `path`) AND swept solid (`path` present), plus
  `taperEnd`, `inset`, `normalOffset`. Adding a depth verb = a field on AddPrism, not a
  new arm. **Every such field defaults to a byte-preserving identity.**
- **Refused-before-build ref-ops** (`AddRevolvedProfile`/`AddExtrudedProfile`/
  `AddSweepProfile`) lower to a buildable carrier and are refused by name on BOTH C++
  and Python — only the lowered carrier reaches a build.
- **Lathe params survive lowering:** `sweepDegrees`/`screwRise`/`screwTurns` live on
  BOTH `AddRevolvedProfile` AND `AddMoulding`, copied at lowering. Helix is a sweep-time
  vertex-z lift (NOT a profile mutation — cannot trip `moulding_profile_not_monotonic`).
- **`AddBoolean` targets a/b BY NAME** (like CutFlutes); proof emits the operands tagged;
  the real CSG is the bpy boolean modifier (execution-only). `a==b` refused.
- **Chaining (`appendRecipe`)** is a pure data merge: binding `opIndex += offset` +
  namespace-all spliced op names + remap their name-references. `remapRecipeOpNameRefs`
  is now an **exhaustive `std::visit`** (`NameRefRemapper`) — a future name-ref arm MUST
  declare its remap or fail to compile.
- **TOON handoff** (`exportRecipeStreamToToon`) reuses the `op.N.<field>` TOML key scheme
  (one shared `recipeOpsToConfig`), refuses an unresolved stream by name, never JSON.
- **The variant is stable at 14 arms** (`static_assert == 14`). No more arm-adders were
  planned for this batch.

## Discipline held throughout

- **Every pre-existing golden stayed byte-identical** — doric (13148-line OBJ) never
  moved; the extruded_figure/swept_profile/boolean_op samples are new. New behavior is
  opt-in, default-off; every default is the identity path.
- **Every arm-add is compiler-exhaustive** across all interpreter sites; goldens
  regenerated additively (only the new key added, writer-sorted position), reviewer- or
  planner-verified.
- **No JSON, no `.js`/`.qml`/QtQml/QtQuick**; formats stayed TOML (recipes) / MessagePack
  (docs, untouched here) / TOON (AI handoff). No `CMakeLists.txt`/`LEDGER.md`/shell edits
  (edi-ui's) — the whole batch fit in existing `src/recipe` + `tools/blender` files.
- **Gates:** the four arm-adds + the lathe params + the chaining + the sweep got
  adversarial reviewer audits (incl. independent manifold checks, hand-traced binding
  arithmetic, and the no-cross-reference crux); the proven-pattern field-adds + the
  pure-Python craftsmen were accepted on the green gate + planner spot-checks. Final
  state: build clean, `ctest -E edi_shell_window_tests` **101/101**, scan clean, the four
  cross-language `--obj-out` goldens byte-identical, `edi_craft_smoke` green.

## Deferred polish (tracked — a future campaign, NOT blocking)

1. Watertight helix (end caps + closure) — BL-07's v1 helix is an open ribbon.
2. `sweep_degrees` × `screw_rise` silent-override validate-WARNING — BL-07 helix ignores
   a partial sweep in v1.
3. Edge-incidence manifold assert in the smoke (lock watertightness) — BL-06 note.
4. `bounds_of` arc/helix/sweep tightness — currently loose-but-correct (±max_radius).
5. Boolean proof-duplication — BL-11 emits operands both standalone AND tagged; suppress
   the consumed operand's standalone emission to mirror bpy (changes the boolean golden).
6. Sweep miter at path corners — BL-08 v1 is straight-segment (sharp corners self-intersect).
7. Per-axis / non-linear taper — BL-09 taper is a uniform centroid scale.
8. Straight-skeleton inset for non-convex loops — BL-10 v1 is edge-normal + a conservative
   collapse guard.

## edi-ui dependencies (chrome — NOT this department's; flagged per task)

The OP/controller verbs ship correct-but-un-clickable; edi-ui wires the chrome from
`docs/ui-surface/blender-lab/BL-surfaces.md`: the profile/path/operand **canvas-pickers**
(refs are TOML-typed today), the File-menu **"Append Ops Recipe…"** (BL-14) + **"Export
Recipe (TOON)…"** (BL-15) entries, and the headless **`--recipe-toon`** CLI verb in
`app/main.cpp`. New depth params surface automatically in the Steps field editor (generic
opField + bind affordance) — no chrome needed for those.

## Not re-opened

No generation/auto-layout (mandate stop-line). The two craftsmen are parametric TOOLS,
not randomizers. CSG is execution-only (the proof never computes it). Seam B
(asset→dungeon) remains downstream.
