# Architecture — edi-blender-lab (the recipe lab / Seam A)

> **Status:** 2026-06-17, **feature batch `blender-lab-20260617-feature-batch` COMPLETE
> (all 15 tasks, BL-01..15)**. Anchors re-trued at batch close; variant stable at 14
> arms. The durable map of how the recipe lab is structured. Folded from the cartography
> reviewer gate, then updated through the feature batch: the extrude spine (BL-01..04),
> push/pull (BL-05), lathe partial-revolve + helix (BL-06/07), the Follow-Me sweep +
> taper (BL-08/09), inset/normalOffset depth verbs (BL-10), the solid boolean (BL-11),
> two sacred-geometry craftsmen (BL-12/13), the named-recipe library + chaining (BL-14),
> and the resolved-stream TOON handoff (BL-15). Throughout, every pre-existing golden
> (doric, the prism/sweep samples) stayed **byte-identical** — new behavior is opt-in,
> default-off. Closeout: `docs/closeouts/blender-lab-feature-batch.md`.
>
> Doctrine: **"Recipe is truth. ASCII preview is proof. Blender script is
> execution."** The human composes an op stream by CLICKING; the AI edits the
> same stream as TOML; both mutate one `m_opsStream`, kept in sync by
> `opsStreamChanged`. Every step is proven in ASCII/OBJ before Blender runs it.

## 1. The `RecipeOp` variant — the vocabulary core

`using RecipeOp = std::variant<…>` at `src/recipe/RecipeOps.h:235`. **14 arms** —
the original 10 plus the four added by the 2026-06-17 feature batch (BL-01
AddExtrudedProfile, BL-03 AddPrism, BL-08 AddSweepProfile, BL-11 AddBoolean).

> **Anchors re-trued at batch close (2026-06-17).** The variant is now stable at 14;
> `(:NN)` anchors below are current as of the feature batch's last commit. Trust the
> symbol, re-grep the line.

Variant declaration order (the index, for visit purposes): AddBox, AddCylinder,
AddSphere, AddRing, AddMoulding, **AddPrism**, AddProfileMoulding, AddRevolvedProfile,
**AddExtrudedProfile**, **AddSweepProfile**, CutFlutes, **AddBoolean**, AddLabel, Script.

| op (decl) | key fields | purpose |
|---|---|---|
| `AddBoxOp` | width/depth/height/z/x/y; material; zMode | rectangular block |
| `AddCylinderOp` | radius/height/z/x/y; vertices; taperTopRadius?; entasis(+ratio); axis; zMode | shaft / column core |
| `AddSphereOp` | radius/z/x/y; vertices(=24); material | sphere |
| `AddRingOp` | radius/tubeHeight/z/overhang/x/y; vertices | ring (cylinder alias) |
| `AddMouldingOp` | baseZ; profile = vector<MouldingPoint{z,radius,term}>; x/y; vertices; **sweepDegrees(360); screwRise(0)/screwTurns(1)** | low-level lathe (BL-06 partial revolve, BL-07 helix) |
| `AddProfileMouldingOp` | baseZ; sequence = vector<MouldingSegment>; … | term-sequence moulding; **compiles INTO AddMoulding** |
| `AddRevolvedProfileOp` | **profile = str (drafted id)**; baseZ; x/y; vertices; **sweepDegrees/screwRise/screwTurns** | **the lathe**; refused-before-build, **lowers to AddMoulding** (the params survive lowering) |
| `CutFlutesOp` | target(str, names earlier op); count; depth; widthRatio; cutter…? | radial flute grooves; targets earlier op BY NAME |
| `AddLabelOp` | text; x/y/z | Blender-side text annotation |
| `ScriptOp` | scriptId(str); x/y/z; params = vector<ScriptParam{key,value}> | custom Python craftsman; untyped param bag; OBJ-only proof |
| `AddExtrudedProfileOp` (:167) | **profile = str (drafted id)**; height; baseZ; x/y; material | **the extrude** (BL-01) — refused-before-build, **lowers to AddPrism** (neg height = push/pull-cut) |
| `AddSweepProfileOp` | **profile + path (drafted ids)**; baseZ; x/y; material; **taperEnd(1)** | **the Follow-Me sweep** (BL-08/09) — refused-before-build, **lowers to a path-bearing AddPrism** |
| `AddPrismOp` (:119) | footprint = vector<PrismPoint{x,y}>; height; baseZ; x/y; material; **path** (BL-08); **taperEnd** (BL-09); **inset/normalOffset** (BL-10) | **the generalized extrude/sweep carrier** — empty path = straight extrude, present = swept solid; NOT refused, OBJ-only proof |
| `AddBooleanOp` | **a + b (names of earlier ops)**; kind = BooleanKind{Union,Subtract,Intersect} | **solid boolean** (BL-11) — combines two earlier ops by name; proof emits operands, CSG is bpy-only |

