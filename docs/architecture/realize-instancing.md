# Realize-by-instancing — design / de-risk doc

Campaign: replace the realizer's **regenerate-per-piece** greybox with
**build-once / instance-many**. Today `edi_realize.plan_greybox` + `build_scene`
rebuild a fresh `primitive_cube_add` / `primitive_cylinder_add` for every piece
(`tools/blender/edi_realize.py`). The forge `edi_craft.build(ops)` builds a
recipe's mesh in bpy but has **no step that saves the built mesh to a reusable
artifact** — `--obj-out` only writes a flat OBJ proof. This doc resolves the
three open questions so the builders can wire the seam without re-deriving the
bpy facts.

Scope of this doc: research + recommendation only (no code). Each recommendation
names the **exact bpy calls** and the **exact TOON column layout**, separates
**verified-from-docs** facts from **asserted/derived** reasoning, and ends with
an implementation map keyed to the campaign slices and a risks list.

The three identity laws this campaign must not break:
1. **Recipe is truth / ASCII is proof / Blender is execution.** The forge bakes;
   the realizer instances; neither invents geometry.
2. **No JSON. TOON for the C++→Python handoff.** The zoo's on-disk format stays
   MessagePack (`AssetZooSerialize`), but Python never reads MessagePack.
3. **No hardcoded dimensions.** Every dimension that survives into new realizer
   code is named DATA (the existing `GreyboxDims` table is the model).

---

## Q1 — Build-once, place-many in bpy

### Recommendation
**Share one mesh datablock across N objects via `bpy.data.objects.new(name,
mesh_data)`** — technique (a). Build (or load) the mesh **once** into
`bpy.data.meshes`, then create one lightweight `Object` per placement that all
point at that same `Mesh` (`obj.data is mesh`). Per-placement transform lives on
the `Object` (`location` / `rotation_euler` / `scale`); the heavy vertex/face
buffer exists exactly once.

Collection instancing (b) is the **runner-up** and is the right tool only for a
*multi-object* asset (an asset that is itself several meshes — e.g. a sarcophagus
+ its lid + a chain). `libraries.load(link=True)` (c) is the *delivery* mechanism
for getting the datablock in from the forge's `.blend` (see Q2); it is
orthogonal to (a) — you link the mesh once, then instance it with (a).

### Why (a) wins
- **Memory.** N shared-data objects store the geometry once. The `Object` struct
  (matrix, name, slot list) is tiny next to the `Mesh` vert/edge/loop/poly
  buffers. This is exactly Blender's "linked duplicate" (Alt+D) at the data
  level. *Verified:* an object's `data` can be assigned a shared mesh datablock;
  multiple objects referencing one mesh is the documented optimization
  ([blenderartists thread on creating instances via the mesh datablock]; the
  Blender manual's Object Duplicate page describes linked duplicates sharing
  geometry).
- **Cycles final-frame render.** Cycles already deduplicates identical mesh data
  internally, but *what we control* is the scene graph: N objects with one mesh
  render with per-object transforms and (with the material caveat below)
  per-object materials. *Asserted (load-bearing):* per-instance
  location/rotation/scale are object-level transforms and are respected by Cycles
  the same way the current `build_scene` already sets `obj.location` /
  `obj.rotation_euler` / `obj.dimensions` per piece — instancing changes only
  *who owns the geometry*, not how the transform is applied.
- **Lowest blast radius.** `build_scene` already does the per-piece transform
  dance. Instancing is a narrow change: build/lookup the datablock once into a
  cache dict keyed by `meshRef`, then `bpy.data.objects.new` instead of
  `primitive_*_add` + `dimensions`.

### The exact bpy calls (asserted shape, standard bpy idiom)
```
# once per distinct meshRef (cache it):
mesh = _load_or_build_mesh(meshRef)          # see Q2 — libraries.load(link=True)
cache[meshRef] = mesh

# per placement:
obj = bpy.data.objects.new(name=f"{asset_id}_{i}", object_data=cache[meshRef])
obj.location       = (x, y, z)
obj.rotation_euler = (0.0, 0.0, math.radians(rot_deg))
obj.scale          = (s, s, s)               # per-instance scale from the wire
bpy.context.collection.objects.link(obj)     # link the OBJECT into the scene
```
Note the two distinct meanings of "link": `libraries.load(link=True)` links an
*external datablock into this file* (Q2); `collection.objects.link(obj)` puts an
*object into the scene's collection* (every object needs this to render — the
current code uses `bpy.context.collection.objects.link(light_obj)` already).

