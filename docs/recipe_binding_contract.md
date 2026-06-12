# Pipeline A binding contract (extracted record)

*R1-B01, 2026-06-11. Pipeline A — the shaper-grammar recipe
(`RecipeDocument`/`RecipeStore`/`resolveRecipe`) — is scheduled for
retirement in R1: the op pipeline absorbs its two crown jewels,
measurement bindings and drafted-profile references. This document is the
contract those features must carry over, extracted from the code while it
still exists, with every refusal wording recorded. The companion test pins
live in `tests/recipe_document_tests.cpp` (marked "contract pin") and
`tests/recipe_store_tests.cpp`; they must keep passing unchanged when the
resolve seam moves (R1-B03). Line numbers cite the tree at branch
`ui-restoration`, post-`a73a2c6`.*

## 1. The TOML key shapes

Flat quoted-string keys, like every edi document. Three value sources per
shaper parameter (writer: `recipeToToml`, `src/recipe/RecipeStore.cpp:29`;
reader: `recipeFromToml`, `:63`):

```toml
# literal — the typed number, lossless text (numberKeyText)
step.4.param.count.value = "20"

# measurement binding — BOTH keys or refusal; replaces .value entirely
step.0.param.size_x.object = "plank"
step.0.param.size_x.field  = "width"

# drafted-profile reference — one object id per profile-taking step
step.2.profile = "base_cove"
```

From the committed sample (`samples/doric_column/doric_column.toml`): the
column uses literals and three profile references (`base_cove`, `shaft`,
`echinus`); **no committed file exercises `.object`/`.field`** — the
binding syntax is proven by `tests/recipe_store_tests.cpp` round trips
instead.

Reader strictness (behaviors pinned in `recipe_store_tests.cpp:79-152`;
note the store tests pin these wordings at `find()` strength — a
paraphrase would pass — unlike the resolve wordings, which
`recipe_document_tests.cpp` pins verbatim with `==`):

- `.object` without `.field` (or vice versa) →
  `"<key>: a measurement binding needs both .object and .field"`
  (`RecipeStore.cpp:114`).
- `.value` together with a binding →
  `"<key>: has both a literal (.value) and a measurement binding
  (.object/.field)"` (`:121`) — refused, never resolved by precedence,
  because the file would show a number the build ignores.
- Omitted keys mean the shaper spec's default stands (`:141`; pinned by
  the omitted-defaults case in `recipe_store_tests.cpp`); the writer
  nevertheless writes every parameter so each number is pointable
  (`:47`, pinned at `recipe_store_tests.cpp:75`).
- Consumption sweep: unknown keys (`"unknown recipe key: <key>"`, `:189`),
  plural/typo'd prefixes, and gapped step indices
  (`"step indices must be contiguous; missing step.N"`, `:167`) all refuse
  by name.

## 2. Identity: how a binding names a drafted object

- `DraftingObjectId` is a plain `std::string`
  (`src/drafting/DraftingTypes.h:11`). Validity is **non-emptiness only**
  (`isValidDraftingObjectId`, `src/drafting/DraftingGeometry.cpp:246`).
  There is no object-level display name — `ObjectMetadata`
  (`DraftingTypes.h:151`) carries provenance fields plus role/tags and two
  scoped labels (`measurement.label`, `guideVisual.label`), none of which
  bindings consult. The id IS the human handle, which is why the sample's
  profiles read as words (`"shaft"`).
- Creation: canvas tools mint `prefix_NNNN` ids via
  `draftingObjectIdForSerial` (`src/drafting/DraftingDocument.cpp:64`,
  e.g. `circle_0007`) through the controller
  (`DrawingDocumentController.cpp:48`); loaders and tests may supply any
  non-empty string. Non-emptiness and uniqueness are enforced on the OP
  path only (`DraftingStore.cpp:87`, `DraftingCommands.cpp:92`) — the
  binary `.edidraw` LOAD path (`DraftingSerialize.cpp:573-585`) pushes
  objects in with no id validation and no duplicate check, consistent
  with §3's "load does not re-validate" contract. A corrupted file can
  therefore deliver empty or duplicate ids, and `findObject` resolves a
  duplicate to the FIRST match — first-match-wins is recorded semantics
  the migration must not accidentally change (or may deliberately fix,
  with a divergence note).
