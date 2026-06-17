# Authoring a custom craftsman

> A teaching doc for the recipe lab (Seam A). This is a C++/Python **learning**
> codebase, so this explains the **why**, not just the steps. Anchored to the
> charter (`docs/departments/edi-blender-lab.md`) and the architecture map
> (`docs/architecture/edi-blender-lab.md` §1, §3, §5, §6).

A **craftsman** is one Python file you drop into `tools/blender/craftsmen/`. It adds
a recipe step the built-in op vocabulary cannot express — a twisting column, a
flower of petals, an {n/k} star — and it appears in the lab's palette with **no C++
change and no recompile**. This doc shows the contract, the reasoning that makes the
contract safe, and one craftsman walked end to end.

The doctrine the whole lab obeys applies to your craftsman too:

> **Recipe is truth. ASCII preview is proof. Blender script is execution.**

A craftsman's shape has no 2D ASCII silhouette — its proof tier is the **OBJ mesh**
(`--obj-out`). So your job is to make the *offline* mesh real, and let Blender be a
faithful twin of it.

---

## 1. The three-part contract

A craftsman is a module exposing exactly three names. The scanner
(`load_craftsmen`, `tools/blender/edi_craft.py:84`) imports every non-`_`-prefixed
`*.py` in the folder and keeps the ones with a `MANIFEST` carrying an `id`.

```python
import math

MANIFEST = {
    "id": "my_craftsman",                 # stable key — the ScriptOp.scriptId
    "label": "My Craftsman",              # palette button text
    "params": [
        # type is one of: number | integer | material
        {"key": "radius", "label": "Radius", "type": "number",   "default": 1.0},
        {"key": "sides",  "label": "Sides",  "type": "integer",  "default": 6},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}

def _local_mesh(params: dict):
    radius = float(params.get("radius", 1.0))      # coerce: params arrive as strings
    sides  = max(3, int(float(params.get("sides", 6))))  # coerce + clamp degenerate
    verts, faces = [], []
    # ... pure closed-form geometry, no randomness ...
    return verts, faces

def proof_mesh(op: dict):
    """PURE (verts, faces). No bpy. Placed at the op's x/y/z."""
    verts, faces = _local_mesh(op.get("params", {}))
    x, y, z = float(op.get("x", 0.0)), float(op.get("y", 0.0)), float(op.get("z", 0.0))
    return [(vx + x, vy + y, vz + z) for vx, vy, vz in verts], faces

def build(op: dict):  # pragma: no cover — exercised only inside Blender
    import bpy
    verts, faces = proof_mesh(op)                  # SAME mesh as the proof
    name = op.get("name", op.get("script", "my_craftsman"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
```

That `build` body is **boilerplate** — all three shipped craftsmen use it
verbatim (`twisted_column.py:66`, `radial_petal.py:87`, `nfold_star.py:95`). The
only file that ever differs is `_local_mesh`. Copy an existing craftsman and
replace the geometry.

---

## 2. Why `proof_mesh` is PURE and bpy-free

This is the load-bearing rule. `proof_mesh` must compute `(verts, faces)` using
nothing but Python + `math` — **never `import bpy`**.

**Because the proof tier runs without Blender.** The OBJ proof
(`obj_objects` → `_mesh_for`, `edi_craft.py:1050`, `:1028`) calls your
`proof_mesh` directly and writes vertices to an `.obj`. That is how a *custom*
step gets proven exactly like a *built-in* one — offline, deterministic, no
Blender process, no GPU. The architecture doc records this as the Script op's
proof tier: it is invisible in the 2D ASCII and dry-run tiers and **OBJ-only**
(`docs/architecture/edi-blender-lab.md` §5).

**And `build` builds from the SAME mesh, so proof and Blender cannot drift.**
Notice `build` does not re-derive geometry — its first line is
`verts, faces = proof_mesh(op)`. The only Blender-specific code is handing those
verts/faces to `bpy.data.meshes`. That is marked `# pragma: no cover` because the
headless test suite never launches Blender — but it doesn't *need* to: the proof
already validated the exact mesh Blender will receive. If `build` recomputed the
shape, a headless-green proof could ship a different mesh into Blender. Routing
both through one `proof_mesh` makes that drift **structurally impossible**, the
same discipline §3 of the arch doc enforces across the C++/Python TOML seam.

---

## 3. Why a param's type lives in the MANIFEST, not in C++