### Per-instance materials — the one caveat (verified)
A `MaterialSlot` has a `link` enum: **`'DATA'`** (default — material lives on the
shared mesh, so all instances share it) vs **`'OBJECT'`** (material lives on the
object, per-instance override). *Verified:* `MaterialSlot.link` is `'OBJECT'` |
`'DATA'`, default `'DATA'`
([bpy MaterialSlot docs](https://docs.blender.org/api/current/bpy.types.MaterialSlot.html)).

Implication for the realizer: the **base material** baked with the asset rides on
the mesh data (shared, free). When a placement needs a *different* material (the
`textureRefs` override per `AssetRecord`, or the brazier's bronze vs a recolor),
set that slot's `link='OBJECT'` and assign the override on the object — the shared
mesh is untouched. If every placement uses the asset's baked material, do
nothing; sharing is automatic.

### What loses
- **Re-importing OBJ per placement** (today's spiritual equivalent of
  regenerate-per-piece): `import_scene.obj` creates a *fresh* mesh datablock each
  call. N imports = N geometries = defeats build-once. Rejected.
- **Collection instancing for single-mesh assets:** an empty-with-`instance_collection`
  is heavier than needed (extra empty + collection indirection) and complicates
  per-instance material override (the override would have to reach into the
  instanced collection). Keep it in reserve for genuinely multi-object assets.

---

## Q2 — `meshRef` artifact format (what the forge bakes)

### Recommendation
**Bake each asset to its own `.blend` file** (one mesh datablock per file, named
deterministically), and have the realizer pull it in with
`bpy.data.libraries.load(filepath, link=True)`. `meshRef` is the **path (or
catalog-relative key resolving to a path) of that `.blend`**, optionally with a
datablock name. This is the only format that delivers a *linkable / appendable
datablock* — i.e. build-once survives.

### Why `.blend` wins over OBJ / glTF
- **OBJ import always copies.** `import_scene.obj` parses text into a brand-new
  `Mesh` every call. There is no "instance this OBJ" — so OBJ as `meshRef` forces
  regenerate-per-placement, the exact thing we are removing. OBJ stays valuable
  as the **deterministic proof tier** (the forge's `--obj-out`, the realizer's
  `pieces_to_obj`), not as the instancing artifact.
- **glTF** is a fine interchange format but, like OBJ, importer-creates fresh
  datablocks and adds a dependency/translation layer with no upside here (we are
  Blender→Blender, not Blender→engine).
- **`.blend` is Blender-native and linkable.** `bpy.data.libraries.load` reads a
  `.blend` and gives you the *actual datablock*; with `link=True` it stays a
  single shared instance backed by the external file — the cleanest possible
  build-once. *Verified:* the documented `libraries.load` pattern is
  ```
  with bpy.data.libraries.load(filepath, link=True) as (data_from, data_to):
      data_to.meshes = [name for name in data_from.meshes if name == wanted]
  # after the with-block, data_to.meshes holds the loaded Mesh datablocks (or None)
  ```
  ([bpy BlendDataLibraries docs](https://docs.blender.org/api/current/bpy.types.BlendDataLibraries.html);
  load example confirmed on the API-examples mirror).

### The forge's new `--asset-out=<path>` step (calls, not code)
`edi_craft.main` gains `--asset-out=<path.blend>` parallel to `--obj-out`. When
present it must, **inside Blender**:
1. `build(ops)` — the existing in-memory bpy build (already produces named
   objects + their mesh datablocks).
2. Collect the datablocks to persist: the built mesh(es) — `set(bpy.data.meshes)`
   filtered to what `build` created, plus the materials they reference if the
   baked-material-rides-on-mesh path is wanted.
3. Bake to a `.blend` with the **library writer**:
   ```
   bpy.data.libraries.write(filepath, data_blocks, fake_user=True)
   ```
   *Verified:* `bpy.data.libraries.write(filepath, datablocks, fake_user=True)`
   writes the given datablocks to a `.blend`; `fake_user=True` keeps an
   otherwise-unreferenced asset mesh from being purged
   ([bpy BlendDataLibraries docs](https://docs.blender.org/api/current/bpy.types.BlendDataLibraries.html)).
   - Alternative if a *whole-file* asset is wanted (camera/lights stripped):
     `bpy.ops.wm.save_as_mainfile(filepath=...)`. `libraries.write` is preferred
     because it persists *only* the datablocks we name — no preview rig leaking
     into the asset.
4. **No hardcoded dimensions** rule applies: the `--asset-out` path adds no new
   dimension literals — it only persists what `build(ops)` already produced from
   the recipe's named ops.

Optional polish (later, not required for the seam): call
`mesh.asset_mark()` so the baked `.blend` shows in Blender's Asset Browser. Not
needed for `libraries.load` instancing; purely an authoring nicety.

### `meshRef` naming
Keep `meshRef` the **opaque string** it already is in `AssetRecord` (the catalog
"index, not geometry" law). Recommended convention: `meshRef` = the `.blend`
basename or a catalog-relative path; the realizer resolves it to an absolute path
the same way it resolves the TOON map path. The datablock name inside the `.blend`
should be derived from the asset id so the `libraries.load` filter is exact.

---

## Q3 — The catalog→realizer bridge (TOON zoo-manifest)

### Recommendation
**Yes — a neutral TOON zoo-manifest, mirroring `MapToonExport`, is the correct
bridge.** A pure-C++ string export of the curated `AssetRecord`s that the Python
realizer parses with the **same grammar** as `parse_toon`
(`name[count]{cols}:` + indented CSV rows, `cell()` quoting, `·`-joined lists).
Python stays TOON-only; it never learns MessagePack.

### Why this (and why the alternative loses)
- **Reading MessagePack in Python is the rejected alternative.** It would force a
  second codec into the Python side, couple Python to the binary envelope
  (`EDIM`), and break the "TOON for the C++→Python handoff" law. Rejected.
- **The grammar already exists and is proven.** `parse_toon` in
  `edi_realize.py` already parses `rooms`/`plugs`/`connections`/`nodes`/`blocks`
  by *column name from the header* (invariant a in `MapToonExport.cpp`). A new
  `assets[N]{...}:` section drops straight into that parser — add one `elif
  section == "assets":` arm; the cell-splitting, quoting, and `·`-list handling
  are reused verbatim.
- **Boundary law holds.** The manifest exporter is **pure-string over zoo
  structs** — it touches only `AssetRecord` / `AssetSocket` / `Anchor2D` and
  `std::string`/`std::ostringstream`. It has **no drafting and no recipe
  dependency**, so `edi_zoo_core` can host it (e.g. a new `ZooToonExport.{h,cpp}`)
  without violating core isolation — exactly the way `MapToonExport` is a pure
  projection over `MapSpec`/`DraftingDocument`. The bridge lives at the *export*
  layer, not in the zoo data structs themselves.
- It reuses the same `cell()` quoting discipline and the `·` (U+00B7 MIDDLE DOT)
  list join already used by `joinFlags` for plug flags, so `textureRefs` and
  sockets encode by an idiom both sides already implement.

### Proposed TOON column layout
A single `assets[N]{...}:` section. Two list columns use the `·` join
(`textureRefs`; and sockets serialized as a compact run). Coordinates inside a
cell carry a comma so they are `cell()`-quoted, exactly like `origin`/`anchor`
elsewhere.

```
assets[N]{id,name,category,meshRef,proxyRef,textures,sockets}:
  asset_0001,crypt_wall,wall,crypt_wall.blend,blk_wall_2d,"stone·moss","door@2,0·edge@5,0"
  asset_0002,sarcophagus,prop,sarcophagus.blend,,marble,
```

Column-by-column:
| column | source field | encoding |
| --- | --- | --- |
| `id` | `AssetRecord.id` | bare token (`asset_0001`) |
| `name` | `AssetRecord.name` | `cell()`-quoted if it has a space |
| `category` | `AssetRecord.category` | bare open-vocab token |
| `meshRef` | `AssetRecord.meshRef` | the `.blend` key/path (Q2); `cell()`-quoted if spaced |
| `proxyRef` | `AssetRecord.proxyRef` | may be empty → empty cell |
| `textures` | `AssetRecord.textureRefs` | `·`-joined run (like `joinFlags`); empty list → empty cell |
| `sockets` | `AssetRecord.sockets` | `·`-joined run of `name@x,y` (or `name:type@x,y`) per socket; whole cell is `cell()`-quoted because it contains a comma |

Notes on the two list columns:
- **`textures`**: identical idiom to plug `flags` — `texA·texB`, empty → `""`.
- **`sockets`**: each socket is `name@x,y` where `x,y` is the `Anchor2D`. If the
  socket `type` is needed downstream, use `name:type@x,y`. The run is `·`-joined;
  because each element contains a comma, the *whole* sockets cell is quoted (one
  cell, the realizer un-quotes then splits on `·` then on `@`/`:`). This mirrors
  how `MapToonExport` quotes the whole `origin` cell because it carries a comma.
- **Curated-only:** the exporter emits **only** records with `curated == true`
  (the zoo's greenlight), so `N` is the curated count, not `zoo.assets.size()`.

### Conditional-emission discipline (inherit from MapToonExport)
Follow invariant (b): if no curated asset has any socket, the `sockets` column may
be omitted from the header (and likewise `textures`), keeping an all-default
manifest minimal and byte-stable. Simpler first cut: always emit all columns (the
manifest is a new section, not a backward-compat extension of an existing one, so
byte-identity with a legacy export is not at stake). Recommend **always-emit all
columns** for the first slice; add conditional columns only if a real need
appears.

### The greybox fallback law (must survive)
The manifest is **additive** to the existing realizer. A `blocks[]` / piece whose
`asset_ref` does **not** resolve to a manifest `meshRef` MUST fall back to the
current `plan_greybox` primitive (the crypt sample has no manifest yet and must
still render). Concretely: the realizer loads the manifest (if a
`--manifest=<zoo.toon>` arg is present), builds a `{asset_ref → meshRef}` map, and
in `build_scene` chooses **instance the datablock** when the ref resolves, else
**fall back to the existing `primitive_*_add` greybox** path. No manifest arg ⇒
pure greybox, byte-identical to today.

---

## Implementation map (which finding feeds which slice)

| slice | what it builds | findings it consumes |
| --- | --- | --- |
| **R1a — manifest export** | new pure `edi_zoo_core` exporter `exportZooToToon(const AssetZoo&)` → TOON `assets[N]{...}:` (curated only); add the matching `elif section == "assets":` arm to `parse_toon` | Q3 (column layout, `·`/`@` encoding, curated filter, boundary law, conditional-emission) |
| **R1b — forge bake** | `edi_craft.main` gains `--asset-out=<path.blend>`; runs `build(ops)` then `bpy.data.libraries.write(path, {meshes,materials}, fake_user=True)` | Q2 (`.blend` over OBJ/glTF, the write call, no new dimension literals) |
| **R2a — realizer plan** | thread an optional `--manifest=<zoo.toon>` into both entry points; parse it; build `{asset_ref → meshRef}`; tag each `Piece` as instanced-or-greybox; keep `plan_greybox` fallback intact | Q3 (parse arm) + Q1 (the plan must record *which* meshRef to instance) + fallback law |
| **R2b — bpy instance** | in `build_scene`, cache `meshRef → Mesh` via `libraries.load(link=True)`, create placements with `bpy.data.objects.new(name, mesh)` + per-instance transform; `link='OBJECT'` only for material overrides; greybox path unchanged for unresolved refs | Q1 (shared-datablock calls, transform, material-slot caveat) + Q2 (`libraries.load` delivery) |

Suggested order: **R1b** (forge can bake an artifact) and **R1a** (manifest can
name it) are independent and can land in either order; **R2a** depends on R1a's
parse shape; **R2b** depends on R2a's plan tag and on R1b producing a `.blend` to
load. The greybox fallback means each slice is shippable without breaking the
crypt render.

---

## Risks / open questions

1. **Per-instance material via `link='OBJECT'` — verify in a real render.** The
   `MaterialSlot.link` enum is documented, but the exact sequence to *add* an
   object-level slot and override it on a shared-data object should be smoke-
   tested on the GPU before R2b is called done (a wrong material claim renders
   wrong but silently). *Status: asserted from docs, not yet rendered here.*

2. **`libraries.load(link=True)` and relative paths.** Linked data records the
   external `.blend` path; if the asset library moves, links break. Decide early
   whether the realizer **links** (light, but path-fragile) or **appends**
   (`link=False`, copies the datablock once into the session — still build-once
   *within a render*, path-robust). For a one-shot headless render, **append**
   may be safer; **link** is better if many renders share a library. *Open
   decision for R2b.* Both are the same call with `link` flipped.

3. **What exactly does `build(ops)` leave in `bpy.data.meshes`?** R1b must scope
   the datablock set precisely (the recipe's meshes, not the preview rig's camera
   /light/leftover). Confirm `build` names its meshes predictably (it does:
   `op["name"] + "_mesh"` for lofted/moulding/prism paths; primitive paths get
   the object name) so the writer can filter by name. *Verify when wiring R1b.*

4. **One mesh per asset vs multi-mesh assets.** Q1(a) assumes one datablock per
   `meshRef`. A recipe that builds several objects (a `AddBoolean` result, or a
   prop + sub-parts) yields multiple meshes. First cut: **join into one mesh**
   before bake (or pick the named root), so `meshRef` → one datablock. If
   genuinely multi-part assets are needed later, that is when collection
   instancing (Q1 runner-up) earns its place — `libraries.load` the collection,
   instance via an empty's `instance_collection`. *Defer; note the seam.*

5. **`Object.scale` vs `Object.dimensions`.** Today `build_scene` sets
   `obj.dimensions` (absolute size) then applies scale. With a *shared* mesh you
   must NOT bake scale into the mesh (that would mutate the shared datablock for
   all instances) — use `obj.scale` (a per-object transform) and never
   `transform_apply(scale=True)` on a shared-data object. *Load-bearing for
   R2b: applying scale to shared data corrupts every instance.*

6. **Catalog path resolution.** `meshRef` is opaque; the realizer needs a base
   directory to resolve `.blend` paths. Propose a `--asset-dir=<dir>` realizer arg
   (named DATA, no hardcoded path) alongside `--manifest=`. *Open for R2a's CLI.*

---

## Sources

- [bpy BlendDataLibraries — `libraries.load` / `libraries.write`](https://docs.blender.org/api/current/bpy.types.BlendDataLibraries.html) — the link/append load pattern and the `write(filepath, datablocks, fake_user=True)` bake call.
- [bpy MaterialSlot](https://docs.blender.org/api/current/bpy.types.MaterialSlot.html) — `link` enum `'OBJECT'` (per-instance override) vs `'DATA'` (shared on mesh, default).
- [Blender Artists — creating an instance via the mesh datablock](https://blenderartists.org/t/create-an-instance-of-an-object-using-mesh-data-block/544822) — multiple objects sharing one `obj.data` mesh datablock (the linked-duplicate technique at the API level).
- [Blender manual — Collection instancing](https://docs.blender.org/manual/en/latest/scene_layout/object/properties/instancing/collection.html) — the runner-up technique for multi-object assets (empty + `instance_collection`).
- Repo source read for the contract: `tools/blender/edi_realize.py` (`parse_toon`, `plan_greybox`, `build_scene`), `tools/blender/edi_craft.py` (`build`, `main`/`--obj-out`), `src/io/MapToonExport.cpp` (the TOON grammar + invariants), `src/zoo/AssetZoo.h` (`AssetRecord`), `src/zoo/AssetZooSerialize.h` (MessagePack store the bridge deliberately bypasses).
