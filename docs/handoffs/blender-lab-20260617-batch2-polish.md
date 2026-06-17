# Handoff — blender-lab-20260617-batch2-polish

> Batch-2: the 8 deferred recipe-lab polish items (from
> `docs/closeouts/blender-lab-feature-batch.md`) + remaining roadmap depth.
> Rebased on master first (the feature batch is merged). Build the OP/Python verbs;
> chrome gates via ui-integration (designing in parallel). Commit to `dept/blender-lab`;
> do NOT merge or touch LEDGER. bus-hub progress + a closeout when done.

- **Campaign**: blender-lab-20260617-batch2-polish
- **Green gate (per slice):** `cmake --build build && ctest --test-dir build -E
  edi_shell_window_tests --output-on-failure` + scan + the `edi_craft` cross-language
  checks. Rebase on master at the START of each slice. **Pre-existing goldens stay
  byte-identical** unless a slice deliberately changes one (then regenerate + verify).

## Plan (prioritized slices)

| # | slice | source | risk | gate |
|---|---|---|---|---|
| P1 | sweep×screw silent-override validate-WARNING (#2) + edge-incidence manifold assert in smoke (#3) | C++ validate + Python smoke | low | spot-check |
| P2 | boolean proof-duplication: suppress consumed-operand standalone emission (#5) | Python obj_objects (changes boolean golden) | med | reviewer |
| P3 | watertight helix — end caps + closure (#1) | Python helix mesh (smoke-pinned) | med | reviewer |
| P4 | per-axis taper on the sweep (#7) | field-add + mesh | med | reviewer |
| P5 | sweep miter at path corners (#6) | Python sweep mesh geometry | high | reviewer |
| P6 | straight-skeleton (or improved) inset for non-convex (#8) | Python geometry | high | reviewer |
| P7 | bounds_of arc/helix/sweep tightness (#4) | Python bounds_of (cosmetic) | low | spot-check |
| RD1 | ScriptOp ASCII bbox proof (roadmap M1) | C++ RecipeOpsAscii + test | low–med | reviewer |
| RD2 | recipe semantic diff → TOON (roadmap M6 slice 2) | C++ free fn + test | med | reviewer |
| RD3 | docs/craftsmen-authoring.md (roadmap M4 teaching doc) | **researcher** (docs, parallel) | n/a | — |

Order: P1 (cheap wins) + RD3 (researcher, parallel) → P2 → P3 → P4 → RD1 → RD2 → P5/P6
(hard geometry) → P7. Re-prioritize on findings. Most are NON-chrome (recipe/Python
internals); RD2's CLI verb is edi-ui's (flag).

## Constraints (carried from batch-1)
- No new C++ source/test FILES (CMakeLists.txt is edi-ui's) — extend existing recipe
  files. No LEDGER/CMakeLists/shell edits. No JSON. Every default byte-preserving.

## Gate log
### Rebase — 2026-06-17 — planner
Rebased `dept/blender-lab` onto master (b5f9d86, carries dungeon-map DM-01..15). Clean,
30 ahead. Full gate green: build clean, ctest 101/101, 4 OBJ goldens byte-identical.

### P1 — builder briefed 2026-06-17 (in flight)

### RD3 — SHIPPED 2026-06-17 (`09cb3bb`, docs-only)
`docs/craftsmen-authoring.md` — the M4 teaching doc (three-part contract, pure proof_mesh,
manifest-typed params, scan flow, radial_petal walk-through, sacred-geometry intent,
gotchas). Cited to the real craftsmen + arch doc. **Researcher flags integrated:** fixed
the stale arch §6 "only twisted_column on disk" → three craftsmen + the authoring-guide
link. The param.type default mismatch (C++ "text" vs Python "number") stays §10 candidate
#5 (LOW, unreachable in practice; the doc now tells authors to always declare `type`).

## Next
- (RD3 done) P1 builder in flight → integrate → P2 (boolean dedup) …
