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

### P1 — SHIPPED 2026-06-17 (`730f6bc`, green-gate accepted)
sweep×screw `helix_ignores_partial_sweep` non-fatal Warning in `checkLatheParams` (both
ops); `assert_manifold` smoke helper applied to full+partial revolve + straight prism
(codifies BL-06's hand-check); helix ribbon + swept excluded (documented). ctest 101/101,
4 OBJ byte-identical, no golden change → green gate is the proof. Accepted.

### P2 — SHIPPED 2026-06-17 (`8ae59f3`, planner spot-check)
Suppress consumed-operand standalone OBJ emission (mirror bpy). boolean_op: 4 objects → 2
(only the tagged operands). Spot-checked: object list = the 2 tagged operands, golden
matches, tagged-operand coords byte-identical (faces just renumber −200), other 3 goldens
byte-identical, smoke ok. Suppress-both (a standalone `a` would be a misleading half-cut).
Chained-boolean handled (composites never suppressed). Accepted.

### P3 — NOT dispatched (HUB PAUSE 2026-06-17)
Paused by hub for a context swap BEFORE dispatching P3. No worker in flight. Resume point:
brief P3 (watertight helix — close the open helix ribbon with end caps + radial closure,
reuse BL-06's partial-revolve closing technique; then ADD the helix to P1's manifold
assert). Only the helix smoke mesh changes (no committed golden uses a helix — doric etc.
have screw_rise=0); the 4 OBJ goldens stay byte-identical. Then P4 → RD1 → RD2 → P5/P6 → P7.

### RD3 — SHIPPED 2026-06-17 (`09cb3bb`, docs-only)
`docs/craftsmen-authoring.md` — the M4 teaching doc (three-part contract, pure proof_mesh,
manifest-typed params, scan flow, radial_petal walk-through, sacred-geometry intent,
gotchas). Cited to the real craftsmen + arch doc. **Researcher flags integrated:** fixed
the stale arch §6 "only twisted_column on disk" → three craftsmen + the authoring-guide
link. The param.type default mismatch (C++ "text" vs Python "number") stays §10 candidate
#5 (LOW, unreachable in practice; the doc now tells authors to always declare `type`).

## RESUMED 2026-06-17 (fresh hub session, full fleet live)
- **Merged + rebased:** edi-ui merged my full batch (feature batch + batch-2 P1/P2/RD3) to
  master `88452bb` (HEAD `f8bed78` after a drafting merge). Rebased dept/blender-lab onto
  master ref → branch == master (`ef9bf0a`, 0 ahead). edi-gate GREEN (101/101 + scan).
- **New toolbelt in use:** `edi-gate` (build+ctest+scan), `bus-reply`/`bus-next`,
  `dept-cycle`/`dept-status`/`bus-ctx`. Model tiering: builder/researcher Sonnet,
  planner/reviewer Opus.
- **Recycled at this boundary:** builder (was 682k/opus + stale brief 030) and researcher
  (idle/opus) both dept-cycled → Sonnet, fresh context.

## Next
- **DONE batch-2:** RD3 craftsmen doc, P1 warning+manifold, P2 boolean dedup — all on master.
### P3 — SHIPPED 2026-06-17 (`707bc06`, Sonnet; Opus audit `replies/031`: SHIP)
Helix closed into a genuine watertight 2-manifold. Reviewer re-derived manifoldness across
12 configs (turns×nprof×sign, incl. nprof=4 + 1-turn) — all closed; screw_rise=0 byte path
untouched (4 goldens clean); lift intact; smoke helper honest. **Reviewer found** (NOT a P3
regression): 18 inverted-winding edges — a PRE-EXISTING property of BL-06's axis-spine
closure that P3 mirrors; cosmetic (OBJ proof has no normals; bpy recalcs). → **P3b** queued.

### P3b — SHIPPED 2026-06-17 (Sonnet; planner spot-check)
Reversed the partial-revolve sector fans + helix outer quads → both axis-spine closures are
now properly ORIENTED. New `assert_oriented` (every directed edge count==1) applied to the
partial revolve + helix; both pass. edi-gate GREEN (102/102), 4 goldens byte-identical
(winding only touched smoke-pinned meshes), smoke passes manifold+oriented. Builder verified
the 18-edge analysis. Accepted on green gate + spot-check. **P3 closure family now both
watertight AND oriented, locked by asserts.**

## REBASE GUARD (standing, hub fleet-infra)
origin/master is STALE/frozen. Rebase ONLY onto LOCAL `master` ref (no `git fetch`, never
origin/master). In EVERY slice brief. Local master is the real line (73c0832 → 1a9a7df (P3)
→ 2473a84 (P3b) → 70aae99 (DR-01..15 + more); rebase onto whatever it currently is).

## AUTONOMOUS run (user call) — run the queue ahead; bus-hub only on milestones
### P4 — SHIPPED 2026-06-17 (`2d2f02b`, Sonnet; spot-check)
Non-linear taper `taperCurve` (default 1.0 = linear = byte-identical). `t → t**taper_curve`
remap; survives lowering; bad_taper_curve validate; additive `taper_curve="1"` golden; 4 OBJ
byte-identical; smoke pins curved≠linear. Accepted on green gate + spot-check.
### P4b — SHIPPED 2026-06-17 (`d78f75c`, Sonnet; spot-check)
Per-axis taper `taperEndY` (0-sentinel = follow taperEnd → uniform = byte-identical). Per-axis
sx/sy scale about centroid; early-out enters when EITHER axis tapers; 0 allowed in validate
(sentinel); additive `taper_end_y="0"` golden; 4 OBJ byte-identical; smoke pins X/Y asymmetry.
**→ the deferred "per-axis / non-linear taper" item is now FULLY CLOSED (P4 + P4b).**
### RD-inset — SHIPPED 2026-06-17 (`e50e976`, researcher, docs-only)
`docs/inset-research.md` — P6 de-risked. Recommendation: replace `_inset_polygon` with
edge-offset-then-intersect (kills the 1/cos_half reflex singularity) + post-hoc refuse
(shoelace sign-flip / self-intersection → None → fallback). Straight-skeleton REJECTED
(polyskel buggy, no dep-free option, proof tier needs good-mesh-or-clear-refusal). Validate:
keep `prism_inset_too_large` + add `prism_inset_reflex_pinch` (inset ≥ 0.5·min adjacent edge).

## Next (autonomous)
- Bundle P4+P4b → rebase local master 70aae99 → edi-gate → bus edi-ui green tip → **RD1**
  (ScriptOp ASCII bbox proof, roadmap M1, builder).
- Then RD2 (recipe semantic-diff → TOON, builder), P5 (sweep miter), P6 (non-convex inset —
  use `docs/inset-research.md`), P7 (bounds tightness). Then batch-2 CLOSEOUT.
