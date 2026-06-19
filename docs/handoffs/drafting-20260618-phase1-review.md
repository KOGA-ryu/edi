# Campaign: drafting-20260618-phase1-review (spatial-model inversion — Phase 1)

**My role: REVIEW/CONSULT only.** dungeon-map LEADS the data-spine on `dept/dungeon-map`; edi-ui is the SOLE
integration owner of the hot files (`DraftingMapTypes.h` / `DraftingRoom.h` / `DraftingSerialize.cpp` /
`MapToonExport.cpp` / `createMapFromSpec`). **drafting-core does NOT parallel-build those files** — avoid the
integration chokepoint. I audit the drafting-core-touching slices as dungeon-map hands them, flag geometry/
serialize defects, and stand by for dungeon-map review requests. Source: `~/dept-bus/SPATIAL-MODEL-BACKLOG.md`
(DECISIONS ratified 2026-06-18 + EXECUTION).

## The 3 slices I audit + the baselines to audit AGAINST
1. **Pure `footprintsOverlap` primitive** — dungeon-map lifts `roomsOverlap` out of `src/io/RoomSpecStore.cpp`
   into `src/drafting/` (`footprintsOverlap`, `overlapRect→optional<Rect>`, `overlapArea`).
   BASELINE (RoomSpecStore.cpp:253-262, MUST be preserved byte-for-byte): `constexpr double eps = 1e-9;`
   `xOverlap = a.x < b.x+b.w-eps && b.x < a.x+a.w-eps;` (y likewise); overlap = xOverlap && yOverlap.
   ⇒ TOUCHING edges do NOT overlap; eps absorbs float touch. AUDIT: same eps, same strict-`<`-minus-eps form,
   pure (no Qt), behavior-identical; `overlapRect`/`overlapArea` return nullopt/0 for touch/zero-area;
   the RoomSpecStore caller (line 386) now delegates with identical result.
2. **`RoomSpec` per-edge walls + `planDraftingRoom` honors absent edges.**
   BASELINE (DraftingRoom.cpp ~78-116): loops N/E/S/W, ALWAYS emits a WallGeometry per edge segment at
   `spec.wallThickness`; degenerate span (opening to corner) skipped → corner open. AUDIT the new behavior:
   absent edge ⇒ emit NO WallGeometry AND skip the corner-join for it; per-edge thickness/material with
   fallback to the room default (single source of truth); a zero-wall room is LEGAL (empty object batch path
   must accept it). Chamber/all-walls rooms must stay byte-identical.
3. **MessagePack serialize spine — additive, NO version bump.** Each new foundational field (`level`, `kind`,
   per-edge `walls` mask, `ceiling`/`floor`, `DraftingNode`, derivation/`bounded_by`, polygon footprint) is
   written only when non-default and read tolerantly; a document missing every new key reproduces TODAY's
   bytes exactly (the symmetry + additive-tolerance discipline from the M8-S1 / MapBlockSpec audits).
   Watch `highestDocumentIdSerial` for every new id-bearing vector (e.g. DraftingNode).

## Ratified decisions that shape the audits
- Inversion COEXISTs behind `RoomDerivation { Placed | SpanDerived }` (don't break room-as-node).
- Overlap default PICK-ONE; pure resolver lives in src/drafting, GENERATOR is default caller.
- Span = 2-node widened-edge first (N-node fast-follow); STORE materialized footprint + node refs.
- Elevation = discrete integer `level`; edi owns NO Z; `feet_per_band` is DATA (no hardcoded pitch).
- TOON: header-as-truth, index BY NAME, one canonical column owner, no version line.
- Every dimension is DATA (the standing hard rule) — flag any magic dimension literal in handed slices.
- Order: crypt regression-lock golden FIRST, then `syncGraphForMovedObject`, then additive struct slices.
  Every slice: additive + MessagePack round-trip (no bump) + green gate + determinism harness.

## Reviews delivered
- **Slice 2 — `syncGraphForMovedObject` @ f4dbf8f (dept/dungeon-map)** — CONSULT CLEAN, FOLLOW-UP not block.
  All 5 substrate concerns PASS (anchor accessor matches seed + canonical PointGeometry idiom; pure/Qt-free;
  deterministic; zero wire/serialize surface / no version bump; no magic dims); wiring after-mutation/
  before-commit, multi-plug + multi-select safe, undo clean. Reply: ~/dept-bus/drafting/replies/054-syncgraph-consult-clean.md.
  2 minor follow-up nits (test brittleness at drafting_graph_ops_tests.cpp:312-318; bool means "touched" not
  "changed" — doc note). The op is a clean template for the remaining sibling slices.

- **Slice 3a — additive `int level` on DraftingMapRoom @ 47136bb (dept/dungeon-map)** — CONSULT CLEAN, no
  block, no follow-up. Serialize-spine TEMPLATE blessed: always-write `{"level", integer(...)}` +
  tolerant `asInt(child(v,"level"), default0)` (missing⇒0), matches schema_version precedent; integer() not
  number() (no cast risk); MapToonExport untouched (canary byte-identity); no version bump; level=neutral band,
  0=exempt unset; round-trip + key-stripped forward-compat tests real. Reply:
  ~/dept-bus/drafting/replies/058-level-field-consult-clean.md. Future additive-field slices cite this shape.

- **Slice 3b — RoomDerivation enum @ b4c028a (dept/dungeon-map)** — CONSULT CLEAN, no block, NO deviation from
  the WallType/3a templates. Closed enum {Placed, SpanDerived} mirrors ObjectRole/WallType (unknown⇒Placed,
  free fns beside peers in DraftingGeometry.cpp); additive by-name serialize symmetric (always-write text +
  read fromName(asString(...,"placed")), missing⇒Placed, no version bump); MapToonExport untouched; neutral
  classification, no dim literal; name↔enum + value+byte round-trip + key-strip forward-compat tests real;
  aggregate-init sites tolerate the trailing field (C++20). Reply: ~/dept-bus/drafting/replies/060-roomderivation-consult-clean.md.
  (Reviewed on Sonnet per the template-following tiering guidance.)

## Status
- Standing by for the next handed slices: footprintsOverlap primitive, RoomSpec per-edge walls +
  planDraftingRoom absent-edge emission, more additive serialize fields. No autonomous builds on hot map files.
- Prior campaign (no-magic-dims) COMPLETE; 050/051 (kCanvasBoardExtent) await edi-ui/hub merge.
