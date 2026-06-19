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

## Next
- Await dungeon-map's first green slice (the regression-lock golden); merge it,
  confirm the canary, edi-gate, continue in order.