The C++ side carries a craftsman's params as `ScriptParam{key, value}` — an
**untyped bag of strings** (`RecipeOps.h`; arch doc §1 row `ScriptOp`,
§6). There is one `ScriptOp` arm in the `RecipeOp` variant, and it serves
*every* craftsman that exists or ever will. C++ has no per-craftsman code: no
`TwistedColumnOp`, no arm to add, no recompile.

So where does "radius is a number, sides is an integer" live? **In the
manifest.** `param.type` is declared in Python and read back as a plain string by
`parseCraftsmanRegistryToml` (`src/recipe/RecipeCraftsmen.cpp:7`). The inspector
uses it to pick a field widget; the value still travels as a string. Then **the
craftsman coerces at the edge** — `float(...)`, `int(float(...))` — at the top of
`_local_mesh`.

This is the data-oriented win the charter calls for: **variation points are
data, not subclasses.** One generic `ScriptOp` arm, plus a manifest that *is* the
type schema, means an infinite family of craftsmen with zero new C++ dispatch.
Compare the built-in ops, where each new shape is a new variant arm forcing
every `std::visit` interpreter to grow (arch doc §1–§2). A craftsman pays none of
that — the price is that **the craftsman owns its own type discipline** (the
coercion and clamping in §7).

> Why `int(float(x))` and not `int(x)`? The value arrives as a string like
> `"6"` — but a manifest default of `4` or a bound measurement can present as
> `"4.0"`, and `int("4.0")` raises. `float()` first parses any numeric string,
> then `int()` truncates. All three craftsmen do this (`twisted_column.py:33`).

---

## 4. The scan / registry flow — no C++ change, no recompile

Dropping a file is the entire install:

1. **Drop** `my_craftsman.py` into `tools/blender/craftsmen/`.
2. **Scan** — `load_craftsmen` (`edi_craft.py:84`) imports it and registers it by
   `MANIFEST["id"]`.
3. **Surface** — `--list-craftsmen` (wired from `app/main.cpp`, an edi-ui host
   file) asks Python for `craftsmen_manifest_toml` (`edi_craft.py:104`), which
   emits flat TOML: `craftsman.i.id/label`, `craftsman.i.param.j.key/label/type/default`.
4. **Read** — C++ `parseCraftsmanRegistryToml` (`RecipeCraftsmen.cpp:7`) walks
   that same `craftsman.N` / `param.N` index-run and builds `CraftsmanManifest`s.
5. **Offer** — the palette shows your label; picking it calls `makeScriptOp`
   (`RecipeCraftsmen.cpp:71`), which seeds a `ScriptOp` with one param per
   manifest entry **at its declared default**.

The manifest TOML uses the same flat `key.N.field` index-run shape as the op
store reader — the file's own ordering *is* the registry's ordering, so a palette
row is a manifest row (`RecipeCraftsmen.cpp:24` comment). Nothing here knows what
`my_craftsman` *does*; it only knows its shape from the manifest.

---

## 5. Worked walk-through — `radial_petal`

A rose-window bloom: N petals arrayed around a hub. (`tools/blender/craftsmen/radial_petal.py`.)

**Manifest** (`:21`) declares five dialable params + a material: `petals`
(integer), `petalLength`, `petalWidth`, `centerRadius`, `zRise` (numbers).

**Geometry** (`_local_mesh`, `:35`) is pure closed-form trig:

- Vertex 0 is the hub center. Each petal then contributes a **4-vertex kite**: a
  base on the hub ring, two mid-flanks half-way out, and a lifted tip.
- The petal's real width is set at the mid radius by converting `petalWidth` into
  an **angle**: `half_angle = (petalWidth * 0.5) / mid_radius` (`:50`). This is the
  arc-length↔angle relation `s = rθ` — width is a chord, so dividing by the radius
  it sits at gives the angle that yields that width. Dialing `petalLength`
  therefore *automatically* keeps a constant-width petal looking right, because
  `mid_radius` moves with it.
- Faces: a **hub fan** of triangles from vertex 0 to consecutive petal bases
  (wrapping shut), then one quad **kite per petal** (`:65`).

**How it renders in `--obj-out`:** `obj_objects` hits the `Script` branch
(`edi_craft.py:1076`), looks up `radial_petal` in the registry, calls
`proof_mesh(op)`, and writes `o <name>` + `v`/`f` lines into the OBJ. You can prove
a bloom with no Blender at all:

