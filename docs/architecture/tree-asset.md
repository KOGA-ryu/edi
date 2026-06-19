# Procedural TREE asset — technique pick + critique rubric + detail ladder

Campaign: the zoo's **first ORGANIC asset** (terrain/environment pillar → forests).
A tree cannot be expressed by the existing op vocabulary (`AddPrism`,
`AddCylinder`, `AddBoolean`, `nfold_star`, `twisted_column`, `radial_petal` are
all closed-form, hand-dialled tools — none branches). The tree is therefore a
**NEW craftsman** that rides the proven loop: forge → bake `.blend` → curate
AssetRecord → TOON manifest → INSTANCE → render. The payoff is **build-once /
place-many with SEED variation** — a forest is many instances of a few tree
assets, each instance a different `seed`.

Scope of this doc: research + decision only (no code). It picks ONE technique,
fixes the MANIFEST param set, writes the pipe-model + branching rules the
generator will apply, sharpens the 6-dimension critique rubric into checkable
tests with reference numbers, and lays out the L0→L5 detail ladder. The builder
implements; the reviewer scores against the rubric.

Facts are tagged **[verified-from-source]** (read in the cited reference or the
repo) vs **[asserted]** (derived/conventional defaults the reviewer can tune).

---

## The deciding constraint — the craftsman contract

Read `tools/blender/craftsmen/nfold_star.py` and `twisted_column.py`. A craftsman
is a `Script` op with three parts, and the contract is unforgiving:

1. **`MANIFEST`** = id, label, params. **Param types are limited to
   `number` / `integer` / `material`** — the C++ `ScriptOp` carries only these.
   So *every* tree parameter (height, trunk radius, branch angle, depth, leaf
   density, SEED …) must be expressible as a number or integer. No strings, no
   curves, no enums-as-text, no nested tables. **[verified-from-source]**
   (`nfold_star.py` MANIFEST; charter `docs/departments/edi-blender-lab.md`
   "param keys are flat bare keys").

2. **`proof_mesh(op) -> (verts, faces)`** — **PURE Python, NO bpy.** This is the
   OBJ/ASCII proof tier the reviewer and ctest exercise offline.
   **[verified-from-source]** (both craftsmen import `math` only in the proof
   path; `import bpy` is inside `build`).

3. **`build(op)`** — the bpy twin. In `nfold_star` it LITERALLY calls
   `proof_mesh(op)` then `mesh.from_pydata(verts, [], faces)`. So **the entire
   mesh (all verts + faces) is generated in pure Python; bpy only instantiates
   it.** **[verified-from-source]** (`nfold_star.build` lines 95-105).

**Consequence — the technique must be deterministic pure-Python `(verts, faces)`
generation, seeded, with both tiers running the SAME generator.** This rules out
two otherwise-obvious tools, verified for THIS headless environment:

- **`sapling_tree_gen` add-on — OUT.** Not available in this headless Blender,
  and it is a bpy add-on with no pure-Python proof twin anyway. **[verified —
  environment fact stated in the order]**
- **Geometry Nodes — OUT.** A geo-nodes tree exists only as a node graph
  evaluated by Blender's dependency graph; it cannot produce a `proof_mesh`
  without bpy, so it breaks the dual-tier contract. **[asserted]**

That leaves **self-implemented algorithms** that produce a **SKELETON** (segments
= nodes with position + radius + parent), which we then **SKIN** to
tapered-cylinder tubes + canopy clumps as `(verts, faces)`. The three candidates
are evaluated below.

---

## 1. Chosen technique — RECURSIVE PARAMETRIC BRANCHING (seeded)

**Pick: recursive self-similar branching with stochastic jitter** — a depth-first
recursion that, at each node, spawns `childrenPerNode` child segments rotated by a
down-angle (elevation) and a divergence (azimuth) angle, with length and radius
decayed per level by fixed ratios, and a seeded RNG perturbing every angle/length
within bounds. The pipe model (below) sets each child's radius from its parent.

This is the **Weber-Penn lineage** [verified-from-source: Weber & Penn,
"Creation and Rendering of Realistic Trees", SIGGRAPH 1995, parameters nLevels,
nDownAngle, nRotate, nLength, nTaper, ratio] reduced to what a pure-Python
generator with only number/integer params can carry.

### Why it wins

