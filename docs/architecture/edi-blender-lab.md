# Architecture — edi-blender-lab (the recipe lab / Seam A)

> **Status:** first draft, 2026-06-16 (campaign `blender-lab-20260616-cartography`).
> The durable map of how the recipe lab is structured. Folded from the reviewer
> gate's read-only enumeration (`~/dept-bus/edi-blender-lab/replies/002-…`),
> spot-verified by the planner. Keep this current as the lab changes.
>
> Doctrine: **"Recipe is truth. ASCII preview is proof. Blender script is
> execution."** The human composes an op stream by CLICKING; the AI edits the
> same stream as TOML; both mutate one `m_opsStream`, kept in sync by
> `opsStreamChanged`. Every step is proven in ASCII/OBJ before Blender runs it.

## 1. The `RecipeOp` variant — the vocabulary core

`using RecipeOp = std::variant<…>` at `src/recipe/RecipeOps.h:192`. **Exactly 10
arms** (verified — no more, no less):

| # | op (decl) | key fields | purpose |
|---|---|---|---|
| 1 | `AddBoxOp` (:41) | width/depth/height/z/x/y; material; zMode | rectangular block |
| 2 | `AddCylinderOp` (:53) | radius/height/z/x/y; vertices; taperTopRadius?; entasis(+ratio); axis; zMode | shaft / column core; taper + entasis bulge |
| 3 | `AddSphereOp` (:71) | radius/z/x/y; vertices(=24); material | sphere |
| 4 | `AddRingOp` (:81) | radius/tubeHeight/z/overhang/x/y; vertices | ring (cylinder alias; overhang widens radius) |
| 5 | `AddMouldingOp` (:95) | baseZ; profile = vector<MouldingPoint{z,radius,term}>; x/y; vertices | low-level lathe: explicit (z,radius) points |
| 6 | `AddProfileMouldingOp` (:105) | baseZ; sequence = vector<MouldingSegment>; … | term-sequence moulding; **compiles INTO AddMoulding** |
| 7 | `AddRevolvedProfileOp` (:124) | **profile = str (drafted object id)**; baseZ; x/y; vertices | the lathe; a REFERENCE to a drafted profile; **lowers to AddMoulding at resolve** |
| 8 | `CutFlutesOp` (:134) | target(str, names earlier op); count; depth; widthRatio; cutterRadius?/atRadius?/startZ?/endZ? | radial flute grooves; explicit-cutter XOR widthRatio |
| 9 | `AddLabelOp` (:155) | text; x/y/z | Blender-side text annotation |
| 10 | `ScriptOp` (:183) | scriptId(str); x/y/z; params = vector<ScriptParam{key,value}> | dispatch to a custom Python craftsman; untyped param bag |

Supporting types: `ScriptParam` (:168), `RecipeFieldBinding` (:213 —
opIndex/fieldKey/objectId/field, the **parallel** binding table), `RecipeOpStream`
(:220 — id/name/ops/bindings).

**The design WIN:** a new arm forces `-Werror=switch`-style exhaustiveness across
every `std::visit` interpreter (below) — a forgotten interpreter *cannot compile*.
That is why behavior lives in free-function visitors over a closed variant, not in
virtual methods on a class hierarchy: the compiler is the checklist.

## 2. The interpreter sites — corrected count

The charter's shorthand "7 visit sites (namer, store writer+reader, validate,
resolve, ascii, bind, schema)" lists 7 **roles**, not visit sites, **and two of
those roles are not `std::visit` at all.** The accurate map:

**Compiler-exhaustive `std::visit` over `RecipeOp` — 10 call sites / 9 distinct
visitor mechanisms.** An added arm fails to compile at each (overload set has no
match):

