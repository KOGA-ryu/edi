# Handoff — drafting-20260617-feature-bucket (DR-01..15)

> Per-campaign state for the dispatched 15-task feature bucket. One gate log per
> task. Source of tasks: `~/dept-bus/work-batch-plan.md` (edi-drafting section);
> surfaces `~/edi/docs/ui-surface/drafting/DR-surfaces.md`; dispatch
> `~/dept-bus/FEATURE-DISPATCH.md`.

- **Campaign**: drafting-20260617-feature-bucket
- **Department**: edi-drafting
- **Goal (one line)**: build the compass-and-straightedge construction kit DR-01..15
  as pure-ops slices (+ thin controller wiring) over the existing Result-struct +
  DraftingCommand pipeline — no generation, no rules. Commit to `dept/drafting`; do
  NOT merge (hub routes verified work to edi-ui).
- **Run order**: DR-01 FIRST (keystone — dungeon-map DM-14/15 blocked on it; bus the
  SHA the instant it's green). Then DR-02…15 per the plan's dependency order.
- **Ratified forks**: DR-13 angular dim = INFER the vertex from the two source lines
  (NO new `DimensionGeometry` field). DR-15 region fill = fill-the-selected-closed-
  object (true flood-fill parked).
- **Gates**: reviewer boundary ONLY where a task genuinely needs one (most are
  spec-settled); builder; green gate `cmake --build build && ctest --test-dir build
  -E edi_shell_window_tests` + scan. Rebase on master at the start of each task.

## Gate log

### DR-01 transformGeometry — builder BRIEFED 2026-06-17
- Brief: `~/dept-bus/edi-drafting/briefs/008-DR01-transformgeometry-builder.md`
- Boundary: SETTLED by spec — signature + per-kind rules + idiom all fixed in the
  work-batch-plan DR-01 entry; no reviewer gate needed up front.
- **Contract-file note:** the spec cites `/tmp/dept-bus-stage/002-transformGeometry-
  contract.md`, which does NOT exist on this box (Mac-side staging path, not
  transferred). The work-batch-plan DR-01 entry reproduces the full signature + all
  14 per-kind rules inline, so it is treated as authoritative. Reported to hub as an
  FYI in case the canonical contract adds anything.
- Plan: builder → green gate → reviewer diff-audit (cross-dept keystone: verify all
  14 arms + the pinned Ellipse/Text v1 limitations) → bus the SHA to hub.

## Open questions / blockers
- (none blocking — proceeding on the inline spec; missing contract file flagged to hub)

## Next
- Builder implements DR-01; planner buses the green SHA to the hub so dungeon-map
  unblocks, then opens DR-02.