- **(a) Pure-python dual-tier feasibility — best.** The skeleton is a closed
  recursion over plain numbers; no spatial data structures, no point clouds, no
  bpy. The same function runs in `proof_mesh` and (via `build`) in bpy, exactly
  like `nfold_star._local_mesh`. **[asserted, grounded in the contract]**
- **(b) Seeded reproducibility — trivially exact.** A single
  `random.Random(seed)` threaded through the recursion makes the whole tree a
  pure function of `(params, seed)`. Same seed → byte-identical `(verts, faces)`
  in both tiers. (See Risks for the determinism discipline.) **[asserted]**
- **(c) Direct parametrization of the rubric — strongest.** Every rubric
  dimension maps to ONE param: taper → `taper`; hierarchical branching →
  `branchLevels` + `childrenPerNode`; pipe-model thinning → `pipeExponent`;
  asymmetry → `seed` + `*Jitter`; canopy → `clumpCount`/`leafSize`. The author
  *dials* the silhouette. **[asserted]**
- **(d) Poly-budget sanity — controllable + cheap.** Poly count is a closed
  formula of `childrenPerNode^branchLevels × tubeSides` plus clump icospheres
  (see §6 budget). No runaway point-cloud growth. **[asserted]**

### Why the others lose (runner-up ideas grafted in)

- **L-systems (stochastic / parametric)** — Lindenmayer rewriting of a string,
  then a turtle interprets it into a skeleton. **Pure-python feasible and
  seedable**, BUT: the *productions* (rules like `F → F[+F]F[-F]F`) are the
  expressive surface, and **a production rule is a STRING — unrepresentable in our
  number/integer/material MANIFEST.** You would either hardcode one fixed grammar
  (then `branchLevels` etc. are just the recursion budget — at which point it IS
  recursive branching with extra indirection) or smuggle a grammar through a
  numeric encoding (opaque, un-dialable, fails "named params"). **Lost on the
  param-type constraint.** **[verified-from-source: the order's MANIFEST type
  limit + L-system rule-as-string fact]**
  *Grafted idea:* L-systems' **parametric, context-free self-similarity** IS our
  recursion; and their **stochastic productions** (pick rule by seeded
  probability) port directly as the per-node seeded jitter we adopt.

- **Space colonization (Runions et al. 2007)** — scatter attraction points in an
  envelope; each iteration, associate each attractor to its nearest tree node
  within radius-of-influence `d_i`, average the normalized pull directions, grow a
  new node by segment length `D`, kill attractors within `d_k`. Produces the most
  natural, competition-shaped crowns. **[verified-from-source: Runions, Lane,
  Prusinkiewicz, "Modeling Trees with a Space Colonization Algorithm", EG NPH
  2007 — algorithm steps, `d_i`/`d_k`/`D` parameters; `d_k` expressed as a
  multiple of `D`.]** BUT for our craftsman it loses on three counts:
  - **Params don't map to dials.** The author wants `height`, `branchAngle`,
    `taper`. Space colonization is governed by `d_i`, `d_k`, point count, and the
    envelope SHAPE — and the envelope is the real silhouette control, which is a
    geometric region, not a number. Hard to expose as named, meaningful dials.
  - **Cost + topology.** It needs a nearest-neighbour query over an attractor
    cloud each iteration (O(points × nodes) naive); pure-Python without spatial
    structures is slow at forest scale, and the skeleton has **irregular valence
    joins** that are harder to skin cleanly than a fixed-fanout recursion.
  - **Determinism is fiddlier** — the result depends on attractor sample order
    and tie-breaking, more surface for the two tiers to diverge.
  **Lost on dial-ability + pure-python cost + topology.** **[asserted, grounded
  in the cited algorithm]**
  *Grafted idea:* keep an optional **`crownRadius` envelope** param so the
  recursion's children are *clipped/biased toward* an ellipsoidal crown region —
  borrowing space colonization's "grow into a volume" silhouette control without
  its cloud machinery. (L4+ nicety, not L0.)

**Decision in the lab's terms:** recursive parametric branching is the only
candidate where *recipe is truth* holds literally — the silhouette is a
closed-form function of named params + seed, the proof tier is the execution
tier, and `nfold_star` is the structural template.

---

## 2. The craftsman param set (the MANIFEST)