- **Rename does not exist.** No op anywhere changes an object's id after
  creation (verified: no `setObjectId`/`renameObject` in the tree). Ids
  are immutable for an object's lifetime, so a binding cannot be orphaned
  by rename — only by deletion.
- **Deletion leaves bindings dangling silently.** The recipe and the
  drawing are separate documents; nothing back-references bindings at
  delete time. The dangle is caught at the resolve meeting point, per
  parameter, with a named message (below). This is recorded behavior, not
  an endorsement — the op pipeline keeps the same meeting-point design
  (R1 standing decision #2), and a delete-time "bound by a recipe" warning
  remains open for a later phase.
- Persistence: `.edidraw` stores ids verbatim (MessagePack envelope);
  recipe TOML stores them verbatim. Cross-document integrity is purely
  by string equality at resolve time.

## 3. Resolution semantics

One resolve pass, one call site. `resolveRecipe(document, drafting, grid)`
(`src/recipe/RecipeDocument.cpp:359`) is called exactly once in the app,
inside File → Export Blender Python…
(`src/widgets/EdiShellWindowIo.cpp:280`), against the LIVE drafting
document and grid projection. Loading and editing a recipe never resolves;
a recipe with bindings is self-consistent without any drawing open.

**Failure isolation is per-parameter, not per-document.** A stale binding
fails its own `ResolvedParam` with a message (`ok=false`,
`resolved.ok=false`) while sibling parameters resolve normally
(`RecipeDocument.cpp:369-377`; pinned at
`recipe_document_tests.cpp:117-141`) — the UI can point at exactly which
binding broke. The hard stop is at EXPORT: `emitBlenderPython` refuses an
unresolved recipe with
`"recipe is not fully resolved; fix the stale bindings first"`
(`src/recipe/RecipeEmit.cpp:238-241`), then gates numbers (non-finite →
`"step N: <shaper>.<param> is not a finite number"`; fractional counts →
`"… must be a positive whole number"`, `:212-227`).

### Measurement fields (closed vocabulary)

`resolveMeasurement` (`RecipeDocument.cpp:223-270`). Physical scaling uses
the grid projection: `width`/`radius` scale along grid X, `height` along
grid Y, `length` is the physical distance of a line's endpoints.

| field | answers for | refusal otherwise |
|---|---|---|
| `width` | any object (bounds width) | — |
| `height` | any object (bounds height) | — |
| `length` | `LineGeometry` only | `"length needs a line"` (`:252`) |
| `radius` | `CircleGeometry`, `ArcGeometry` | `"radius needs a circle or arc"` (`:264`) |
| anything else | — | `"unknown measurement field: <field>"` (`:267`) |

Missing object, any field: `"object not found: <objectId>"` (`:235`).

Document-op gates upstream (pinned in `recipe_document_tests.cpp`):
`bindParamToMeasurement` refuses empty halves
(`"a binding names an object and a field"`, `RecipeDocument.cpp:194`);
`setParamLiteral` refuses non-finite (`"not a finite number: <param>"`,
`:155`).

### Profile references

`resolveStepProfile` (`RecipeDocument.cpp:326-355`) +
`profileSourcePoints` (`:285-318`). Only `needsProfile` shapers accept one
(`setStepProfile` refuses others: `"this shaper does not take a profile"`,
`:176`; empty id: `"profile object id is empty"`, `:179`).

The page-to-part convention (the lathe's foundation, comment at
`:320-325`): **page-left edge is the spin axis** (drafted x → physical
radius, grid-X scaling) and **page-bottom is z = 0** (drafted y points
down, so z = physicalHeight(1 − y)). Explicit, never inferred.

Sources and determinism: a line contributes its two endpoints; a polyline
its vertices (≥ 2 or refusal — the `.edidraw` load path does not
re-validate geometry, so this guards corrupted files); an arc samples at
64 segments per full circle with exact endpoints (`:301-313`) — the same
drafted arc always yields the same mesh, byte for byte.

Refusals (step-level, `profileOk=false`, fail the step like a stale
binding): `"no profile bound"` (`:334`),
`"profile object not found: <id>"` (`:339`),
`"profile needs a line, polyline, or arc"` (`:315`),
`"profile polyline needs at least two vertices"` (`:299`).

## 4. Shaper inventory → op-vocabulary mapping

The six A-grammar shapers (`shaperTable()`, `RecipeDocument.cpp:38-59`)
and where each goes when A retires (R1-B06):

| shaper | params | absorbing op construct | notes |
|---|---|---|---|
| `cube` | size_x/y/z, loc_z | `AddBox` with `z_mode="base"` | loc_z places by BOTTOM — exactly AddBox's base mode; size_x/y/z → width/depth/height |
| `cylinder` | radius, depth, loc_z | `AddCylinder` with `z_mode="base"` | depth → height |
| `lathe` | segments, loc_z + profile | **`AddRevolvedProfile` (new, R1-B04)** lowering to `AddMoulding` points at resolve | segments → vertices; loc_z → base_z; profile points = (radius, z) pairs per §3 |
| `radial_groove` | count, cutter_radius, depth, at_radius, z_from, z_to | `CutFlutes` — **partial; planner decision needed** | count→count, depth→depth, z_from/z_to→start_z/end_z map cleanly. **Gap:** A specifies the cutter explicitly (`cutter_radius` + `at_radius`); CutFlutes derives it from the TARGET (`edi_craft.py cut_flutes()`, :584-586): target radius = max(dimensions.x, dimensions.y) × 0.5 (bounding box), cutter_radius = max(0.02, radius × width_ratio × **0.5**), cutter centre at max(0.001, radius + cutter_radius − depth) — note the ×0.5, the 0.02 cutter floor, and the 0.001 offset floor. Same machine, different parameterization — B04/B06 must either extend CutFlutes with optional explicit-cutter fields or adopt this derivation, floors and all, as the surviving semantics. |
| `bevel` | width, segments | **NO equivalent — flagged gap.** | The op pipeline's polish is hardcoded per craftsman (`edi_craft.py polish()`: box 0.04, cylinder 0.035, ring 0.035, moulding 0.03, sphere 0.01). A recipe-controlled bevel needs a planner decision: per-op polish fields, a finish op, or deliberate loss. |
| `array` | count, offset_x | **NO equivalent — flagged gap.** | Linear repetition has no op. (The drafting canvas owns repeat/grid/radial arrays for 2D; nothing places repeated 3D parts.) |

The doric column sample uses cube/lathe/radial_groove only — the
acceptance benchmark (B06: same column, drafted numbers, one pipeline)
does not by itself force the two flagged gaps; they need explicit planner
disposition before A's code is deleted.

## 5. What the op pipeline must reproduce (the contract, distilled)

1. Three value sources per numeric field — literal, measurement binding
   (`.object` + `.field`), and (for profile-taking ops) a drafted-profile
   reference — in the flat key shapes of §1, with the reader refusing
   half-bindings, double sources, unknown keys, and gaps BY NAME.
2. Binding identity is the drafted object's immutable string id; the only
   dangle is deletion, caught at resolve, per parameter, with
   `"object not found: <id>"`-grade messages.
3. The closed field vocabulary and its exact refusal wordings (§3) — the
   pins in `recipe_document_tests.cpp` are the executable form.
4. The page-to-part profile convention: page-left = spin axis, page-bottom
   = z 0, grid-scaled, arcs sampled 64/circle deterministically.
5. Resolution as an explicit pass against the live drawing + grid;
   documents load and save with bindings intact; the proof/execution
   tiers refuse unresolved input (A enforced this at export; the op
   pipeline enforces it at compile/preview/export per R1 standing
   decision #3).
