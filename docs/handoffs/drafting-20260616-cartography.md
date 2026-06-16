# Handoff — drafting-20260616-cartography

> The per-campaign state. Each gate appends its result; the NEXT gate reads this
> first. Agents hand off THROUGH this file — they cannot message each other.

- **Campaign**: drafting-20260616-cartography
- **Department**: edi-drafting
- **Goal (one line)**: MAP, DOCUMENT, and behavior-preservingly REFACTOR the
  drafting core (`src/drafting`, `src/core`) so its architecture is understood and
  clean BEFORE features land — produce `docs/architecture/edi-drafting.md` and a
  vetted refactor backlog.
- **Boundary (the question the reviewer gate must settle)**: what is the true
  file/type inventory of the drafting core, what is core-geometry (ours) vs
  map-specific (dungeon-map's domain living in our files), where are the seams to
  other departments, and which refactor candidates are real (duplication, dead
  code, non-exhaustive visits, drift from the data-oriented rules, missing
  explaining comments) — all BEHAVIOR-PRESERVING.

## Gate log

### Research gate — SKIPPED
- The missing input is an internal OWNERSHIP / architecture-mapping question
  ("what's in our own files, what belongs to whom"), not external/reference
  knowledge. Per the gate discipline this opens at the reviewer gate.

### Reviewer gate — OPEN 2026-06-16 — edi-drafting-reviewer (briefed via bus)
- Brief: `~/dept-bus/edi-drafting/briefs/001-cartography-reviewer.md`
- Awaiting the inventory + boundary + refactor-candidate findings.

## Open questions / blockers
- The **`src/drafting` ownership boundary**: map graph (plug/connection vectors,
  `DraftingMapRoom`/wall geometry, `CreatePlugCommand`/`DeletePlugCommand`) lives
  inside our files but is edi-dungeon-map's DOMAIN. To be flagged to the hub once
  the reviewer confirms the exact file/type split — the hub arbitrates with
  edi-dungeon-map.

## Next
- Reviewer returns findings → planner writes `docs/architecture/edi-drafting.md`
  (first draft) + decides the refactor slices → brief the builder.
