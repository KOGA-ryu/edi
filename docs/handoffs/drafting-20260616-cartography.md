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

### Reviewer gate — DONE 2026-06-16 — edi-drafting-reviewer (via bus)
- Reply: `~/dept-bus/edi-drafting/replies/001-cartography-reviewer-map.md` (full
  inventory: 1a files, 1b types, 1c the 14 geometry arms + 19 visit sites, 1d the
  33 command arms + the single dispatch, 1e plan/build fns, 1f controller helpers,
  1g call-graph).
- **Folded into `docs/architecture/edi-drafting.md` (first draft).**
- **Gate verdict: boundary CONFIRMED but NOT settled — ONE fork escalated to hub**
  (the map-graph ownership axis; see blockers). Code-health verdict: issues-found,
  all behavior-preserving (one HIGH, rest MED/LOW).
- **Refactor backlog captured** in the architecture doc §5. The behavior-preserving
  slices do NOT depend on the ownership fork → builder batch can run in parallel
  with the hub's arbitration.

### Builder batch — BRIEFED 2026-06-16 (exhaustiveness + nits)
- Brief: `~/dept-bus/edi-drafting/briefs/002-cartography-exhaustiveness-builder.md`
- Slices: (1 HIGH) DraftingCommand terminal `else` → `static_assert(always_false_v)`;
  (2 MED) make the 3 unguarded geometry visits exhaustive by making current behavior
  EXPLICIT (Mirror, QuickMeasure, PlotPlan ×2) — NOT a bare guard; (3 LOW)
  `circleSegments` named constant + `highestDocumentIdSerial` comment fix.
- DEFERRED to a later batch: MoveSelection/Align/Distribute dedup (MED, restructures
  behavior-bearing code — wants its own careful gate). Map-graph extraction is
  BLOCKED on the hub fork.

## Open questions / blockers
- **ESCALATED TO HUB 2026-06-16 — `src/drafting` map-graph ownership fork.** The map
  graph physically lives in our files and is NOT separable by file alone:
  `DraftingDocument` (a CORE type) embeds the plug/connection/room/block vectors and
  `DraftingCommand` (a CORE variant) embeds 7 map arms. The two departments collide
  on `DraftingDocument.h` + `DraftingCommands.h` regardless. The hub must rule the
  boundary AXIS: (a) by-file ownership (dungeon-map gets a sub-target, map structs
  migrate, forcing a Document/Command split) OR (b) by-domain ownership (structs stay
  in the shared core doc; dungeon-map owns the `*Ops`/`*Room`/`*Corridor`/`*Pathfind`/
  `*AsciiMap` files + map command semantics). No builder spend on a split until ruled.
- `transformGeometry` is absent and is a shared primitive (us + dungeon-map) — flag
  for JOINT design before either builds it (architecture doc §7).

## Next
- Builder runs batch 002 (exhaustiveness + nits) → green gate → commits to
  dept/drafting → reply. Planner keeps the architecture doc current as it lands.
- Await hub ruling on the ownership fork.
