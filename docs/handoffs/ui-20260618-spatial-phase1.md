# Handoff — ui-20260618-spatial-phase1 (Phase 1 integration owner)

> edi-ui is the **SOLE integration owner** for the dungeon spatial-model
> inversion, Phase 1 (foundational data-spine). Every hot-file change
> serializes through edi-ui to LOCAL master so concurrent struct changes
> cannot collide. Source backlog: `~/dept-bus/SPATIAL-MODEL-BACKLOG.md`
> (DECISIONS + EXECUTION, ratified 2026-06-18).

- **Campaign**: ui-20260618-spatial-phase1
- **Role**: integration only (LEAD builder = edi-dungeon-map on dept/dungeon-map;
  drafting-core = review/consult, does NOT parallel-build the hot files).
- **Hot files (serialize ALL through edi-ui):** `src/drafting/DraftingMapTypes.h`,
  `src/drafting/DraftingRoom.h`, `src/drafting/DraftingSerialize.cpp`,
  `src/io/MapToonExport.cpp`, `createMapFromSpec` (DrawingDocumentController.cpp).

## MERGE ORDER (each: green on dept/dungeon-map → I merge → edi-gate → green)
1. **Crypt regression-lock golden** (the CANARY — lands FIRST, the safety net).
2. **`syncGraphForMovedObject`** (deferred anchor-resync prereq; fixes deriveEdge drift).
3. **Additive struct slices** (DraftingNode, span-room RoomDerivation, level band,
   vertical features, …) — each ADDITIVE + MessagePack round-trip (NO version bump)
   + green gate. Determinism harness is a Phase-1 gate.

## THE CANARY — byte-identical TOON regression lock
The crypt/dungeon TOON must stay **byte-identical** after every new-field-default
and the default `reject` path must still fire. If the golden EVER drifts
non-byte-identical, **a default is not neutral → HALT + flag the hub** (do NOT
re-bless to hide it).