Supporting types: `ScriptParam`, `RecipeFieldBinding` (opIndex/fieldKey/objectId/field,
the **parallel** binding table), `RecipeOpStream` (id/name/ops/bindings),
`PrismPoint` (:106 — one planar footprint vertex, physical x/y), `BooleanKind` (enum).

**The design WIN:** a new arm forces `-Werror=switch`-style exhaustiveness across
every `std::visit` interpreter (below) — a forgotten interpreter *cannot compile*.
That is why behavior lives in free-function visitors over a closed variant, not in
virtual methods on a class hierarchy: the compiler is the checklist.

## 2. The interpreter sites — corrected count

The charter's shorthand "7 visit sites (namer, store writer+reader, validate,
resolve, ascii, bind, schema)" lists 7 **roles**, not visit sites, **and two of
those roles are not `std::visit` at all.** The accurate map:

**Compiler-exhaustive `std::visit` over `RecipeOp` — now 13 call sites / 12 distinct
visitors**, each covering **14 arms**. The feature batch added two NEW exhaustive
visitors beyond the originals: **`MutableName`** (RecipeOps.cpp:228/245 — the writable
own-name pointer, nullptr for nameless ops; BL-14) and **`NameRefRemapper`**
(RecipeOps.cpp:251/281 — rewrites op-name REFERENCES on chaining: CutFlutes→target,
AddBoolean→a/b, the other 12 explicit no-ops; BL-11 hardened it from a `get_if` so a
future name-ref arm MUST declare its remap or fail to compile). The
`static_assert(std::variant_size_v<RecipeOp> == 14)` tripwire is at RecipeOpsStore.cpp:752.
Live visit anchors: Namer (RecipeOps.cpp:44), MutableName (:245), NameRefRemapper (:281),
OpWriter (RecipeOpsStore.cpp:713), OpChecker (RecipeOpsValidate.cpp:460), NameGetter
(:436), BoundsEstimator (RecipeOpsAscii.cpp:168), ProjectionDrawer (:569), FieldVisit ×2
(RecipeOpsBind.cpp:195/208), FieldList (:213), appendExtras/setExtra (RecipeOpSchema.cpp).
An added arm fails to compile at each (overload set has no match).

| visitor | site | mechanism |
|---|---|---|
| `Namer` | RecipeOps.cpp:40 | std::visit + overload struct |
| `OpWriter` (store WRITER) | RecipeOpsStore.cpp:571 | std::visit + overload struct |
| `OpChecker` (validate) | RecipeOpsValidate.cpp:297 | std::visit + overload struct |
| `NameGetter` (validate) | RecipeOpsValidate.cpp:273 | std::visit + overload struct |
| `BoundsEstimator` (ascii framing) | RecipeOpsAscii.cpp:158 (struct :76) | std::visit + overload struct templated on the `include` lambda |
| `ProjectionDrawer` (ascii draw) | RecipeOpsAscii.cpp:535 (struct :344) | std::visit + overload struct |
| `FieldVisit` (bind — exists) | RecipeOpsBind.cpp:144 | std::visit + member-ptr table |
| `FieldVisit` (bind — write) | RecipeOpsBind.cpp:157 | std::visit + member-ptr table |
| `FieldList` (bind — list) | RecipeOpsBind.cpp:162 | std::visit + member-ptr table |
| `appendExtras` (schema) | RecipeOpSchema.cpp:264 | std::visit generic-λ → 10 free-fn overloads |
| `setExtra` (schema) | RecipeOpSchema.cpp:277 | std::visit generic-λ → 10 free-fn overloads |