```
python3 tools/blender/edi_craft.py --obj-out=/tmp/bloom.obj <compiled.toml>
```

The same verts/faces are what `build` later hands Blender — so the OBJ you eyeball
is the asset you get.

---

## 6. The sacred-geometry intent — TOOLS, not generators

The three shipped craftsmen are not decoration; each serves a figure the author
*composes with*, and each is a parametric **tool the author dials**, never a
generator. The mandate (recorded as the batch stop-line,
`docs/closeouts/blender-lab-feature-batch.md`: *"The two craftsmen are parametric
TOOLS, not randomizers… No generation/auto-layout"*):

| craftsman | figure it serves | the move |
|---|---|---|
| `twisted_column` | the twisting shaft | an N-gon cross-section rotated continuously up its height — a column the built-in lathe (a *surface of revolution*) cannot make, because a twist is not revolution. |
| `radial_petal` | the rose-window / flower bloom | N petals fanned around a hub — radial symmetry the author tunes by petal count, length, and lift. |
| `nfold_star` | the girih / {n/k} star prism | the classic compass-and-straightedge star polygon, extruded to a prism — `skip` (k) drives the point sharpness. |

The discipline that makes them tools and not generators: **every vertex is a
closed-form function of the params.** Nothing random, nothing auto-laid-out. The
author moves a slider and the geometry moves with it, predictably. That is the
whole reason `proof_mesh` can be the proof — a generator's output couldn't be
pinned to a golden.

---

## 7. Gotchas (the rules your craftsman must keep)

- **Param keys are flat bare keys.** A key must be letters / digits / `_` / `-`
  only, and must not collide with a built-in field (`type`, `script`, `name`,
  `x`, `y`, `z`). C++ enforces this at store-write, store-read, and validate via
  `recipeScriptParamKeyProblem` (`src/recipe/RecipeOps.cpp:47`). A `.` in a key
  would nest a TOML table and diverge the two readers — don't use one. (Arch doc
  §3.) `camelCase` is fine (`petalLength`); `petal.length` is not.

- **Clamp degenerate params inside the craftsman.** C++ carries params untyped
  and does *not* range-check them, so the craftsman is the last line of defense.
  Floor counts (`max(3, int(float(params.get("sides", 4))))`,
  `twisted_column.py:33`); floor radii away from zero
  (`max(1e-4, float(...))`, `radial_petal.py:39`). The richest example is the
  {n/k} **coprime guard** (`nfold_star.py:30`, `_resolve_k`): it clamps `k` into
  `[2, n-1]` and, if `gcd(k, n) != 1`, snaps to the nearest coprime — so a
  degenerate request like `{6/2}` lands on the closest *legitimate* star instead
  of an empty or self-crossing mesh. A craftsman must **never crash and never
  emit a zero-area mesh**, because the proof tier calls it directly.

- **Be deterministic.** No `random`, no auto-layout, no hidden global state. The
  proof, the OBJ golden, and the Blender build must agree byte-for-byte every
  run; randomness breaks all three and violates the mandate (§6).

- **Only the three known param types.** Declare `number`, `integer`, or
  `material` — the types the C++ inspector renders. (A type C++ doesn't know
  falls back to a plain string field; see the note below.) The shipped craftsmen
  stick to these three for a reason: the registry must round-trip through
  `parseCraftsmanRegistryToml` cleanly (`radial_petal.py:8` comment).

---

## Flagged while writing (for the planner — not fixed here)

Two pre-existing inaccuracies, recorded not touched (I write docs, not code):

1. **Stale arch-doc count.** `docs/architecture/edi-blender-lab.md:228` (§6)
   says *"only `twisted_column.py` on disk today"*. There are now **three**
   craftsmen on disk (`twisted_column`, `radial_petal`, `nfold_star`, added
   BL-12/13). The §6 prose predates the feature batch; the line should read three.

2. **`param.type` default mismatch (already on the backlog).** When a manifest
   param omits `type`, Python's `craftsmen_manifest_toml` defaults it to
   `"number"` (`edi_craft.py:116`) but C++ `parseCraftsmanRegistryToml` defaults
   the same missing key to `"text"` (`RecipeCraftsmen.cpp:47`). The two halves
   disagree on an under-specified manifest. This is **refactor candidate #5**
   (arch doc §10, deferred — LOW, only reachable on a hand-built registry TOML;
   all three shipped craftsmen declare every type, so it is never hit in
   practice). Authors: always declare `type` and you never meet it.
