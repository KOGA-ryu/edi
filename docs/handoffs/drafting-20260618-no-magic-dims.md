# Campaign: drafting-20260618-no-magic-dims

**Rule:** CLAUDE.md hard rule + PROTOCOL.md "No hardcoded dimensions" + SCALE-POLICY.md invariant 0
(USER, 2026-06-18): every DIMENSION is DATA (named field in a spec/config/constants-table, derived, or
parameterizable) — never a magic literal in logic. EXEMPT: epsilons/tolerances, "unset" 0.0 defaults,
direction/unit vectors. Reviewer-enforced.

**Scope (this dept):** `src/drafting/**` + the map/canvas authoring path in `src/core/`
(`DrawingDocumentController.cpp`). EXCLUDES `src/widgets` (edi-ui) and the bpy realizer (blender-lab).

## Status
- 044 (corridor derive-from-scale, the COHERENCE slice) DONE green, reviewer ACCEPT-WITH-NIT, tip 6f64598
  on dept/drafting. Reported ready to merge (M0-render-critical: doubled crypt corridor → 10 ft). The nit =
  the magic literals 044 itself introduced → folded into this sweep.
- Reviewer inventory COMPLETE (brief 045 → reply). 14 literals classified below.

## INVENTORY (reviewer 045) — src/drafting + map/canvas path
| file:line | literal | dimension | proposed home |
| --- | --- | --- | --- |
| DrawingDocumentController.cpp:~3199 | `/ 3.0` | corridor = ⅓ narrowest room short edge | NAMED `kCorridorPerRoomShortEdge` (keep division form) |
| ~3200 | `0.045` | corridor-width fallback (roomless) | NAMED `kDefaultCorridorWidth` (the spine) |
| ~3204 | `0.02/0.045` | door-leaf : corridor ratio | NAMED `kDoorLeafToCorridorRatio` (keep division) |
| ~3205 | `0.015/0.045` | corridor-wall : corridor ratio | NAMED `kCorridorWallToCorridorRatio` (keep division) |
| ~2230 | `0.045` | interactive corridor width | NAMED `kDefaultCorridorWidth` (same spine) |
| ~2231 | `0.015` | interactive corridor wall thickness | NAMED (or width×ratio) |
| ~2584 | `0.0225` | setPlugType leaf half-width | DERIVE `kDefaultCorridorWidth / 2.0` |
| ~2591 | `0.02` | setPlugType leaf band thickness | NAMED `kDefaultDoorLeafThickness` |
| ~3446 | `0.62` | ascii auto-fit board fraction | NAMED `kAsciiBoardFillFraction` |
| ~4144 | `0.02,0.02` | duplicate nudge offset | NAMED `kDuplicateNudge` |
| ~4197 | `0.02,0.02` | paste nudge offset | NAMED `kPasteNudge` |
| DraftingRoom.h:~84 | `0.1` | RoomSpec.wallThickness default | already a SPEC FIELD; name the default |
| DrawingDocumentController.cpp:~2086 | `0.5` | floor fill opacity | EXEMPT (not spatial); optional tidy `kFloorFillOpacity` |
| DraftingCorridor.h:~21-22 | `0.06`,`0.02` | CorridorSpec default w/thk | EXEMPT-in-practice (dormant; both callers override) |

Counts: NAMED-CONSTANTS-TABLE 9 · DERIVE 1 · SPEC-FIELD(already) 1 · EXEMPT 3.
Behavior flags: keep the 044 ratios as `x*(a/b)` (NOT pre-rounded) for byte-identical history; 2584's
`0.0225` becomes `width/2` (tracks the spine — correct, no value change at current spine).

## SWEEP PLAN (3 slices, shared spine)
- **Slice A+B (046):** introduce a named canvas-dims data table in `src/drafting/` (constexpr FREE constants,
  not a class — DOD): `kDefaultCorridorWidth`, `kCorridorPerRoomShortEdge`, `kDoorLeafToCorridorRatio`,
  `kCorridorWallToCorridorRatio`, `kDefaultDoorLeafThickness`. Wire createMapFromSpec + createConnection +
  setPlugType to it (corridor/door/wall dims). Behavior-preserving (identical values). One reviewable
  "corridor geometry as data" unit.
- **Slice C (047):** editing/fit offsets — `kPasteNudge`/`kDuplicateNudge` (paste+duplicate), 
  `kAsciiBoardFillFraction`; tail: name RoomSpec.wallThickness default + (optional) CorridorSpec defaults +
  floor opacity. Separate slice (unrelated to corridors), keeps 046 tight.

Each slice: edi-gate green, behavior-preserving, reviewer-audited for residual magic dims, then merged.
