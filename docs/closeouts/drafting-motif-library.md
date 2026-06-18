# Closeout — motif library / flash sheet (M8)

> Freezes the motif boundary so future work does not re-litigate it.

- **Boundary**: the reusable-motif (flash-sheet) model — where it lives, its shape, and how it
  captures/serializes/places.
- **Department**: edi-drafting
- **Campaign**: drafting-20260617-batch2 (M8)
- **Date**: 2026-06-17

## The decision
A **`DraftingMotif { std::string name; std::vector<DraftingObject> objects; Bounds2D bounds; }`**
is a **document-level CORE field** (`std::vector<DraftingMotif> motifs` on `DraftingDocument`,
beside `objects`/`layers`) — a deliberate **TWIN** of the map-owned `DraftingBlock`, stripped of
the map baggage: **no `id`, no `assetRef`, name-keyed** (so it never touches the map-region
`highestDocumentIdSerial`), no coupling to the block system (H2).
- **Capture** (`buildMotifFromObjects`, mirrors `buildBlockFromObjects`): normalize to lower-left
  (0,0), reset `locked=false`/`visible=true`; EXCLUDE guides, INCLUDE construction lines + the
  rest; `addMotif` rejects empty/duplicate name + empty objects. Capture rides a transient
  `CreateMotifCommand` (commands are not serialized).
- **Serialize**: additive top-level `"motifs"` key reusing the `DraftingObject` codec; **no
  version bump**; default-empty when absent (the wall_visual/plug/fill precedent). `bounds` is
  derived, NOT serialized. (Reviewer-audited: encode/decode exact inverses.)
- **Place** (`placeMotif` → FLATTEN): emit FRESH ordinary `DraftingObject`s — new ids (minted via
  the controller's normal `m_nextObjectSerial`), translated so the motif's (0,0) lands at the
  pick point, **no motif back-reference** — in one `CreateObjectsCommand` = one undo. Controller:
  `defineMotifFromSelection` + `PointCaptureIntent::MotifPlacement` (`beginMotifPlacement(name)` /
  `runMotifAtPoint`).

## Why (not to re-argue)
- **Document-level over sidecar:** rides the existing snapshot undo/redo + `.edidraw` persistence
  for free; matches "capture this selection, re-drop on this grid." A cross-document GLOBAL flash
  sheet would be a sidecar — a possible FUTURE follow-up, not v1.
- **A twin, not a shared base with `DraftingBlock`:** sharing would re-entangle the H2 boundary
  the hub just split. Revisit a shared base only if a 3rd flash-sheet consumer appears.
- **FLATTEN-on-place:** placed objects are indistinguishable from hand-drawn, so the ~16
  object-walkers stay simple (the unanimous block-library decision).

## Out of scope / parked
- **S3 transform-on-place (oriented stamping)** — DEFERRED behind a fork. `transformGeometry`
  (DR-01) makes it geometrically cheap, but the real cost is the placement UX (an angle/scale
  gesture + edi-ui surface). Decide after v1 translate-only is in use.
- **Palette/browser widget** to pick a motif — edi-ui dependency.
- Cross-document/global motif library — future (sidecar).

## Pointers
- Code: `src/drafting/DraftingMotifOps.{h,cpp}`, `DraftingDocument.h` (`DraftingMotif`/`motifs`),
  `DraftingCommands.{h,cpp}` (`CreateMotifCommand`), `DraftingSerialize.cpp` (`"motifs"`),
  `src/core/DrawingDocumentController.cpp` (`defineMotifFromSelection`/`beginMotifPlacement`/
  `runMotifAtPoint`, `PointCaptureIntent::MotifPlacement`).
- Tests: `tests/drafting_motif_ops_tests.cpp`, motif blocks in `drafting_serialize_tests.cpp` +
  `drawing_document_controller_tests.cpp`.
- Handoff: `docs/handoffs/drafting-20260617-batch2.md`. Reviewer gate + audit:
  `~/dept-bus/edi-drafting/replies/008-...` + `009-...`. Hub scope record: block/motif twin.
