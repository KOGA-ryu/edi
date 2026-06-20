# Handoff — ui-20260619-cleanup-assetlink

> Cleanup-wave campaign (HUB allocation): D17 controller→library, D08 live
> AssetZoo consumer, D07 wire edi_asset_link + validate from controller.

- **Campaign**: ui-20260619-cleanup-assetlink
- **Department**: edi-ui (on master, the integration line)
- **Goal (one line)**: factor the controller into `edi_controller_core`; give the
  shell a live AssetZoo; wire `edi_asset_link` so the controller can validate a
  document's asset refs against that zoo.
- **Boundary settled by reviewer gate below — YES on all three.**

## Gate log

### Reviewer gate — 2026-06-19 — edi-ui-reviewer (agent ae476647481d6f9c2)

KEY DISCOVERY: the 7 "sibling" `src/core/*.cpp` (DrawingCore, DrawingCoreCommon,
DrawingObjectModel, DrawingGeometry, DrawingObjectOps, DrawingCommands,
DrawingModelBuilder, DrawingSvgExport) are **empty namespace shells** (4–14 lines,
zero out-of-line defs). The real `drawing_core` impl is header-inline in
`DrawingCoreInternal.h` + inside `DrawingDocumentController.cpp` (4567 lines). That
is why test targets compile only Controller + Projection (+ Store) and still link.

**D17 — settled YES. `edi_controller_core` STATIC lib, authoritative members:**
```
src/core/DrawingCore.h  DrawingCoreInternal.h  DrawingObjectModel.h  DrawingObjectModel.cpp
src/core/DrawingCore.cpp  DrawingCoreCommon.cpp  DrawingDocumentController.cpp
src/core/DrawingDocumentProjection.h  DrawingDocumentProjection.cpp
src/core/DrawingGeometry.cpp  DrawingObjectOps.cpp  DrawingCommands.cpp
src/core/DrawingModelBuilder.cpp  DrawingSvgExport.cpp
src/io/DrawingDocumentStore.h  src/io/DrawingDocumentStore.cpp   <-- REQUIRED (controller holds m_store)
```
- Link: `target_link_libraries(edi_controller_core PUBLIC edi_drafting_core Qt6::Core)`;
  `target_include_directories(edi_controller_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)`.
  Do NOT add edi_format_core directly (transitive via edi_drafting_core).
- Include the full set (empty shells cost nothing; keeps it byte-identical to what
  the shell links today → behavior-preserving). Don't delete the shells (out of scope).
- 8 targets: replace the Controller/Projection/Store source lines with a link to
  `edi_controller_core`. Targets at CMakeLists ~L463 crypt_generator_tests, ~L491
  map_regression_lock_tests, ~L523 map_determinism_tests, ~L553 drafting_graph_sync_tests,
  ~L655 drawing_document_controller_tests, ~L673 edi_shell_window_tests, ~L761
  drawing_canvas_widget_tests, ~L812 `edi` shell. WATCH-ITEM: crypt_generator_tests +
  map_regression_lock_tests also compile MapToonExport/CryptGenerator/RoomSpecStore —
  those STAY listed per-target; don't over-sweep.
- No symbol-set change to any target; no golden re-bless (zero pixels). Net WIN:
  the 4567-line TU compiles once instead of 8×.

**D08 — settled YES (look-flag).** Hold `edi::zoo::AssetZoo m_assetZoo;` as a value
member on **EdiShellWindow** (NOT the controller — that would invert core→zoo dep).
Best-effort load at startup in app/main.cpp beside loadSettings/loadWorkspaceLayout/
loadTextSession (after ~L364): `edi::io::loadAssetZoo(edi::io::defaultAssetZooPath())`;
on FormatResult failure fall back to **empty `AssetZoo{}`**, NOT testerAssetCatalog()
(that's fixture data, would pollute D07 validation). Expose `const AssetZoo& assetZoo() const`
getter. NO visible UI surface — the look is the user's; a zoo panel is a separate brief.
No new CMake link (shell already links edi_zoo_core + compiles AssetZooStore).

**D07 — settled YES (look-flag).** Dependency dir legal: controller → edi_asset_link
→ {drafting_core, zoo_core}; no core includes assetlink. Add `edi_asset_link` **PRIVATE**
to `edi_controller_core` (validation is an impl detail, not in the public header); the
shell gets it transitively — do NOT double-list on the shell target. Expose an
**on-demand const query method** (NOT on modelChanged — 43 emit sites, O(doc) scan
per emit is the forbidden pattern):
```
edi::assetlink::DocumentAssetRefValidation
DrawingDocumentController::validateAssetRefs(const edi::zoo::AssetZoo &zoo) const;
```
computes against m_document, returns by value, no member/no signal/no caching. The
window passes its m_assetZoo in. Surfacing unresolved/uncurated warnings as chrome is
a DEFERRED user-look decision — expose the data, paint NO new surface.

**Sequence: D17 → D08 → D07.** One commit each, working tree clean between. All three
touch shared rebase-contract files (CMakeLists.txt: D17 heavy/D07 light; app/main.cpp:
D08; EdiShellWindow: D08+D07) — serialize on master, don't parallelize.

### Builder batch — 2026-06-19 — edi-ui-builder (afc98a403992ee9ef)
- Slices: D17 `f6725e6` (edi_controller_core lib + 8 targets link it) · D08 `a0fec6b`
  (m_assetZoo on EdiShellWindow, best-effort load, empty-on-miss, getter) · D07
  `4688714` (validateAssetRefs(zoo) const query; edi_asset_link PRIVATE on lib).
- Green gate: clean build + ctest 120/120 + scan, plus a from-scratch /tmp build
  confirming all 8 targets link with no undefined refs (no symbol set changed).
- Deviation (in-scope): added AssetZooStore.cpp + edi_zoo_core to edi_shell_window_tests
  because EdiShellWindowIo.cpp now calls loadAssetZoo. Reviewer confirmed minimal/correct.
- Noticed-but-didn't-do: no warning chrome (D07), no zoo panel (D08) — deferred look
  decisions; no empty-shell deletion (kept byte-identical archive); no magic dims.

### Reviewer-accept gate — 2026-06-19 — edi-ui-reviewer (aed2743612dd1979c)
- **ACCEPT all three. No must-fix.** Re-ran build + ctest 121/121 (post drafting-merge).
- D17: membership/link/8-target edits exact; watch-item honored (MapToonExport/Crypt/
  RoomSpec stay per-target); behavior-preserving.
- D08: zoo on the window not the controller (no core→zoo inversion); empty-on-miss
  verified; NO look surface; no dangling (assetZoo() has zero callers yet).
- D07 SIGNAL-SAFETY PASS: validateAssetRefs has ZERO call sites — not wired to any of
  the 44 modelChanged emits; no member/cache/signal; returns by value (no aliasing).
  edi_asset_link PRIVATE; header exposure limited to the by-value return type. Legal acyclic dep.

**CAMPAIGN COMPLETE** — all 3 slices landed + reviewer-accepted on master @4688714.

## Open questions / blockers
- DEFERRED user-look decisions to surface to the user (NOT built in this campaign):
  (1) a visible AssetZoo browser/panel; (2) a warning surface for unresolved/uncurated
  asset refs. Both await a look brief.

## Next
- Brief edi-ui-builder with the batch (D17 → D08 → D07). Then reviewer-accept the diff.
