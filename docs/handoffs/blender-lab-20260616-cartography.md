# Handoff — blender-lab-20260616-cartography

> The per-campaign state. Each gate appends its result; the NEXT gate reads this
> first. Agents hand off THROUGH this file — they cannot message each other.

- **Campaign**: blender-lab-20260616-cartography
- **Department**: edi-blender-lab
- **Goal (one line)**: MAP + DOCUMENT + behavior-preservingly REFACTOR the recipe
  lab so its architecture is understood and clean before features land.
- **Boundary (the question the reviewer gate must settle)**: What exactly is the
  recipe lab's architecture today — the `RecipeOp` variant and its full set of
  exhaustive-visit sites, the C++↔Python TOML contract key-for-key, the
  resolve/lowering path, the proof tiers, the craftsmen scan, the Blender
  subprocess wiring, click→PNG call graph, the seams in/out — and which
  behavior-preserving refactors are warranted (duplication, dead/empty arms,
  drift from data-oriented rules, un-commented wiring)?

## Gate log

### Reviewer gate — 2026-06-16 — edi-blender-lab-reviewer (OPEN)
Brief sent: `~/dept-bus/edi-blender-lab/briefs/002-cartography-reviewer.md`.
Awaiting findings (read-only map + refactor candidates, NO code).

## Open questions / blockers
- Worktree has NO `build/` dir yet — `cmake -S . -B build` needed once before the
  green gate runs (builder concern when that gate opens).

## Next
- Fold reviewer findings into `docs/architecture/edi-blender-lab.md` (first draft).
- Decide behavior-preserving refactor slices → builder batch.
