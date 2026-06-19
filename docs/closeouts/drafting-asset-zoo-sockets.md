# Closeout — asset-zoo sockets + the anchor-type boundary

> Freezes a boundary so future work does not re-litigate it.

- **Boundary**: socket attach points on `AssetRecord`, and WHERE the 2D anchor type lives (the zoo-isolation question).
- **Department**: edi-drafting
- **Campaign**: drafting-20260619-zoo-store (pillar A — the asset zoo), slice A3
- **Date**: 2026-06-19
- **Commit**: `2e0106d` on `dept/drafting` (not pushed — hub owns the origin bridge)

## The decision
`AssetRecord` carries `std::vector<AssetSocket> sockets` — named NEUTRAL attach points
(the generator/placement connection points). Shapes:
```cpp
struct Anchor2D   { double x = 0.0; double y = 0.0; };          // LOCAL asset space
struct AssetSocket{ std::string name; std::string type; Anchor2D anchor; };
```
- `type` is a free-form **neutral open-vocab** string (e.g. "door"/"edge") — never an enum
  (same three-tier neutrality law as `category`/`meshRef`).
- The anchor is **zoo-local** (`Anchor2D`), deliberately NOT the drafting core's `Point2D`.
- Codec: each asset gains a `sockets` array key; each socket = `{name, type, anchor:[x,y]}`
  with the anchor a **2-element array** (parity with `DraftingSerialize` pointValue/readPoint).
- Ops: `addSocket(AssetRecord&, AssetSocket)->bool` (reject empty/duplicate name **within the
  record**), `findSocket(const AssetRecord&, name)->const AssetSocket*`.

## Why (the reasoning that must NOT be re-argued)
- **Zoo-local `Anchor2D`, not drafting `Point2D`**: `edi_zoo_core` links ONLY
  `edi_format_core`. Reusing `Point2D` would drag `edi_drafting_core` into that link line and
  break the zoo's deliberate isolation. Promoting a shared math type into `edi_format_core`
  was also rejected — the format core has no geometry today, and adding one to save a 2-field
  struct buys coupling for ~no reuse. The distinct NAME is intentional: a socket anchor is in
  **local asset space**, a genuinely different space from a canvas/document coordinate — two
  types is the honest model and stops a future "dedup" back into a dependency.
- **Anchor as a 2-element `[x,y]` array**: that is how every edi binary writes a point
  (DraftingSerialize) — one wire dialect, not two.
- **No version bump**: the version gate checks only schema+version, never per-asset keys, and
  `readAsset` reads every key tolerantly with a default. Old file (no `sockets`) → empty
  sockets; new file → still satisfies the v1 reader. `kAssetZooVersion` stays 1; EDIM
  envelope untouched (the wall_visual additive discipline).
- **Socket-name uniqueness is record-local**: two assets may each have a "door" socket; a
  name is a within-asset label, like a port name on a component.

## The contract (what future work must respect)
- Keep `edi_zoo_core` single-dep on `edi_format_core` — no `#include "drafting/..."` in
  `src/zoo`, no new link dep. If another zoo type needs a point, use/extend `Anchor2D`, do
  NOT reach for drafting's `Point2D`.
- Anchors serialize as 2-element `[x,y]` arrays; the reader must bounds-guard (size==2 else
  {0,0}) and tolerate Int-or-Double scalars (the local `asDouble`).
- Reads stay additive + tolerant; new socket fields (if any) follow the same missing=>default
  pattern with no version bump.
- `type` stays a free-form neutral string.

## Out of scope / explicitly NOT allowed
- No controller/UI/canvas wiring for sockets yet (no placement snapping consumes them).
- No socket geometry beyond the local anchor (no orientation/normal) — add additively if a
  later slice needs it.
- No JSON; MessagePack via the existing value codec only.

## Queued next in this campaign (need hub go)
- **assetRef ↔ catalog wiring** — make a drawing's `DraftingBlock.assetRef` /
  `BlockPlacementMetadata.assetRef` resolve/validate against the zoo (closes the
  dangling-string loop). This one WILL cross into `src/drafting`/`src/core` — a
  cross-boundary slice; the resolver likely lives controller-side, not in `edi_zoo_core`.

## Pointers
- Code: `src/zoo/AssetZoo.h` (Anchor2D/AssetSocket), `src/zoo/AssetZooOps.{h,cpp}`
  (addSocket/findSocket), `src/zoo/AssetZooSerialize.cpp` (sockets codec + local asDouble)
- Tests: `tests/asset_zoo_tests.cpp` (socket round-trip / tolerant-missing / addSocket /
  findSocket)
- Reuses pattern from: `src/drafting/DraftingSerialize.cpp:95-130` (point codec)
- Related: `docs/closeouts/drafting-asset-zoo-store.md` (A2), charter
  `docs/departments/edi-drafting.md`, vision `~/dept-bus/edi-architecture-tool-vision.md`