> `BoundsEstimator` was added by the cartography campaign (`4a561e8`) — it replaced a
> non-exhaustive 5/10 `get_if` ladder. See §10.

**The two roles that are NOT compiler-enforced** (a new arm slips through silently):

- **Store READER** `recipeOpsFromToml` (RecipeOpsStore.cpp:584–845): a **string
  if/else-if ladder over the `type` key** with a refusing default at :844
  (`"unknown op type"`). A new variant arm does not force a branch here — it is
  rejected at *runtime* as unknown, not at compile time. This is the single place
  the "every visit is exhaustive" promise is not the compiler's job. **Guarded since
  the cartography campaign by a `static_assert(std::variant_size_v<RecipeOp> == 10)`
  tripwire at the top of the function (`b6af915`, :597)** — growing the variant now
  fails this assert, reminding the author to add a reader branch + a matching
  `parse_ops` arm. See §10.
- **Resolve** `resolveRecipeOps` (RecipeOpsResolve.cpp): touches only the lathe arm
  by design (`get_if<AddRevolvedProfileOp>` at :93, `holds_alternative<…>` at :166).
  Not a dispatch over all arms; nothing to make exhaustive.

**`estimateBounds` is now exhaustive (was the 3rd silent `get_if` ladder).** Before
the cartography campaign it framed only 5/10 ops via a `get_if` chain; `4a561e8`
converted it to the `BoundsEstimator` `std::visit` above (RecipeOpsAscii.cpp:144,
struct :76) — 5 ops keep identical extents, the other 5 are explicit no-op arms, so a
new op now forces a framing decision at compile time.