All types are `number` / `integer` / `material` only. No hardcoded dimensions —
every dimension is a named param. Defaults below produce a generic upright
broadleaf tree (~6 m, the scale-reference figure is 6 ft, so the tree reads as a
mature tree against it).

| key | type | default | meaning / rubric dimension |
|---|---|---|---|
| `seed` | integer | `0` | RNG seed; drives ALL jitter → ASYMMETRY/VARIATION. Same seed = identical mesh. |
| `height` | number | `6.0` | total tree height (trunk base → crown top), metres → PROPORTION/SILHOUETTE |
| `trunkRadius` | number | `0.18` | radius at trunk base (the pipe-model root) → PROPORTION, BASE |
| `taper` | number | `0.85` | fraction of radius retained along a single segment's length (tip/base) → PROPORTION (trunk taper profile) |
| `branchLevels` | integer | `4` | recursion depth (0 = trunk only) → BRANCHING (hierarchy) |
| `childrenPerNode` | integer | `3` | child branches spawned per branch tip → BRANCHING fanout |
| `firstBranchHeight` | number | `0.35` | fraction of height with bare trunk before first branch (clear bole) → PROPORTION, crown ratio gate |
| `downAngle` | number | `45.0` | elevation/down angle (deg) of a child off its parent axis → BRANCHING, SILHOUETTE |
| `downAngleJitter` | number | `12.0` | ± random spread (deg) on `downAngle` → ASYMMETRY |
| `rotateAngle` | number | `137.5` | azimuth divergence between successive children around the parent (golden angle) → BRANCHING phyllotaxis |
| `rotateJitter` | number | `20.0` | ± random spread (deg) on `rotateAngle` → ASYMMETRY |
| `lengthRatio` | number | `0.72` | child length as fraction of parent length per level → SILHOUETTE decay, PROPORTION |
| `lengthJitter` | number | `0.15` | ± fractional random spread on each segment length → ASYMMETRY |
| `pipeExponent` | number | `2.2` | the da Vinci exponent: `r_parent^e = Σ r_child^e` → PROPORTION, BRANCHING thinning |
| `curve` | number | `15.0` | gentle bend (deg) accumulated along a branch's segments (gravity/light lean) → SILHOUETTE, naturalism |
| `segmentsPerBranch` | integer | `4` | nodes along one branch (more = smoother curve, more polys) → ASSET-HEALTH budget |
| `tubeSides` | integer | `6` | radial verts per tube ring → ASSET-HEALTH (poly budget), SILHOUETTE roundness |
| `clumpCount` | integer | `14` | number of foliage clumps placed at outer branch tips → CANOPY mass + gaps |
| `clumpSize` | number | `0.55` | radius of one foliage clump blob → CANOPY |
| `clumpJitter` | number | `0.35` | ± fractional random spread on clump size/position → CANOPY irregularity, ASYMMETRY |
| `leafSubdiv` | integer | `1` | icosphere subdivisions per clump (0=ico, 1=smoother) → ASSET-HEALTH budget |
| `baseFlare` | number | `1.6` | multiplier on radius at the very bottom ring (root flare/buttress) → BASE |
| `barkMat` | material | `"bark"` | trunk/branch material |
| `leafMat` | material | `"leaf"` | canopy clump material |

**Origin convention (matches `nfold_star`/`twisted_column`):** trunk base at the
op's `(x, y, z)`; the generator builds the local mesh with the base ring centred
on local origin at z=0, growing +z, then the proof/`build` wrapper offsets by the
op's x/y/z. **[verified-from-source: `proof_mesh` offset wrapper in both
craftsmen.]**

---

## 3. The pipe-model + branching rules (the generator's laws)

These are the rules the recursion APPLIES; the reference numbers become the
defaults above and the rubric's pass/fail bands.

### 3.1 Pipe model / da Vinci's rule (radius from parent)

**Law (area-preserving / da Vinci):** the parent's cross-section equals the sum of
the children's cross-sections. With cross-section ∝ radius², the exponent is
**e = 2**. Real trees deviate: measured daughter/mother cross-section RATIOS run
**~1.1-1.4 (i.e. slightly more than area-preserving)**, and the literature uses a
**generalized exponent in roughly 1.8-2.5**, with **~2.0-2.3** the natural-looking
band. **[verified-from-source: PLOS One "Tree Branching: Leonardo da Vinci's Rule
versus Biomechanical Models" — Δ=2 is the area-preserving prediction, measured
daughter/mother ratios > 1.0 (~1.1-1.4); arXiv 2411.08024 "Leonardo vindicated"
— A_left²+A_right²=A_parent² form.]**

