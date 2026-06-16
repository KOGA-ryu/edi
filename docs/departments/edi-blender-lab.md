# Department charter — edi-blender-lab

Seam A: the recipe lab that turns drafted measurements into 3D assets. "Recipe is
truth. ASCII preview is proof. Blender script is execution." An op stream the
human composes by clicking and the AI edits as TOML, rendered by a Python
craftsmen library.

## Scope (what this department owns)
- `src/recipe/` — the op pipeline: the `RecipeOp` variant + `RecipeOpStream`,
  the strict TOML store (`RecipeOpsStore`), `RecipeOpsValidate`,
  `RecipeOpsResolve` (bindings + `AddRevolvedProfile` lowering), `compileRecipeOps`
  (profile mouldings), `RecipeOpsAscii` (the proof projections),
  `RecipeOpsBind` (the member-pointer binding registry), `RecipeOpSchema` (the
  inspector's scalar schema), `RecipeMouldings`, `RecipeMeasure`, and
  `RecipeCraftsmen` (the `--list-craftsmen` registry + `makeScriptOp`).
- `tools/blender/edi_craft.py` + `tools/blender/craftsmen/` — the Python
  craftsmen library: `parse_ops`, the OBJ/`--obj-out` mesh proof, the bpy build,
  and the scanned custom-craftsman scripts (each a `MANIFEST` + pure `proof_mesh`
  + `build`).
- The recipe-lab parts of the shell: the Blender workspace, the
  `Palette | Render | Compiled` + `Steps | Editor | ASCII Proof` panels, the
  Steps inspector / step palette / binding picker, and the `--list-craftsmen`
  wiring in `app/main.cpp`. (The shell HOST + theming is edi-ui's; you own these
  panels' content.)

Do NOT touch: the shell chrome/theming (edi-ui) or the drafting core
(edi-drafting) — but you CONSUME drafting measurements through recipe bindings.

## Architecture (the rules to obey)
The `RecipeOp` variant is the vocabulary core; **every `std::visit` over it is
exhaustive** — namer, store writer+reader, validate, resolve, ascii, bind,
schema (add the arm or it won't compile). The stream is TOML (`recipeOpsToToml`/
`recipeOpsFromToml`), never JSON; the C++ writer's shape must match
`edi_craft.parse_ops` key-for-key (the cross-language contract). Custom craftsmen
= the generic `ScriptOp` (id + placement + untyped param bag; param keys are
flat bare keys, enforced at read/write/validate via `recipeScriptParamKeyProblem`).
Data-oriented: free functions over plain structs, no subclassing. Division of
labor (locked): the human composes by CLICKING; the AI edits the TOML — both
mutate the one `m_opsStream`, kept in sync by `opsStreamChanged`.

## Read first
- Memory: `edi-blender-feature-vision` (the governing vision + current status).
- `docs/recipe_binding_contract.md`, `docs/handoff-2026-06-16.md`,
  `docs/project-map.md` (the lab tracks).
- The doric reference: `samples/doric_column/` (recipe, compiled, OBJ golden,
  ascii previews).

## Verify (the green gate for this department)
```
cmake --build build && ctest --test-dir build --output-on-failure   # incl. recipe_* tests
```
plus the scan. AND the cross-language checks (no Blender needed):
`python3 tools/blender/edi_craft.py --obj-out=/tmp/x.obj <compiled.toml>`,
`--list-craftsmen`, and `tests/edi_craft_smoke.py`. ASCII goldens live under
`samples/doric_column/previews`. Eyeball the lab offscreen:
`QT_QPA_PLATFORM=offscreen ./build/edi --workspace blender --ops-file <recipe> --snapshot /tmp/lab.png`.

## Backlog
`docs/project-map.md` (the lab tracks). Open lab follow-ups: an ASCII bbox for a
`Script` op (draws nothing today — OBJ is its proof tier), moulding-SEQUENCE
editing in the inspector (TOML-only), a frameless pop-out skin, and eyeballing the
R2 OBJ golden in a 3D viewer once.