**Empty / stub `ProjectionDrawer` arms** (all deliberate, most commented):
`AddLabelOp` (:480, labels are Blender-side text) and `ScriptOp` (:486, a
craftsman's shape is its Python `proof_mesh`; OBJ is its proof tier, not 2D ASCII).
`AddProfileMoulding`/`AddRevolvedProfile` arms (:452/:453) are empty but
**unreachable** — `renderOpsProjection` refuses both at :502/:507 before dispatch.
`OpChecker(AddLabelOp)` is empty (:233 — nothing to validate). (The 5 `BoundsEstimator`
no-op arms — AddProfileMoulding/AddRevolvedProfile/CutFlutes/AddLabel/Script — are at
:131–138.) **AddExtrudedProfile** + **AddSweepProfile** (refused-before-build) get the
lathe treatment: no-op bounds + empty draw + a `renderOpsProjection` refusal. **AddPrism**
+ **AddBoolean** (buildable) mirror `ScriptOp`: no-op bounds + empty draw + **NO** refusal
— invisible in ASCII, proven in OBJ (AddBoolean's OBJ proof emits its named operands; the
CSG is bpy-only).

## 3. The C++↔Python TOML contract — key-for-key, NO DRIFT

The cross-language seam: C++ `OpWriter` **writes** a TOML op stream; Python `parse_ops`
**reads** it. **Every key the writer emits, the reader consumes, with matching
name/type/default. No drift** — verified live across the whole batch (doric +
extruded_figure + swept_profile + boolean_op `--obj-out` all byte-identical golds).
**The flat `op.N.<field>` key scheme is shared three ways:** `recipeOpsToToml` (truth),
`exportRecipeStreamToToon` (the AI handoff, BL-15 — both project ONE `recipeOpsToConfig`,
so they cannot drift), and Python `parse_ops`. Feature-batch contract additions:
`AddPrism` carries `footprint.i.{x,y}` + optional `path.i.{x,y}` + `height,base_z,x,y,
material,taper_end,inset,normal_offset` (all defaults are byte-preserving); `AddMoulding`/
`AddRevolvedProfile` gained `sweep_degrees,screw_rise,screw_turns`; `AddBoolean` writes
`a,b,kind`; the refused-before-build ref-ops (`AddRevolvedProfile`/`AddExtrudedProfile`/
`AddSweepProfile`) are refused by name on BOTH sides (only their lowered carriers reach
Python).

**Extrude spine (2026-06-17) extends the contract:** `AddExtrudedProfile` is
**refused-before-build on BOTH sides** (Python `parse_ops` raises "must be resolved
before building", edi_craft.py:245 — only the lowered carrier reaches Python).
`AddPrism` (the carrier) is read key-for-key: writer `type,name,height,base_z,x,y,
material` + a contiguous `footprint.i.{x,y}` run ↔ Python `parse_ops` AddPrism branch
(edi_craft.py:348) + `_prism_world` mesh (:652) + `add_prism` build (:1041). Audited
exact, defaults included (BL-04, `replies/010`).

Per-op keys (all confirmed equal): AddBox `type,name,width,depth,height,z,x,y,material,z_mode`;
AddCylinder adds `vertices,[taper_top_radius],entasis,entasis_ratio,axis`; AddSphere
`…,vertices(=24),material`; AddRing `…,tube_height,overhang`; AddMoulding `base_z` +
`profile.i.{term,z,radius}`; AddPrism `name,height,base_z,x,y,material` + `footprint.i.{x,y}`;
CutFlutes `target,count,depth` + `(cutter_radius+at_radius)`
**XOR** `width_ratio(0.28)` + `[start_z],[end_z]` (the XOR + present-together rule is
enforced **identically and with the same wordings** on both sides —
RecipeOpsStore.cpp:735–758 / edi_craft.py:344–351); AddLabel `type,name,text,x,y,z`;
Script `type,script,name,x,y,z` + param bag. `AddProfileMoulding`/`AddRevolvedProfile`
are refused-before-build on BOTH sides (they never reach Python).

**Where drift would silently break the proof:** any key renamed/retyped on one side
only, or a new op's writer/reader keys diverging. The contract has no compiler — the
guard is the cross-language smoke (`--obj-out`, `tests/edi_craft_smoke.py`). Run it
on every store or `edi_craft.py` change.

Param-key rule `recipeScriptParamKeyProblem` (RecipeOps.cpp:43) enforced in THREE C++
places — store write (RecipeOpsStore.cpp:560), store read (:834), validate
(RecipeOpsValidate.cpp:251) — guarding the bare-key/no-collision rule the Python
`tomllib` half needs. Materials table parity: C++
`recipeMaterialTable` (RecipeOps.cpp:10) ≡ Python `MATERIALS` (edi_craft.py:57),
identical 7 entries.

## 4. Resolve / lowering (RecipeOpsResolve.cpp)

`resolveRecipeOps(stream, drafting, grid)` → a COPY (pure pass):
1. **Bindings pass** (:28–72): for each `RecipeFieldBinding`, measure via the shared
   `resolveMeasurementField` (RecipeMeasure); `binding.field` = which measurement
   (width/height/length/radius), `binding.fieldKey` = where on the op it lands. Gates
   finiteness (:59), writes through the bind registry member pointer `setOpFieldValue`
   (:68; false → "not a bindable field"). Bound opIndex check at :38.
2. **Lathe lowering** (:92–139): every `AddRevolvedProfileOp` → `AddMouldingOp` via
   `resolveProfilePoints` (RecipeMeasure). **Page-to-part convention:** seam points are
   physical (x=radius, y=z-from-page-bottom); moulding points are (z,radius) local to
   baseZ. Runs AFTER bindings. A strictly-falling profile is direction-normalized
   (order reversed, no coordinate touched, :112–121); a folded/non-monotonic profile
   lowers verbatim and fails validation honestly (`moulding_profile_not_monotonic`).
3. **Extrude lowering** (RecipeOpsResolve.cpp:141, the spine's BL-03): every
   `AddExtrudedProfileOp` → `AddPrismOp` via `resolveExtrudeProfilePoints` (RecipeMeasure).
   **Different page-to-part convention from the lathe:** extrude takes the drafted figure
   as a **footprint** (drafted x→physical x, drafted y→physical y, NO radius/z-flip) and
   rises +z by `height` from baseZ — contrast the lathe's page-left=axis spin. The two
   projectors share one point-extraction helper `resolveProfileSource` (arc sampling +
   the four refusal wordings live once); the lathe output stayed byte-identical. Runs
   after bindings; refuses deleted/open/degenerate profiles by name.
4. **All-or-nothing**: any finding → empty stream; else clear bindings, ok.

**Why a parallel binding table, not a sum-type per field:** the op's plain doubles
stay the resolved value every downstream consumer reads — no consumer needs to know a
field was bound. Refuse-by-name when unresolved: `recipeOpsResolved` (RecipeOpsResolve.cpp:220)
is false if any binding remains OR any `AddRevolvedProfileOp` **or `AddExtrudedProfileOp`**
survives; downstream refusals by name at compile, ascii, Python `parse_ops`. After the
spine, **no raw extrude survives a successful resolve** — only the lowered `AddPrism`
carrier (buildable) does.

## 5. The proof tiers

| tier | producer | proves | ops drawn | invisible |
|---|---|---|---|---|
| **ASCII** | `renderOpsProjection` (RecipeOpsAscii.cpp:491) | 2D silhouette front/side/top vs goldens | Box, Cylinder, Sphere, Ring, Moulding, CutFlutes | **Script (empty :486)**, AddLabel (empty :480); ProfileMoulding/Revolved refused :502/:507 |
| **dry-run** | `plan_lines` (edi_craft.py:685) | one deterministic build line per op | Box, Cylinder, Sphere, Ring, Moulding, CutFlutes, AddLabel | **Script — no branch** (header counts it, emits no line) |
| **compiled** | `compileRecipeOps` (RecipeOps.cpp:70) | ProfileMoulding→Moulding term expansion | — | — |
| **OBJ mesh** | `obj_objects`/`obj_lines` (edi_craft.py:641/670) | deterministic mesh, honest dimensions | all incl. **Script via `proof_mesh`** and **AddPrism via `_prism_world`** (:652) | AddLabel (text, no mesh) |

**`ScriptOp` AND `AddPrism` are OBJ-only-proof** — invisible in ASCII (and Script in
dry-run too); the OBJ mesh (`--obj-out`) is their proof tier. The extrude's golden:
`samples/extruded_figure/extruded_figure.obj` (an L-bracket prism, 12v/8f, z-extent =
height, byte-pinned in `tests/edi_craft_smoke.py`). ASCII goldens:
`samples/doric_column/previews/*`.

## 6. Custom-craftsmen scan / manifest path

- **Discovery:** `load_craftsmen(folder)` (edi_craft.py:83) scans `*.py` (skips
  `_`-prefixed), imports each, registers those exposing a `MANIFEST` with an `id`.
  Default folder `tools/blender/craftsmen` (only `twisted_column.py` on disk today).
- **Manifest → C++:** `craftsmen_manifest_toml` (:103) emits flat TOML
  (`craftsman.i.id/label`, `craftsman.i.param.j.key/label/type/default`); C++
  `parseCraftsmanRegistryToml` (RecipeCraftsmen.cpp:7) reads the same index-run shape.
  `--list-craftsmen` is the bridge (called from app/main.cpp:227 — edi-ui host file).
- **Three-part contract:** `MANIFEST` + `proof_mesh(op)->(verts,faces)` (pure, **no
  bpy** — so the proof tier needs no Blender) + `build(op)` (bpy). `makeScriptOp`
  (RecipeCraftsmen.cpp:71) seeds each param at its manifest default.
- **Untyped param carriage:** `ScriptParam{key,value}` are raw strings; the *type*
  lives in the manifest (`param.type`), coerced by the craftsman at proof/build.

## 7. Seams (in / out / subprocess) — what is OURS vs adjacent

- **IN — consumes drafting (read-only):** via `RecipeMeasure`
  (`resolveMeasurementField`, `resolveProfilePoints`) **at resolve time only**. Reads
  the live DraftingDocument + grid; never writes the drafting core. Binding identity =
  the drafted object's immutable string id; the only dangle is deletion, caught
  per-parameter at resolve. *(Contract: `docs/recipe_binding_contract.md`.)*
- **OUT — produces opaque asset ids:** the stream `id` / op `name`s. Asset→solid
  expansion (Seam B) is **downstream**, not in `src/recipe` or `tools/blender`.
- **Blender subprocess — ADJACENT, not ours to edit:** `ProcessRunStore`
  (`src/io/ProcessRunStore.{h,cpp}`, async QProcess wrapper, injectable so offscreen
  tests never launch real Blender); the WHAT-to-run is a pure `BlenderRunPlan`
  (`src/scripting/BlenderRunPlan.h`, `planBlenderRender`). Wiring lives in
  `src/widgets/EdiShellWindow.cpp:199–212` (edi-ui host). **The lab owns the panel
  CONTENT; src/io + src/scripting + EdiShellWindow are seams we record, not edit.**

## 8. Click → rendered PNG (the call chain)

Click → mutate `m_opsStream` → `opsStreamChanged` fan-out (EdiShellWindow.cpp:261,
289,634) → panels re-serialize/re-render. ASCII proof re-renders in-process
(`renderOpsProjection`); the Blender job resolves (`resolveRecipeOps`,
EdiShellWindowIo.cpp:383,419,784) → `planBlenderRender` (:641) → `m_blenderRunner` →
`ProcessRunStore::run` → `onBlenderRunFinished` → PNG. *(Caveat: the per-slot
panel-content-vs-edi-ui-host split inside EdiShellWindow{,Io}.cpp (~1321 lines) is
grepped, not fully line-read; treat the host files as edi-ui's.)*

## 9. Hypotheses settled (planner priors → verified)

1. ~~**Only canvas→bpy bridge is the lathe; no extrude op.**~~ **SUPERSEDED
   2026-06-17** — the extrude spine (BL-01/03/04) added the second canvas→bpy bridge:
   `AddExtrudedProfile` (a drafted closed figure → depth) lowering to the `AddPrism`
   carrier → OBJ/Blender. The lathe is no longer the only bridge. (BL-08 will add a
   third: `AddSweepProfile`.)
2. **`AddRevolvedProfile` AND `AddExtrudedProfile` are authored, ABSENT from
   `recipePaletteOpTypes` —** still {AddBox, AddCylinder, AddSphere, AddRing}. Both the
   lathe and the extrude need a profile *reference*, so they are authored (pick a
   profile + tune), not one-click-appended. `AddPrism` is a lowered carrier, never
   hand-clicked.
3. **`ScriptOp` empty ASCII arm at RecipeOpsAscii.cpp:486 — CONFIRMED** (and
   commented). EXTENSION: Script is also dropped from the dry-run tier; OBJ is its only
   proof.

## 10. Refactor candidates (behavior-preserving only — ranked)

From the reviewer gate. Map-and-clean campaign: **no features** (no new ops, no
extrude). Status reflects the LANDED builder batch (`4a561e8`, `b6af915`,
reviewer-audited clean; cartography closed — see `docs/closeouts/blender-lab-cartography.md`).

| # | value | candidate | cheapest fix | status |
|---|---|---|---|---|
| 1 | MED–HIGH | `estimateBounds` non-exhaustive (5/10 ops, silent fall-through) | convert to a `std::visit` overload set; no-op arms for draw-nothing ops; same extents | **DONE — `4a561e8` (BoundsEstimator)** |
| 2 | MED | store READER if-ladder not compiler-exhaustive | `static_assert(variant_size_v<RecipeOp> == 10)` + teaching comment beside the ladder | **DONE — `b6af915`** |
| 3 | MED | charter says reader/resolve are exhaustive visits (they aren't) | record the corrected map (this doc) + a charter note | **DONE — this doc §2 + charter note** |
| 4 | LOW–MED | dry-run `plan_lines` emits no Script line (header miscounts) | add a Script branch | OPEN (deferred) — touches dry-run *behavior*; folds with the backlog Script-ASCII work (roadmap M1 slice 2) |
| 5 | LOW | craftsman `param.type` default: C++ "text" vs Python "number" | align C++ fallback / document | OPEN (deferred) — only reachable on hand-built registry TOML |
| 6 | LOW | `estimateBounds` omits AddLabel (Python `bounds_of` includes it) | comment the divergence (no behavior change) | **DONE — folded into `4a561e8` as the AddLabel-arm comment** |

No data-oriented-rule violations found: no subclassing-for-behavior, no stateful logic
objects, no hidden JSON, no `.js`/`.qml` in scope. The variant + free-function +
member-pointer-table dispatch is consistent throughout.