**Generator formula** — when a parent of radius `r_p` splits into `n =
childrenPerNode` children, each child gets

```
r_child = r_p * (1 / n)^(1 / pipeExponent)
```

(equal-share form of `r_p^e = Σ r_child^e` ⇒ `r_child^e = r_p^e / n`). With the
default `pipeExponent = 2.2` and `n = 3`, `r_child ≈ 0.62 · r_p`. Within a single
branch the radius additionally tapers by `taper` from its start node to its tip
node (the second da Vinci postulate — no taper *between* nodes — is relaxed to a
gentle visual taper for a believable tube). The very bottom ring is scaled by
`baseFlare` for the root buttress.

### 3.2 Branching angles

- **Down/elevation angle:** child axis tilts `downAngle ± downAngleJitter` off the
  parent axis. Real branch angles span widely by species: **~10-50° (Fagus,
  broadleaf)** up to **~50-80° (Abies, conifer)**. Default **45° ± 12°** sits in
  the broadleaf middle. **[verified-from-source: PLOS One paper, field angles
  10-50° / 50-80°.]**
- **Azimuth/divergence:** successive children rotate `rotateAngle ±
  rotateJitter` around the parent axis. Default **137.5° (the golden angle)** —
  the phyllotactic spiral that minimizes child overlap; trees with many nodes
  approach it, opposite-branching species sit nearer 90°. **[verified-from-source:
  Weber & Penn note "any number near 140° works well"; The Grove "golden angle in
  trees" — 137.5° alternate, 90° opposite.]**

### 3.3 Length & level decay

- Each level's branch length = parent length × `lengthRatio ± lengthJitter`
  (default 0.72). Geometric decay → the self-similar fractal silhouette.
  **[verified-from-source: Weber & Penn `nLength` ratio per level; rule-based
  modeling reviews — "branch length ratio controls apical dominance/outline".]**
- **Crown ratio gate:** first branch appears at `firstBranchHeight` (0.35) up the
  trunk, leaving a clear bole; foliage then fills the upper portion. A **live
  crown ratio of ~40-60%** of total height is the healthy-tree target.
  **[verified-from-source: Open Oregon "Live Crown Ratio" — healthy LCR 40-60%.]**

### 3.4 Curve

Each segment within a branch adds `curve / segmentsPerBranch` degrees of bend in a
seeded direction, so branches arc rather than running dead straight — cheap
naturalism. **[asserted, conventional]**

---

## 4. Enriched critique rubric

Six dimensions in **priority order** (the reviewer scores each from 2-3 render
angles — front, 3/4, top — and a silhouette/matte pass). Each dimension is now
CONCRETE and checkable with reference numbers. A failed higher-priority dimension
caps the score regardless of lower ones.

### A. SILHOUETTE (highest)
The black-on-white outline must read unmistakably as a tree.
- **A1.** Single dominant vertical trunk axis from a clear base; not a bush, not a
  starburst. Trunk axis deviation from vertical at the base < ~15°.
- **A2.** Crown is **wider than tall is NOT required**, but crown width is **0.5-1.0×
  the tree height** (broadleaf) — not a thin spike, not a flat disc.
- **A3.** Outline is **irregular and gapped**, not a smooth circle or triangle:
  visible notches/lobes from branch structure showing through foliage.
- **A4.** Recognizable at thumbnail size (silhouette test): a viewer names it
  "tree" in < 2 s.

### B. PROPORTION
Trunk, crown, and radii in believable ratio.
- **B1.** Live crown ratio (foliage height ÷ total height) in **0.40-0.65**.
  **[ref: LCR 40-60%]**
- **B2.** Trunk base radius ÷ height in **~0.02-0.05** (a 6 m tree → trunk base
  radius ~0.12-0.30 m). Not a telephone pole, not a stick.
- **B3.** Trunk visibly **tapers**: top-of-trunk radius ÷ base radius in
  **0.3-0.6** before the first major split.
- **B4.** Base flare present: bottom ring radius ÷ next ring radius ≥ **1.3**.

### C. BRANCHING
Hierarchical, pipe-model-thinned, golden-angle distributed.
- **C1.** At least **3 visible branch levels** (trunk → primary → secondary →
  tertiary/tips). `branchLevels ≥ 3`.
- **C2.** **Radius thinning per split** obeys the pipe model: child radius ÷ parent
  radius in **0.55-0.75** for a 3-way split (matches `(1/n)^(1/e)`, e≈2.0-2.3).
  Children of a node are NOT the same thickness as the parent.
- **C3.** **Primary branch elevation (down) angle 30-55°** off the trunk; not
  horizontal shelves, not vertical brooms. **[ref: 10-50° broadleaf field band.]**
- **C4.** Successive children **spiral** around the parent (azimuth ≈ golden
  angle), not all on one side and not coplanar.

### D. CANOPY
A clumped, gapped foliage mass — NOT a spherical lollipop.
- **D1.** Canopy is **≥ 6 distinct clumps** (default 72), placed at OUTER branch
  tips, with **visible gaps** between them — sky/branches show through.
- **D2.** Bounding-sphere **fill ratio `0.15 ≤ fill < 0.6`** (the reads-as-MASS
  band): the union of clumps fills between 15% and 60% of its bounding sphere
  volume. The lower bound rules out a near-empty crown that reads as decorated
  tips (the OPPOSITE failure from a lollipop); the upper bound keeps it irregular,
  not solid.
- **D3.** Clump sizes **vary** (seeded jitter ≥ ±25%); no two clumps identical.
- **D4.** Foliage sits in the **upper crown region only** (above
  `firstBranchHeight`), not skirting the bare bole.

### E. ASYMMETRY / VARIATION
Seed actually changes the tree; no mirror symmetry.
- **E1.** Two renders with **different `seed`, same other params** are visibly
  different trees (different branch placement, clump layout) — side-by-side
  distinguishable.
- **E2.** Two renders with the **SAME seed** are byte-identical meshes
  (determinism check; reviewer diffs vertex count + a hash).
- **E3.** No global mirror or rotational symmetry in the silhouette; branch
  lengths/angles show natural spread.

### F. ASSET-HEALTH (gate, not aesthetic)
Mesh is clean and instanceable.
- **F1.** **Manifold-ish**: no isolated/duplicate verts beyond join seams; tubes
  closed; caps present. `from_pydata` + `mesh.update(calc_edges=True)` succeeds
  with no errors.
- **F2.** **Poly budget** for one tree at default params **< ~25k tris** (forest
  instancing target). Reviewer reads tri count from the OBJ proof.
- **F3.** **Origin at trunk base** (local z=0), so instances sit on terrain at
  their placement point.
- **F4.** OBJ proof and bpy build produce the **same vertex/face count** (dual-tier
  parity).

---

## 5. Refined DETAIL LADDER (L0 → L5)

Each level switches ON specific params/rules and is GATED by a rubric dimension —
do not advance until the gate passes from the review angles.

| Level | Goal | Params/rules ON | Gate to pass |
|---|---|---|---|
| **L0 — FORM** | A tapered trunk + a placeholder blob crown reads as "tree" in silhouette. | `height`, `trunkRadius`, `taper`, `tubeSides`, `baseFlare`; ONE icosphere crown of `clumpSize`; NO branching yet. | **A (SILHOUETTE) A1+A4** — names "tree" at thumbnail; single vertical trunk. |
| **L1 — ARMATURE** | The seeded recursive skeleton exists (nodes + parent links), drawn as bare tubes. | `seed`, `branchLevels`, `childrenPerNode`, `downAngle`, `rotateAngle`, `lengthRatio`, `segmentsPerBranch`, `firstBranchHeight`; skeleton + tube skin; no jitter, no canopy. | **C (BRANCHING) C1+C4** — ≥3 levels, golden-angle spiral. |
| **L2 — BRANCHING + TAPER** | Pipe-model radii + per-segment taper + gentle curve → believable thickness hierarchy. | `pipeExponent`, refined `taper`, `curve`. | **C2+C3 and B2+B3** — child/parent radius 0.55-0.75; trunk taper 0.3-0.6; down-angle 30-55°. |
| **L3 — CANOPY** | Clumped, gapped foliage at outer tips; lollipop is FORBIDDEN. | `clumpCount`, `clumpSize`, `leafSubdiv`, `leafMat`; clumps at outer branch tips. | **D (CANOPY) D1+D2+D4** — ≥6 clumps, fill 0.15-0.6, upper crown only. |
| **L4 — BARK / BASE** | Root flare + crown-ratio tuning + curve naturalism; materials assigned. | `baseFlare` tuned, `firstBranchHeight` tuned, `barkMat`; (optional grafted `crownRadius` envelope bias). | **B (PROPORTION) B1+B4** — LCR 0.40-0.65, base flare ≥1.3. |
| **L5 — SEED-VARIATION + PAYOFF** | Jitter wired through; a forest of distinct-but-coherent instances; determinism proven. | `downAngleJitter`, `rotateJitter`, `lengthJitter`, `clumpJitter`; full seeded RNG. | **E (ASYM/VAR) E1+E2+E3** AND **F (HEALTH) all** — seeds differ, same-seed identical, budget + origin + dual-tier parity. |

---

## 6. Risks / open questions

- **R1 — Branch-join topology.** Bridging `childrenPerNode` child tubes onto one
  parent tip without non-manifold fans is the hard part. **Mitigation for L1:**
  do NOT weld — emit each branch as its own closed, capped tube and let them
  *overlap/interpenetrate* at the join (the foliage and curve hide it; `nfold_star`
  already ships overlapping caps). Welding into a single manifold skin is an L4+
  nicety, not a gate. The rubric's F1 is "manifold-ish", deliberately lenient on
  join seams. **[asserted; topology fact is the standard tube-tree pitfall.]**

- **R2 — Canopy poly budget.** Per-clump icosphere at `leafSubdiv=1` is 80
  tris; 14 clumps ≈ 1.1k tris. Tubes dominate: a 4-level, 3-fanout tree is
  `1+3+9+27+81 = 121` branches × `segmentsPerBranch` rings × `tubeSides` quads.
  At defaults ≈ 121 × 4 × 6 × 2 ≈ 5.8k tris + caps + clumps ≈ **well under the
  25k F2 budget**. But `childrenPerNode=4, branchLevels=5` explodes to ~1.4k
  branches — the rubric F2 + `segmentsPerBranch`/`tubeSides` are the throttles the
  reviewer must watch. **[asserted, closed-form count.]**

- **R3 — `seed` determinism across both tiers.** The whole asset's value is
  reproducibility, so the generator MUST: (a) use ONE `random.Random(seed)`
  instance threaded through the recursion in a FIXED traversal order (depth-first,
  children in index order); (b) draw RNG values in the SAME order in proof and
  build (trivially satisfied since `build` calls `proof_mesh`); (c) avoid any
  iteration over Python `set`/`dict` whose order could differ, and avoid
  `hash()`-based randomness. Rubric **E2** is the explicit guard. **[asserted;
  this is the core dual-tier contract risk.]**

- **R4 — Float seed coercion.** C++ carries params untyped and the craftsmen
  coerce via `int(float(...))` (see `nfold_star._resolve_k`). `seed` must be read
  as `int(float(params.get("seed", 0)))` so a TOML `0` / `0.0` both work and the
  RNG is stable. **[verified-from-source: coercion pattern in `nfold_star.py`.]**

- **R5 — Crown envelope (grafted space-colonization idea).** If L5 trees still
  look too fractal-regular, add the optional `crownRadius` ellipsoid bias (clip /
  attract tips toward a crown volume) — borrows space colonization's "grow into a
  shape" silhouette control without its point cloud. Defer past L0; only if the
  SILHOUETTE rubric needs it. **[asserted; grounded in Runions et al. 2007.]**

---

## 7. Reviewer-boundary refinements (PINNED — builder + reviewer obey)

Boundary gate verdict: GREEN, L0 may proceed. New tree craftsman is purely
additive (a `Script` op rides the existing generic `ScriptOp` path; no C++ edit,
no core touches the forge). The following are now part of the contract:

**MUST-HONOR (builder):**
1. **`tree.build(op)` calls `tree.proof_mesh(op)` and ONLY instantiates** it
   (`from_pydata`) — the `nfold_star` pattern, NEVER a second/independent
   generator. This is what guarantees dual-tier parity (rubric F4) and same-seed
   determinism (E2). `build` draws no RNG of its own.
2. **Origin = base at local z=0** (the `nfold_star` convention: verts start at
   z=0 and grow +z), **NOT** `twisted_column`'s centred `-height/2` convention —
   instancing sets `obj.location=(x,y,z)`, so a centred tree would sink half below
   terrain. (rubric F3)
3. **Materials default to EXISTING `MATERIALS` keys** (use `aged_stone`/`stone`
   for bark; pick an existing key for leaves) — `"bark"`/`"leaf"` are NOT in the
   table and adding them would breach additive-only. Materials don't affect the
   OBJ proof, so this only bites at the L4 material slice; default safely now.
4. **Determinism discipline:** one `random.Random(int(float(seed)))`, threaded in
   fixed depth-first child-index order; draw EVERY per-node RNG value
   unconditionally in fixed order, THEN decide keep/clip (never let a branch/clip
   decision gate a `.random()` call); no `set`/`dict` iteration, no `hash()`.

**RUBRIC SHARPENINGS (pinned, mostly OFFLINE-computable from `proof_mesh`):**
- **A1 (trunk vertical):** fit a line to the trunk-ring centroids BELOW
  `firstBranchHeight·height`; measure its angle from +z (< ~15°).
- **D2 (canopy fill):** pin the cheap proxy — `Σ(clump sphere volumes, not
  overlap-deduped) ÷ crown-bounding-sphere volume`, held in the reads-as-MASS
  BAND `0.15 ≤ fill < 0.6` (or a fixed-res voxel occupancy). The lower bound
  enforces "actually a canopy" (a near-empty crown reads as decorated tips); the
  upper bound rules out a solid lollipop. One definition, reproducible across
  reviewers.
- **F1 (manifold-ish):** concrete = no fully-duplicate verts beyond allowed join
  seams; every tube closed (2 cap faces per tube). `from_pydata` +
  `update(calc_edges=True)` succeeds.
- **L0 checklist (verify even though they're L5 gates — cheapest when the mesh is
  trivial):** F3 origin-at-base and F4 dual-tier parity.

The rubric is ~75% computable offline from a single `--obj-out` proof (vertex/
radius/angle math, tri count, hashes); reserve RENDERS for A3/A4/E1/E3 (silhouette
+ thumbnail). Poly budget (F2): compute the tri count OFFLINE before any render so
an over-budget tree (e.g. `childrenPerNode=4, branchLevels=5` ≈ 1.4k branches) is
caught for free.

## Sources

- [Tree Branching: Leonardo da Vinci's Rule versus Biomechanical Models (PLOS One / PMC3979699)](https://pmc.ncbi.nlm.nih.gov/articles/PMC3979699/) — da Vinci exponent Δ=2 (area-preserving), measured daughter/mother ratios ~1.1-1.4; field branch angles 10-50° (Fagus) / 50-80° (Abies).
- [Leonardo vindicated: Pythagorean trees… (arXiv 2411.08024)](https://arxiv.org/html/2411.08024v1) — A_left²+A_right²=A_parent² form of the rule.
- [Runions, Lane, Prusinkiewicz — Modeling Trees with a Space Colonization Algorithm (Algorithmic Botany, EG NPH 2007)](https://algorithmicbotany.org/papers/colonization.egwnp2007.pdf) — space colonization steps, d_i / d_k / D parameters.
- [Weber & Penn — Creation and Rendering of Realistic Trees, SIGGRAPH 1995](https://courses.cs.duke.edu/cps124/fall01/resources/p119-weber.pdf) — recursive branching params (nLevels, nDownAngle, nRotate, nLength, nTaper); "any number near 140° works well".
- [The Grove — The golden angle in trees](https://www.thegrove3d.com/research/the-golden-angle-in-trees/) — 137.5° alternate / 90° opposite divergence.
- [Open Oregon — Forest Measurements: Live Crown Ratio](https://openoregon.pressbooks.pub/forestmeasurements/chapter/5-4-live-crown-ratio/) — healthy LCR 40-60%.
- [Rule-based Procedural Tree Modeling Approach (arXiv 2204.03237)](https://arxiv.org/pdf/2204.03237) — branch angle + length ratio as the structural dials; apical-dominance via length ratio.
- Repo (verified): `tools/blender/craftsmen/nfold_star.py`, `twisted_column.py` (the three-part contract); `docs/departments/edi-blender-lab.md` (param-type limit, flat keys); `docs/architecture/realize-instancing.md` (build-once/place-many, origin convention).