| visitor | site | mechanism |
|---|---|---|
| `Namer` | RecipeOps.cpp:40 | std::visit + overload struct |
| `OpWriter` (store WRITER) | RecipeOpsStore.cpp:571 | std::visit + overload struct |
| `OpChecker` (validate) | RecipeOpsValidate.cpp:297 | std::visit + overload struct |
| `NameGetter` (validate) | RecipeOpsValidate.cpp:273 | std::visit + overload struct |
| `ProjectionDrawer` (ascii draw) | RecipeOpsAscii.cpp:492 | std::visit + overload struct |
| `FieldVisit` (bind — exists) | RecipeOpsBind.cpp:144 | std::visit + member-ptr table |
| `FieldVisit` (bind — write) | RecipeOpsBind.cpp:157 | std::visit + member-ptr table |
| `FieldList` (bind — list) | RecipeOpsBind.cpp:162 | std::visit + member-ptr table |
| `appendExtras` (schema) | RecipeOpSchema.cpp:264 | std::visit generic-λ → 10 free-fn overloads |
| `setExtra` (schema) | RecipeOpSchema.cpp:277 | std::visit generic-λ → 10 free-fn overloads |

**The two roles that are NOT compiler-enforced** (a new arm slips through silently):

- **Store READER** `recipeOpsFromToml` (RecipeOpsStore.cpp:605–831): a **string
  if/else-if ladder over the `type` key** with a refusing default at :828
  (`"unknown op type"`). A new variant arm does not force a branch here — it is
  rejected at *runtime* as unknown, not at compile time. This is the single place
  the "every visit is exhaustive" promise is not the compiler's job. *(Hardening
  candidate — see §10.)*
- **Resolve** `resolveRecipeOps` (RecipeOpsResolve.cpp): touches only the lathe arm
  by design (`get_if<AddRevolvedProfileOp>` at :93, `holds_alternative<…>` at :166).
  Not a dispatch over all arms; nothing to make exhaustive.

**A third non-exhaustive `get_if` ladder the charter never claimed:** `estimateBounds`
(RecipeOpsAscii.cpp:81–116) accumulates ASCII framing bounds for **only 5 of 10**
ops (Box, Cylinder, Sphere, Ring, Moulding) and silently ignores the other 5. A new
op compiles with no bounds contribution. *(Top hardening candidate — see §10.)*

