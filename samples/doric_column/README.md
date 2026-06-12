# Doric column — the no-guesswork pipeline, end to end

A 9.6-inch column maquette on edi's default 12×12 inch board, built
through ONE pipeline: **"Recipe is truth. ASCII preview is proof.
Blender script is execution."** The drafting canvas is the measurement
authority; the op stream is the pointable document; the craftsmen
library executes.

Two column documents live here, deliberately:

- **The drafted column** (the R1 acceptance benchmark — pipeline A's
  column, rebuilt on the surviving vocabulary):
  - `doric_column_profiles.edidraw` — the drafted measurement authority.
    Three polyline profiles (`base_cove`, `shaft`, `echinus`) with exact
    coordinates; every vertex grid-true and numerically editable.
  - `doric_column_drafted_ops.toml` — the SOURCE op stream: boxes by
    explicit size, three `AddRevolvedProfile` REFERENCES into the
    drafted document, and `CutFlutes` with the explicit cutter
    (r 0.16 riding at 1.056, depth 0.12 — the drafter's own numbers,
    inexpressible by ratio derivation).
  - `doric_column_drafted_resolved.toml` — the same stream after
    `resolveRecipeOps` against the live drawing: the lathe references
    lowered to mouldings carrying the DRAFTED radii and heights
    (base cove r 1.32→1.056, shaft r 1.056→0.792 with the entasis
    drafted in, echinus r 0.792→1.32).
- **The ported prototype column** (the port-fidelity artifact —
  ascii_blender_dryrun_v0's own example, translated key for key):
  `doric_column_ops.toml`, `doric_column_ops_compiled.toml`,
  `previews/doric_{front,side,top}_preview.txt` (byte-identical to the
  prototype's own output), `doric_dry_run.txt`.

## The loop

1. Open `doric_column_profiles.edidraw` in edi.
2. File → Open Ops Recipe… → `doric_column_drafted_ops.toml`.
3. File → Export Resolved… — resolves every profile reference and
   measurement binding against the LIVE drawing and grid; a stale
   reference refuses with every `op.<i>.<field>` address listed.
4. File → Export Ops Previews… — front/side/top ASCII proof.
5. Blender (≥ 4.1):
   `blender --python tools/blender/edi_craft.py -- <resolved-or-compiled>.toml`

Headless, the same loop is `edi_recipe_ops` (the directory must exist —
the CLI refuses rather than invents paths; and it resolves against the
DEFAULT 12x12 grid, which is exactly the board this column was drafted
on — a drawing on a custom grid needs the shell verbs until the grid
rides in the `.edidraw`):

    mkdir -p out
    edi_recipe_ops samples/doric_column/doric_column_drafted_ops.toml \
        --resolve samples/doric_column/doric_column_profiles.edidraw \
        --previews out --compiled-out out/compiled.toml

To change the column, point at the exact thing: "the flutes should be
24" → `op.4.count = "24"`. "The shaft neck is too thin" → edit the
`shaft` polyline's last vertex in edi and re-resolve — the recipe file
does not change at all; the profile is a REFERENCE, and the drafting
document stays the single source of truth.

## Conventions (stated, never inferred)

- The page's **left edge is the spin axis**: drafted x is the radius,
  scaled by the grid's width like every radius measurement in edi.
- The page **bottom is z = 0**: drafted height stands the part up
  (z = (1 − y) × grid height).
- Units are the grid's units (inches on the default board); 1 Blender
  unit = 1 grid unit.
- A drafted profile drawn top-down lofts identically to one drawn
  bottom-up (direction-normalized when strictly falling); a FOLDED
  profile fails validation by name — a stacked-ring loft cannot
  represent an overhang.

## Anatomy (op → part, the drafted column)

| op | type | part | the numbers |
|----|------|------|-------------|
| 0  | AddBox | plinth | 3 × 3 × 0.48 at base 0 |
| 1  | AddBox | plinth step | 2.64 × 2.64 × 0.36 at base 0.48 |
| 2  | AddRevolvedProfile | base cove | profile `base_cove` (r 1.32 → 1.056, z 0.84 → 1.008) |
| 3  | AddRevolvedProfile | shaft | profile `shaft` (r 1.056 → 0.792, z 1.008 → 8.04, entasis drafted) |
| 4  | CutFlutes | 20 flutes | cutter r 0.16 at radius 1.056, depth 0.12, z 1.2 → 6 |
| 5  | AddRevolvedProfile | echinus | profile `echinus` (r 0.792 → 1.32, z 8.04 → 8.4) |
| 6  | AddBox | abacus step | 2.64 × 2.64 × 0.24 at base 8.4 |
| 7  | AddBox | abacus | 3.36 × 3.36 × 0.96 at base 8.64 |

## Known limitations (decisions, not surprises)

- Flute cutters are straight cylinders; on a tapered shaft the bite
  shallows toward the narrow end. A tapered cutter is a future op
  parameter, not a different architecture.
- **Documented losses from pipeline A's retirement (R1-B06, standing
  decision 6):** the `bevel` shaper (polish is hardcoded per craftsman
  in `edi_craft.py` — a recipe-controlled finish op is a later design)
  and the `array` shaper (3D placement repetition belongs to the
  R6 jobs/composition model). Neither was used by this column; both are
  losses by decision, recorded here so they cannot be losses by
  amnesia.

## Regenerating the samples

Every artifact is byte-guarded by a test that IS its generator recipe:

- Drafted set: `tests/recipe_drafted_column_tests.cpp` builds the source
  stream from `tests/recipe_drafted_fixture.h`, resolves it against the
  REAL `.edidraw` (default grid), and byte-compares both committed
  TOMLs. The resolved numbers are pinned against a probe of pipeline
  A's resolver taken immediately before its retirement.
- Ported set: `tests/recipe_ops_tests.cpp` (ops + compiled TOML) and
  `tests/recipe_ops_ascii_tests.cpp` (previews) byte-compare against
  `tests/recipe_doric_fixture.h`; `doric_dry_run.txt` is asserted by
  `tests/edi_craft_smoke.py` and regenerates via
  `python3 tools/blender/edi_craft.py --dry-run
  samples/doric_column/doric_column_ops_compiled.toml`.