**Independent baseline (master @0d5ca60, pre-Phase-1):**
- `tests/data/dungeon.map.toml` → TOON: **sha256 `6c632293229e43c9d57cb6aae443f6d4f8c93927c3196e001fae8f2c3012b0e3`**, 1685 bytes.
- Rows: **rooms[12] · plugs[26] · connections[12]** (no blocks in this fixture).
- Re-check anytime: `QT_QPA_PLATFORM=offscreen ./build/edi --map-file tests/data/dungeon.map.toml --export-map /tmp/c.toon && sha256sum /tmp/c.toon` — must equal `6c632293…`.
- (The backlog's "74-object count" = internal DraftingObject count; dungeon-map's
  golden test pins it. The TOON sha256 above is edi-ui's primary cross-check.)

## Merge log
| When | Slice | dept tip → merge | canary (sha256 6c632293…?) | edi-gate |
| --- | --- | --- | --- | --- |
| — | baseline | — | baseline 6c632293 | master @0d5ca60 GREEN 106/106 |
| 2026-06-18 ~19:3x | **1. regression-lock golden (CANARY)** | `3e01ca7` → `5aa28d9` | ✅ byte-identical (test #107 + independent sha256) | GREEN 107/107 |
| 2026-06-18 ~19:5x | **2. syncGraphForMovedObject** | `f4dbf8f` → `7e94407` | ✅ byte-identical (no map data changed) | GREEN 108/108 |
| 2026-06-18 ~20:0x | **3a. additive int level=0 (DraftingMapRoom)** | `47136bb` → `c3e41da` | ✅ byte-identical (level=0 default IS neutral) | GREEN 108/108 |
| 2026-06-18 ~20:1x | **3b. RoomDerivation enum (COEXIST) + derivation field** | `b4c028a` → `ab58a27` | ✅ byte-identical (Placed default neutral) | GREEN 108/108 |
| 2026-06-18 ~20:2x | **3c. DraftingNode connector entity + id recovery** | `ec593fd` → `7fa3886` | ✅ byte-identical (off the TOON wire) | GREEN |
| 2026-06-18 ~20:3x | **3d. OverlapPolicy enum + footprintsOverlap primitive** | `581e249` → merge | ✅ byte-identical + reject-path fires (PickOne default) | GREEN |
| 2026-06-18 ~20:4x | **3e. RoomKind enum + per-edge wall presence (RoomSpec)** | `81d42b3` → merge | ✅ byte-identical + object-count 170 (defaults neutral; not wired till Phase 2) | GREEN |
| 2026-06-18 ~20:5x | **3f. level on DraftingPlug + DraftingDeclaredConnection** (LAST struct slice) | `7ca4b8f` → merge | ✅ byte-identical (level off the TOON wire, default 0) | GREEN |
| 2026-06-18 ~21:0x | **3g. determinism gate + stabilize corridor-obstacle iteration** | `fd5579c` → merge | ✅ byte-identical (sort output-preserving) | GREEN 110/110, ZERO segfaults (arbiter) |

**✅ PHASE-1 SLICE-3 COMPLETE** — canary 6c632293 byte-identical through ALL slices (1 golden / 2 sync / 3a-f structs / 3g determinism). Builder's 7-segfault flag resolved: a stale-build artifact; clean rebuild = full ctest 110/110, zero segfaults.

Slice 3a = the FIRST additive FIELD. Rides ONLY in struct + MessagePack
(field-tagged, missing→0); MapToonExport UNTOUCHED, so the positional TOON wire is
unchanged → canary byte-identical = the default is neutral. drafting-core
CONSULT-CLEARED (no follow-up). **This is the template for every additive struct
slice: MessagePack-additive + TOON-untouched-or-omit-at-default.**

Slice 2 = pure DraftingGraphOps anchor-resync (move-counterpart of prune); retires
the deriveEdge drift. Hot files touched (DraftingGraphOps, DraftingMapTypes.h
comment-only, controller move-path) — NO field/wire/bump. drafting-core
CONSULT-CLEARED (5 substrate concerns pass; 2 non-blocking nits tracked dept-side).

The canary (`tests/map_regression_lock_tests.cpp`, #107) now runs in every gate.
Object count pinned at **170** (backlog's "~74" was an estimate; corrected to the
real count at 0d5ca60). HALT protocol embedded in the test.

## Risks I own (from the backlog)
- **TOON is positional** (no version line) — one non-neutral default breaks every
  golden simultaneously. I own column order + the canary. Any drift = HALT.
- **Derived-value drift** — derive-on-edit via `syncGraphForMovedObject`, not new
  uncached-on-move caches.
- **Determinism** — sorted containers + seed; the existing `unordered_map`
  iteration in createMapFromSpec is a known non-determinism source; watch for it.

## ✅ PHASE 1 COMPLETE (master 3ce29a3) — canary byte-identical through ALL slices.

---

# PHASE 2 — full breadth (sole integrator, hub relays SHAs)

I merge each hub-pinged dept SHA **in order**, edi-gate, canary-gated. Critical path:
- **P2-A WIRE EXTENSION** (dungeon-map = ONE column-order owner): header-as-truth —
  add `nodes[]{name,anchor,type}` + rooms `derivation`/`bounded_by` + walls-mask +
  kind + level (+ levels manifest). **THE SLICE I SCRUTINIZE HARDEST.**
- **P2-B SPAN-ROOM GENERATOR** (dungeon-map + drafting): mint nodes + derive span
  footprint (2-node widened-edge first).
- **P2-C REALIZER** (blender-lab): first REAL inverted render on the 5090 = MILESTONE.
- THEN breadth: dual-graph/subsume, per-edge wall rendering, overlap+merge+resolver,
  DraftingVerticalFeature, .map.toml grammar, multi-floor camera.

## CANARY PROTOCOL for Phase 2 (the wire changes)
1. **Every slice:** re-export `tests/data/dungeon.map.toml` → MUST be `6c632293…`
   (the node-less reference dungeon). Off-wire/additive slices keep it unchanged.
2. **On the WIRE slice (P2-A):** a golden change is OK **only if ALL**: (a) DELIBERATE
   — dungeon-map updated `tests/map_regression_lock_tests.cpp` expected TOON + sha
   **in the same commit** with a stated wire-extension reason; (b) the **node-less
   reference dungeon STILL emits byte-identical** (no spurious empty `nodes[]`,
   default columns omitted — header-as-truth); (c) edi-gate GREEN. A golden FAILURE
   without a same-commit deliberate update → **HALT + flag the hub**.
3. If a deliberate wire change lands, **update the reference sha** below + record WHY.

## Phase 2 merge log
| When | Slice | dept SHA → merge | canary | edi-gate |
| --- | --- | --- | --- | --- |
| — | (Phase 2 baseline) | master `3ce29a3` | `6c632293` (node-less ref) | GREEN 110/110 |
| 2026-06-18 ~21:xx | **A1. nodes[] TOON section (conditional, WIRE)** | `426b379` → merge | ✅ NEUTRAL — `6c632293` unchanged (empty nodes[] omitted; 0 nodes[] in node-less ref) | GREEN |
| 2026-06-18 ~21:xx | **A2. level column on rooms+plugs (conditional, WIRE)** | `5fa7583` → merge | ✅ NEUTRAL — `6c632293` unchanged (all-level-0 → column omitted; headers carry no ,level) | GREEN |
