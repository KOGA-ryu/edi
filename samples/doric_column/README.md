# Doric column — the no-guesswork pipeline, end to end

A 9.6-inch column maquette on edi's default 12×12 inch board. Three files,
one loop:

- `doric_column_profiles.edidraw` — the drafted measurement authority.
  Three polyline profiles (`base_cove`, `shaft`, `echinus`) with exact
  coordinates. Open it in edi; every vertex is on the grid and numerically
  editable through the geometry inspector.
- `doric_column.toml` — the recipe: shapers in order, every parameter an
  explicit key. This is the pointable document.
- `doric_column.py` — the emitted Blender script. Generated, never edited:
  every number in it is either typed in the recipe or measured from the
  drafted profiles.

## The loop

1. Open `doric_column_profiles.edidraw` in edi.
2. File → Open Recipe… → `doric_column.toml`.
3. File → Export Blender Python… → `doric_column.py`.
4. Blender: run the script (Scripting workspace → Open → Run, or
   `blender --python doric_column.py`).

To change the column, point at the exact thing:

- "The flutes should be 24, not 20" → `step.4.param.count.value = "24"`,
  re-export.
- "The shaft neck is too thin" → select the `shaft` polyline in edi, edit
  its last vertex's x from 0.066 to 0.070 (r 0.792 → 0.84 inches),
  re-export. The recipe file does not change at all — the profile is a
  *reference*, and the drafting document stays the single source of truth.

## Conventions (stated, never inferred)

- The page's **left edge is the spin axis**: drafted x is the radius,
  scaled by the grid's width like every radius measurement in edi.
- The page **bottom is z = 0**: drafted height stands the part up
  (z = (1 − y) × grid height).
- Units are the grid's units (inches on the default board). The script
  emits those numbers verbatim; 1 Blender unit = 1 grid unit.

## Anatomy (step → part)

| step | shaper        | part         | the numbers |
|------|---------------|--------------|-------------|
| 0    | cube          | plinth       | 3 × 3 × 0.48 at base 0 |
| 1    | cube          | plinth step  | 2.64 × 2.64 × 0.36 at base 0.48 |
| 2    | lathe         | base cove    | profile `base_cove` (r 1.32 → 1.056, z 0.84 → 1.008) |
| 3    | lathe         | shaft        | profile `shaft` (r 1.056 → 0.792, z 1.008 → 8.04, entasis) |
| 4    | radial_groove | 20 flutes    | cutter r 0.16, depth 0.12 at r 1.056, z 1.2 → 6 |
| 5    | lathe         | echinus      | profile `echinus` (r 0.792 → 1.32, z 8.04 → 8.4) |
| 6    | cube          | abacus step  | 2.64 × 2.64 × 0.24 at base 8.4 |
| 7    | cube          | abacus       | 3.36 × 3.36 × 0.96 at base 8.64 |

## The op-stream pipeline (the ported prototype tools)

Alongside the shaper-grammar files above, this directory carries the same
column in the PORTED prototype pipeline — "Recipe is truth. ASCII preview
is proof. Blender script is execution.":

- `doric_column_ops.toml` — the source op stream (the prototype's own
  example recipe, translated key for key into strict TOML).
- `doric_column_ops_compiled.toml` — after the compile pass: moulding term
  sequences expanded into exact (z, radius) points.
- `previews/doric_{front,side,top}_preview.txt` — the ASCII proof,
  byte-identical to the prototype's own generated previews.
- `doric_dry_run.txt` — the craftsmen library's build plan for this
  column, one line per op, plus the computed preview rig.

To build it in Blender (>= 4.1):

    blender --python tools/blender/edi_craft.py -- samples/doric_column/doric_column_ops_compiled.toml

To inspect the plan without Blender:

    python3 tools/blender/edi_craft.py --dry-run samples/doric_column/doric_column_ops_compiled.toml

The library (`tools/blender/edi_craft.py`) is the durable half — the
craftsmen. The recipe TOML is the dimension sheet. Change the recipe,
re-run; the library does not change.

This pipeline has no edi menu verbs yet; it runs via the commands above.
File → Open Recipe… belongs to the shaper-grammar pipeline at the top of
this README — pointed at an ops TOML it will refuse with an unknown-key
error (by design: two strict vocabularies, no cross-parsing).

The op-stream artifacts are asserted in-repo: `doric_column_ops.toml`, the
compiled TOML, and the previews are byte-compared against the construction
in `tests/recipe_doric_fixture.h` by `recipe_ops_tests` and
`recipe_ops_ascii_tests`; `doric_dry_run.txt` is asserted by
`tests/edi_craft_smoke.py` and regenerates by redirecting the `--dry-run`
command above into the file
(`… --dry-run … > samples/doric_column/doric_dry_run.txt`).

## Known limitations (V1, stated so they are decisions, not surprises)

- Flute cutters are straight cylinders; on a tapered shaft the bite
  shallows toward the narrow end (here: 0.12 at the foot easing to ~0.024
  at z 6). Real Doric flutes follow the taper — a tapered cutter is a
  future shaper parameter, not a different architecture.
- Lathe meshes are open surfaces (profiles are not auto-closed to the
  axis); the SCREW modifier merges the seam but caps are the renderer's
  problem for now.
- Regenerating the sample: the construction lives in
  `tests/recipe_column_tests.cpp` (asserted) — the committed files were
  generated from the identical step list.
