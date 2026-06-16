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

### Builder batch — DONE 2026-06-16 (edi-drafting-builder, via bus)
- Reply: `~/dept-bus/edi-drafting/replies/002-cartography-exhaustiveness-builder.md`.
- **Slice 1 `985e200`** — `applyDraftingCommand` terminal `else` →
  `static_assert(always_false_v<Command>)`; all 33 arms compile-match, runtime
  byte-identical. **Slice 2 `2a1be77`** — four geometry visits made
  compile-exhaustive WITHOUT behavior change (explicit per-kind arms + terminal
  guard): Mirror (7 transform + 7 explicit pass-through), QuickMeasure (5 measured +
  9 explicit `Unsupported`), PlotPlan appendPlotSegments + closedFillRing (explicit
  empty/no-segment arms; fillable set + circle-32 frozen-boundary respected).
  **Slice 3a `cae383a`** — magic `32` → `constexpr kCircleSegments` (stroke + fill
  must stay equal). **Slice 3b** dropped + reverted (map region).
- Green gate: build clean (compiling PROVES exhaustiveness over 14 kinds); ctest
  95/95 (`-E edi_shell_window_tests`, the known edi-ui golden drift); scan clean.
- Builder flags for the arch doc: the mirrorable set lives in TWO hand-kept lists
  by design (`mirrorGeometry` visit keys on geometry type, `supportsMirror` keys on
  `DraftingShapeKind`) — NOT unified (would be behavior-risking); the visit guard now
  catches a forgotten kind on the VISIT side + a sync comment added.

### Reviewer diff-audit — OPEN 2026-06-16 (edi-drafting-reviewer, via bus)
- Brief: `~/dept-bus/edi-drafting/briefs/004-cartography-diff-audit-reviewer.md`
- Priority question: confirm QuickMeasure's 9 `Unsupported` arms reproduce the OLD
  `else` result exactly (the gate-map called it "degrades to base measure" — verify
  base-measure vs Unsupported is not a silent behavior change).

## Open questions / blockers
- **RESOLVED 2026-06-16 — HUB RULING H2 (`~/dept-bus/RULING-H2-src-drafting-boundary.md`):
  by-domain, SINGLE document.** Do NOT split `DraftingDocument`/`DraftingCommand`.
  Shared headers co-edited BY REGION (drafting=core, dungeon-map=map). Recorded
  verbatim in architecture doc §6. Consequences folded:
  - Builder slice 3b (`highestDocumentIdSerial` comment) is a MAP region → DROPPED
    from batch 002 via amendment brief 003; deferred to dungeon-map. Batch is now
    Slice 1 + Slice 2 + Slice 3a (all core).
  - `transformGeometry` is DRAFTING-owned (dungeon-map consumes); it is a parked
    FEATURE, not cartography work — ownership recorded in architecture doc §7.
  - dungeon-map will extract map struct/enum defs into a `DraftingMapTypes.h`
    (their slice, not ours) — shrinks our shared-edit surface to an include line.

## Next
- Builder runs batch 002 (exhaustiveness + nits) → green gate → commits to
  dept/drafting → reply. Planner keeps the architecture doc current as it lands.
- Await hub ruling on the ownership fork.
