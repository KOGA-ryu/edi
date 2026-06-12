# edi — direction and build roadmap

*2026-06-11. The strategic record: what this project is, what stands, what
gets built next and why in that order. Companion to `shell_architecture.md`
(the host design) and the memory files (the running context). Update this
document at phase boundaries; a roadmap nobody maintains is a guess wearing
a filename.*

---

## 1. The thesis

edi is a toolchain for building 3D assets **without guesswork** — between
the user and the tool, between the tool and Blender, and between the human
and the AI. One sentence governs every design decision:

> **Every number is pointable.** A person (or an AI under instruction) can
> name the exact key, vertex, or term that must change — and nothing
> downstream re-derives, approximates, or silently substitutes it.

The pipeline that enforces it:

```
drafted geometry      recipe documents       proof              execution
(exact measurements) → (strict TOML,     →  (dry-run plan,  →  (Blender, via the
 the canvas is the      every key          ASCII projections,   craftsmen library —
 measurement            pointable)         later: mesh)         scripts read recipes)
 authority)
```

In the prototype's own words: **"Recipe is truth. ASCII preview is proof.
Blender script is execution."**

**Division of labor** (user directive, recorded): the user researches and
directs; the assistant builds the tools; the AI's distinct power is
capturing researched detail — the Michelangelo's-Moses standard, where a
pressed pinky reveals a rarely-seen forearm muscle because someone
*observed* it. A part is **acceptable-complete** only when it is (1) named
in the taxonomy, (2) researched — how the mason made it, with what tools,
documented — and (3) built with exact measurements. The scripts are the
craftsmen (durable, versioned techniques); the recipes are the dimension
sheets they read.

Two of the user's own projects ride alongside and are **theirs to build**:
the custom glyph language for previews (the seam exists: `AsciiGlyphSet`,
every mark a named role) and the text editor (its seam is the lab's bottom
panel). Build seams, never the things.

---

## 2. What stands (honest inventory)

**The drafting surface — done and earning.** Exact-measurement 2D canvas:
projection-cached painting (1.8 ms/frame @300 objects), per-object styling
through painter/SVG/HPGL/G-code, zoom + rulers in document units,
parametric creation and repeat/grid/radial arrays, atomic batch commands,
modular panels, close-without-saving guard. Every phase shipped through
adversarial review (20–56 confirmed findings per cycle, applied).

**The prototype — the spec.** `ascii_blender_dryrun_v0` is the user's
working proof: 15 typed ops, the classical moulding grammar, sweeps,
petal blooms, a Mii-grade face, a proportional humanoid, ~39 tests, 12
example recipes. It is a prototype on purpose: working in some ways,
broken in others. The port rule that has held: **its tests and generated
outputs are goldens; port from the library source, never the generated
script; every deliberate divergence is documented at the site.**