**Empty / stub `ProjectionDrawer` arms** (all deliberate, most commented):
`AddLabelOp` (:437, labels are Blender-side text) and `ScriptOp` (:443, a
craftsman's shape is its Python `proof_mesh`; OBJ is its proof tier, not 2D ASCII).
`AddProfileMoulding`/`AddRevolvedProfile` arms (:409/:410) are empty but
**unreachable** — `renderOpsProjection` refuses both at :456/:464 before dispatch.
`OpChecker(AddLabelOp)` is empty (:233 — nothing to validate).

## 3. The C++↔Python TOML contract — key-for-key, NO DRIFT

The cross-language seam: C++ `OpWriter` (RecipeOpsStore.cpp:47–216) **writes** a TOML
op stream; Python `parse_ops` (`tools/blender/edi_craft.py:201–385`) **reads** it.
The reviewer checked every op key-for-key: **every key the writer emits, the reader
consumes, with matching name/type/default. No drift.** Verified live on the doric
sample: `--dry-run`, `--obj-out` (13148-line OBJ), `--list-craftsmen` all green.

Per-op keys (all confirmed equal): AddBox `type,name,width,depth,height,z,x,y,material,z_mode`;
AddCylinder adds `vertices,[taper_top_radius],entasis,entasis_ratio,axis`; AddSphere
`…,vertices(=24),material`; AddRing `…,tube_height,overhang`; AddMoulding `base_z` +
`profile.i.{term,z,radius}`; CutFlutes `target,count,depth` + `(cutter_radius+at_radius)`
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
places — store write (:558), store read (:817), validate (:250) — guarding the bare-
key/no-collision rule the Python `tomllib` half needs. Materials table parity: C++
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
3. **All-or-nothing** (:146): any finding → empty stream; else clear bindings, ok.

**Why a parallel binding table, not a sum-type per field:** the op's plain doubles
stay the resolved value every downstream consumer reads — no consumer needs to know a
field was bound. Refuse-by-name when unresolved: `recipeOpsResolved` (:156) is false
if any binding remains OR any `AddRevolvedProfileOp` survives; downstream refusals by
name at compile (RecipeOps.cpp:78), ascii (:456/:464), Python parse_ops (:228/:231).

## 5. The proof tiers

| tier | producer | proves | ops drawn | invisible |
|---|---|---|---|---|
| **ASCII** | `renderOpsProjection` (RecipeOpsAscii.cpp:448) | 2D silhouette front/side/top vs goldens | Box, Cylinder, Sphere, Ring, Moulding, CutFlutes | **Script (empty :443)**, AddLabel (empty); ProfileMoulding/Revolved refused |
| **dry-run** | `plan_lines` (edi_craft.py:685) | one deterministic build line per op | Box, Cylinder, Sphere, Ring, Moulding, CutFlutes, AddLabel | **Script — no branch** (header counts it, emits no line) |
| **compiled** | `compileRecipeOps` (RecipeOps.cpp:70) | ProfileMoulding→Moulding term expansion | — | — |
| **OBJ mesh** | `obj_objects`/`obj_lines` (edi_craft.py:641/670) | deterministic mesh, honest dimensions | all incl. **Script via `proof_mesh`** | AddLabel (text, no mesh) |

**`ScriptOp` is invisible in TWO proof tiers — ASCII *and* dry-run. OBJ (`--obj-out`)
is its only proof.** ASCII goldens: `samples/doric_column/previews/*`.

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

1. **Only canvas→bpy bridge is the lathe (`AddRevolvedProfile`); no extrude op —
   CONFIRMED.** (M2 of the roadmap would add `AddExtrudedProfile`; not this campaign.)
2. **`AddRevolvedProfile` is TOML/CLI-only, ABSENT from `recipePaletteOpTypes` —
   CONFIRMED.** Palette = {AddBox, AddCylinder, AddSphere, AddRing} (RecipeOps.cpp:115);
   `makeRecipeOp` builds only those four. The lathe needs a profile reference, so it is
   authored, not one-click-appended.
3. **`ScriptOp` empty ASCII arm at RecipeOpsAscii.cpp:443 — CONFIRMED** (and
   commented). EXTENSION: Script is also dropped from the dry-run tier; OBJ is its only
   proof.

## 10. Refactor candidates (behavior-preserving only — ranked)

From the reviewer gate. Map-and-clean campaign: **no features** (no new ops, no
extrude). Status tracks this campaign's builder batch.

| # | value | candidate | cheapest fix | this campaign? |
|---|---|---|---|---|
| 1 | MED–HIGH | `estimateBounds` non-exhaustive (5/10 ops, silent fall-through) | convert to a `std::visit` overload set; no-op arms for draw-nothing ops; same extents | **YES — slice A** |
| 2 | MED | store READER if-ladder not compiler-exhaustive | `static_assert(variant_size_v<RecipeOp> == 10)` + teaching comment beside the ladder | **YES — slice B** |
| 3 | MED | charter says reader/resolve are exhaustive visits (they aren't) | record the corrected map (this doc) + a charter note | **YES — done in this doc; charter note** |
| 4 | LOW–MED | dry-run `plan_lines` emits no Script line (header miscounts) | add a Script branch | deferred — touches dry-run *behavior*; folds with the backlog Script-ASCII work |
| 5 | LOW | craftsman `param.type` default: C++ "text" vs Python "number" | align C++ fallback / document | deferred — only reachable on hand-built registry TOML |
| 6 | LOW | `estimateBounds` omits AddLabel (Python `bounds_of` includes it) | comment the divergence (no behavior change) | folds into slice A as a comment |

No data-oriented-rule violations found: no subclassing-for-behavior, no stateful logic
objects, no hidden JSON, no `.js`/`.qml` in scope. The variant + free-function +
member-pointer-table dispatch is consistent throughout.