**The port — architectural core landed.** Moulding term compiler
(numerically golden-true), the 8-op vocabulary with strict TOML streams
and finding-report validators, the ASCII proof backend (three projections,
**byte-identical** to the prototype's own previews), and the craftsmen
library `edi_craft.py` (consumption-audited reader, computed preview rig,
v0-verbatim mesh math). The doric column exists in both pipelines, every
artifact byte-guarded by the suite.

**The unification — DONE (R1, 2026-06-11).** One vocabulary. Measurement
bindings (`.object`/`.field` side table + member-pointer registry),
drafted-profile references (`AddRevolvedProfile` lowering to moulding
points at resolve), the shared measurement/profile seam (`RecipeMeasure`,
pipeline A's wordings verbatim), an all-or-nothing resolve pass with
per-binding findings, explicit-cutter flutes, four File-menu verbs +
`edi_recipe_ops` CLI behind a resolved-stream gate — and pipeline A
deleted against its own benchmark: the same column, from the same
drafted profiles, byte-guarded through the survivor
(`tests/recipe_drafted_column_tests.cpp` resolves the REAL `.edidraw`).
Deferred as documented losses: `bevel` (a finish op, later) and `array`
(R6 jobs). Six slices, each adversarially reviewed; three built by an
external builder session under the operation protocol.

**The debt, named plainly:**

- **The proof tier has a ceiling.** Silhouettes catch scale/symmetry/order
  — perfect for architecture, weak for faces. The prototype's own #1 next
  slice (OBJ/GLB dry-run mesh, no Blender needed) is the answer.
- **The bpy half is reviewed, not executed.** Everything up to the Blender
  boundary is golden-tested; the boolean-heavy build path deserves an
  optional headless `blender --background` smoke.
- **Figure work is mid-restart** (see §3) — the most ambitious track is
  also the youngest, and its own author rebuilt its foundation last week.

---

## 3. The destination, sequenced by the evidence

The prototype's commit-hour lineage tells the true story of where this is
going: *columns → sweeps (twisted bars, rose scrolls) → petal sheets
(blooms, with real flower references) → a face as a mirrored petal sheet
(the Mii mask: 9 anatomical ribs, 10 named deformation fields, seeded
"slop" chisel-mark asymmetry) → figures built literally from column
machinery (limbs are tapered cylinders, `entasis: false`) → a 7.5-heads
proportional canon → 9 named humanoid body modules (data only, no consumer
yet) → and then a deliberate restart:* **`face_surface/`**, the newest code
in the lab.

That restart is the project's most important design statement. It re-founds
the face on an **addressable parametric surface grid** — every vertex
carries (u, v, side, row, col, *anatomical label*) — with a **selector
grammar that proves vertex selections numerically before any sculpting
touches them**, exact mesh statistics, and deformations stubbed with an
intentional `NotImplementedError` whose docstring reads:

> *"Deformations intentionally come after the address system, selectors,
> and mesh stats are proven."*

That is "recipe is truth, ASCII is proof" applied to anatomy: **name the
bridge of the nose before you push on it.** The roadmap's figure phases
follow this order because the user already chose it in code. Anything that
jumps from the Mii mask straight at Moses skips the foundation its author
just rebuilt on purpose.

The ladder, then: **column (done) → ornament (mouldings done; flutes done)
→ flora (sweeps, blooms, scrolls — next port) → architecture-at-scale
(convex walls + extrude → arches, vaults, tracery) → figures (addressable
grid → proven selectors → deformation fields → module-built bodies) →
the cathedral and the Moses-grade detail pass.** Each rung is independently
useful; none requires the rung above it to pay off.

---

## 4. Architecture north star

When the roadmap below is done, the system looks like this:

- **One op vocabulary** (the prototype's, industrialized). Fields accept
  literals **or** measurement bindings **or** drafted-profile references —
  resolution against the live drafting document is a property of the
  pipeline, not of one grammar. The vocabulary itself becomes a **spec
  document** (tool → params → meanings → bpy mapping) before it grows past
  ~20 ops: it is simultaneously the validator's table, the UI's palette,
  and the AI's reference, so no one ever guesses tool semantics.
- **Documents all the way down**, all strict, all pointable: recipes
  (TOML op streams), presets (named op bundles — the petal-preset compiler
  generalized to humanoid modules and beyond), the taxonomy (part →
  recipe-params + research doc), workspaces, vocabulary spec. Strict means:
  every key audited, every failure names its offender, both readers (C++
  and python) accept and reject identically.
- **Three proof tiers**, cheapest first: the dry-run plan (one line per
  op), the ASCII projections (glyphs as data — the user's glyph project
  drops in), and the dry-run **mesh** (OBJ/GLB, no Blender) for organic
  work the silhouettes can't judge.
- **The craftsmen library** as the only execution path: versioned
  techniques in `tools/blender/`, fed by compiled+resolved recipes, scene
  rig computed from the build's own bounds, failures raised loudly.
- **The lab** (shell feature #3, per `shell_architecture.md` §Feature 2):
  taxonomy tree left, scripting jobs right (click to scope into the
  chain), the terminal bottom (script text — the user's editor mounts
  here; ASCII render beside it), the canvas main (the shape forge).
  Recipe dirty-state folds into the close guard (documented obligation).
- **The AI loop, both directions**: the vocabulary spec + TOON emission so
  an AI knows *how* to build; **semantic diff** of recipes and drafted
  profiles ("step.4.param.count.value: 20 → 24"; "shaft vertex 3:
  (0.066, 0.330) → (0.070, 0.330)") so the tools state *what changed*
  without prose. Flat keys made pointability and diffability the same
  property — the diff is nearly free and disproportionately valuable.

---

## 5. The roadmap

Phases are ordered by dependency and leverage, not ambition. The standing
rule from every phase so far: **end with a runnable, pointable artifact**
— a thing that builds, previews, and can be pointed at — never a layer of
abstraction awaiting its consumer.

### R0 — Close the port phase *(DONE 2026-06-11, master `a73a2c6`)*
Finished the P7 review items plus a second adversarial pass over the
fixes themselves (12 more findings; all closed, each pinned by a
mutation-killable test). DoD met: 78/78 green on master; HANDOFF.md
deleted; memory updated.

### R1 — One vocabulary (the unification) *(DONE 2026-06-11 — see §2; commits c442f61..0262c85 on ui-restoration; the E-track [text editor host] runs next, pulled forward from R7 by user direction, then R2)*
The op pipeline absorbs pipeline A's two crown jewels:
1. **Measurement bindings** on op fields (literal | binding {objectId,
   field}) — promote `resolveMeasurement` from RecipeDocument.cpp's
   anonymous namespace into a shared seam; resolution happens in a
   `resolveRecipeOps(stream, draftingDoc, grid)` pass between load and
   compile.
2. **Drafted-profile references**: a new `AddRevolvedProfile` op (profile
   from a drafted polyline/arc chain via the existing page-left-axis /
   page-bottom-z convention in `resolveStepProfile`) lowering to
   `AddMoulding` points at resolve time — the lathe, op-vocabulary native.
3. Shell verbs migrate to the op pipeline (Open/Save Ops Recipe…, Export
   Resolved…, Export Previews…); an `edi_recipe_ops` CLI joins
   (EdiPlotJobReport pattern: load → compile → validate → render).
4. The shaper grammar **retires**; the column sample re-grounds once on
   drafted profiles + ops (the duplicated sample in `samples/doric_column`
   is the acceptance benchmark — same column, drafted numbers, one
   pipeline).
**DoD:** the column builds from *drafted* profiles through the op
pipeline end to end; pipeline A removed; one sample set; menus point at
the surviving pipeline.

### R2 — Deepen the proof tier
1. **OBJ/GLB dry-run mesh** export from the op stream, no Blender (the
   prototype's own #1 next slice) — the proof faces and flora actually
   need. Deterministic, byte-guardable.
2. Optional **headless Blender smoke** (`find_program(blender)`-gated,
   like the python gate): build the column `--background`, assert object
   count/names/bounds.
**DoD:** the column's mesh proof is committed and diffable; Blender smoke
runs where Blender exists.

### R3 — Port the floral family
`sweep_geometry.py` (15 functions) + AddSectionStack, AddPathSweep,
AddPetalBloom, AddPetalBloomPreset (the preset *compiler* — the pattern
the humanoid modules will reuse), AddPetalBloomDetail, AddPetalScroll.
Goldens: the prototype's 23 floral pytest cases + its 8 floral example
recipes; the preset zoo becomes the byte-guarded sample. Same fidelity
rule, same divergence discipline.
**DoD:** every floral example recipe builds via edi byte-true to v0;
the rose zoo previews and dry-runs under the suite.

### R4 — The shape forge (canvas work, drafting-surface priority)
1. **Per-edge curvature** on closed shapes: vertices pinned, each wall's
   bulge one exact pointable number (the convex-walls rectangle), drawn
   and edited on canvas with full inspector support.
2. **AddExtrudedProfile**: a drafted closed profile extruded along z —
   the cathedral's workhorse (walls, buttress sections, tracery ribs).
3. Path families beyond spiral (prototype next-slice #5: Bézier strokes,
   arches, S-scrolls, tracery ribs) as drafted-path references — the
   canvas draws the path, the sweep follows it.
**DoD:** a curved-wall drafted shape extrudes to Blender with every wall's
curvature pointable; one arch sample, drafted to built.

### R5 — Figures, in the author's own order
Port **face_surface generation 2** as the foundation (it is the newest,
most rigorously tested code in the lab — 14 exact-value tests):
1. The addressable grid (profile stations → labeled mirrored UV grid),
   selector grammar, and mesh stats — with the selector *reports* as the
   proof artifact ("chin band: exactly 52 vertices").
2. **Then** deformation fields onto the grid — the stubbed phase two —
   using the Mii mask's 10 named fields (gen 1) as the behavioral spec,
   targeted through selectors instead of free-form rib math.
3. The **humanoid module compiler**: make the 9 data-only body modules
   buildable (the petal-preset compiler generalized), assembling the
   proportional figure from named parts.
4. Mount the grid face on the proportional figure.
Gen-1 `AddMirroredFacePetal` ports only as a golden bridge if cheap; the
grid is the future the user already chose.
**DoD:** a face whose every region is selectable by name with a numeric
proof, deformed by pointable fields; a figure assembled from named
modules.

### R6 — Library, jobs, and the taxonomy data model
Recipes referencing recipes (a job = a chain — "a forehead is seven
scripts"); a library directory of named recipes/presets (standard set +
user's own); the taxonomy as data: part → recipe-params binding + research
document slot. Pure models with strict stores first; the UI consumes them
in R7.
**DoD:** the column, an arch, and a bloom exist as library entries a job
can chain; a taxonomy file binds parts to entries.

### R7 — The lab (shell feature #3)
The four-panel workspace per the recorded semantics: taxonomy left, jobs
right with drill-down, terminal bottom (script text view — the user's
editor seam — plus ASCII/mesh preview), canvas main. The op pipeline earns its controller here (or the
window's m_opsStream is ratified as enough — B05 flag 7 deferred this
decision to R7), and the ops-recipe dirty flag (documented obligation,
migrated from pipeline A's controller at its R1-B06 retirement) folds
into the close guard.
FeatureDescriptor row #3 on the existing registry; the
bus carries the drawing controller today and grows what the lab needs
(pipeline A's recipeController slot was removed with A in R1-B06).
**DoD:** the column workflow runs entirely inside edi: draft → recipe →
proof → export, no hand-run CLI.

### R8 — The AI loop tooling
1. **Vocabulary as a spec document** (before the op count grows past
   ~20): each op/param's meaning and bpy mapping as strict data; the
   validators and the AI read the same file.
2. **Semantic diff**: recipe-to-recipe (flat-key delta) and profile-to-
   profile (vertex delta), emitted as a document — "what I changed" /
   "what you must change," machine- and human-readable.
3. **TOON emission** for AI handoffs (per the format law: TOML for
   humans-and-tools, MessagePack for documents, TOON for AI).
**DoD:** an AI session can be handed vocabulary + recipe + diff and
produce a correct edit naming exact keys — demonstrated once, recorded.

### R9 — Research binding and the long ladder
Taxonomy nodes bind research documents (the user's lane: how the mason
cut it, how it was set, sources); the reference library begins. *(Survey
correction, 2026-06-11: the lab repo `~/gameguy-3d-lab` already holds
substantial research — `docs/research/` (19 areas incl. architectural
measurements with an honest weak-areas ledger, head construction, craft
fabrication methods, UI workbench design), `geometry_dictionary/` (58
schema'd terms — a working v0 of R8's vocabulary spec), and
`data/characters/head_construction/`. The earlier "references are
unstarted" note was wrong; the open lane is filling the gaps the lab
itself names, not starting from zero. See `.claude/plans/STATE.md`
research queue for the phase-by-phase mapping.)* The
acceptable-complete bar applies from here forward; before R9 it is
deliberately *not* enforced, or the early rungs would never ship.
The prototype's "ASCII/image reference parser" idea lives here as a
research-assist, not a guesswork channel.
**DoD:** one part — suggested: the Doric flute — taken to
acceptable-complete: named, researched, built. The standard demonstrated,
not described.

---

## 6. Method (the constitution — proven, keep)

1. **Prototype loose, port golden-true.** Messy working code is the spec;
   its tests and outputs are the corpus; numeric/byte fidelity is what
   "port" means. Port from the *library source*, never the generated
   artifact (this bit us once; the review caught two false "v0 never did
   X" claims).
2. **Strictness is the product.** Every reader audits every key by
   consumption and names its offender. Both language sides accept/reject
   identically (parity-tested). Silent fallback = guesswork by omission.
3. **Divergences documented at the site.** A fix to prototype behavior
   carries a comment saying what v0 did and why this differs. A limitation
   written down is a parameter not yet built; one unwritten is a guess
   someone inherits.
4. **Every flagship artifact is byte-guarded.** If it's committed and the
   suite doesn't compare it, it will drift — the review proved this on the
   one file that lacked a guard.
5. **Adversarial review every phase** (find → verify). It has caught
   must-fixes in 100% of rounds, including bpy-semantics bugs verified
   against a live Blender. Budget for it.
6. **Mutation-check new test logic once**; hard-rebuild around
   mutate/restore (mtime granularity runs stale binaries).
7. **Known traps, paid for once**: python `round()` is banker's rounding;
   `strtod`/`std::to_string` are locale-sensitive — `from_chars`/
   `to_chars` only; Qt modals hang offscreen suites — every dialog is an
   injectable callable; QSS owns geometry; pixels are truth, property
   reads lie.
8. **Each phase ends with a runnable pointable thing.** The project's
   anti-scope-death mechanism: if the summit is never reached, every camp
   was worth building.

---

## 7. Open decisions (the user's, not the roadmap's)

| # | Decision | Default until decided |
|---|----------|----------------------|
| 1 | **The bar**: when does acceptable-complete (named+researched+built) start binding? | R9 — earlier rungs ship at built-and-pointable |
| 2 | **Gen-1 face op**: port `AddMirroredFacePetal` for fidelity, or let gen-2 supersede it? | Supersede; keep the Mii recipe as a behavioral spec for R5's fields |
| 3 | **AddRing**: stay a cylinder alias or become a true torus? | Alias (validator notes it); torus when something needs it |
| 4 | **Tapered flute cutters** (real Doric flutes follow the taper) | Straight cutters, documented, until a column demands better |
| 5 | **Mesh proof format**: OBJ (simple, diffable) vs GLB (richer) | OBJ first — text, byte-guardable |
| 6 | **Glyph project & text editor timing** — both are seams awaiting the user | No deadline; seams stay stable |

---

*Maintenance: revise at each phase merge — move the finished phase into §2,
re-examine the order of what remains, and record any decision from §7 that
got made. A direction document that survives contact with three phases
unedited is no longer describing the project.*
