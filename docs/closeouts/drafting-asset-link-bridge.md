# Closeout — the asset-link bridge (cross-core wiring pattern)

> Freezes a boundary so future work does not re-litigate it.

- **Boundary**: WHERE code that needs BOTH the drafting core and the zoo core lives — the cross-core meeting-point — and the assetRef→catalog resolver that establishes it.
- **Department**: edi-drafting
- **Campaign**: drafting-20260619-zoo-store (pillar A — the asset zoo), slice A4
- **Date**: 2026-06-19
- **Commit**: `c842554` on `dept/drafting` (not pushed — hub owns the origin bridge)

## The decision
Code that must touch BOTH `edi_drafting_core` (a drawing) and `edi_zoo_core` (the
catalog) lives in a **dedicated bridge lib**, never inside either core. For the assetRef
resolver this is the new lib **`edi_asset_link`** (`src/assetlink/`, namespace
`edi::assetlink`), linking `edi_drafting_core` + `edi_zoo_core`.

Three pure, Qt-free free functions:
```cpp
const zoo::AssetRecord *resolveAssetRef(const zoo::AssetZoo&, const std::string& ref);
std::set<std::string>   collectDocumentAssetRefs(const drafting::DraftingDocument&);
DocumentAssetRefValidation validateDocumentAssetRefs(const drafting::DraftingDocument&,
                                                     const zoo::AssetZoo&);
// DocumentAssetRefValidation { vector<AssetRefStatus{ref,resolved,curated}> refs;
//                              vector<string> unresolved; vector<string> uncurated; }
```
The document model and the zoo are **UNCHANGED**: `assetRef` stays a neutral `std::string`
on `DraftingBlock` and `BlockPlacementMetadata`; resolution is a read-side QUERY.

## Why (the reasoning that must NOT be re-argued)
- **Neither core may link the other.** `edi_drafting_core` and `edi_zoo_core` each link
  only `edi_format_core` — deliberate isolation (the zoo is a cross-document catalog, the
  drafting core a single drawing). A resolver needs both types, so it CANNOT live in either
  without creating the forbidden cross-core dep. It belongs at a higher layer.
- **A dedicated bridge, not `edi_app_core`.** app_core is the workspace/session layer
  (AppState, ProjectWorkspace); pulling the whole zoo into it for catalog queries would
  widen its dep surface for an unrelated concern. A small named bridge makes the
  drafting↔zoo edge VISIBLE in the dependency graph instead of hiding it. Precedent:
  `edi_recipe_core` is the same shape (a small lib that links the drafting core because
  recipes resolve against the document).
- **Query, not type change.** Resolution is computed on demand and returns VALUES (refs +
  bools). It does NOT cache a resolved pointer / id back into the document — that would
  smuggle a live back-reference into a model whose whole design is neutral snapshot strings
  (the FLATTEN no-live-link discipline). Keeps the slice behavior-preserving.
- **unresolved vs uncurated are distinct.** UNRESOLVED = the ref points at nothing (a
  dangling error); resolved-but-UNCURATED = the asset exists but is not greenlit (a softer
  warning). The campaign's whole point is the curation signal — do not collapse them.
- **`std::set` for collect** gives dedup + deterministic order for free (stable tests/UI).
  Placement refs counted only where `instanceId` is non-empty (an empty instanceId is an
  ordinary object, not a placement); empty assetRefs excluded on both paths.

## The contract (what future work must respect)
- Keep both cores isolated: NO `zoo/` include in `src/drafting`, NO `drafting/` include in
  `src/zoo`. Anything needing both goes in `edi_asset_link` (or another bridge above both).
- `edi_asset_link` stays Qt-free, pure free functions over plain structs.
- Resolution stays a read-side query; never cache a resolved pointer/id into the document.
- `resolveAssetRef`'s returned pointer aliases the zoo and dangles on zoo mutation (parity
  with findAsset/findSocket) — read what you need immediately, don't retain it.

## Out of scope / explicitly NOT allowed
- No controller/UI wiring yet — nothing surfaces this validation to the user (a natural
  follow-up: a document-controller check or a UI warning on dangling/uncurated refs).
- No document-model or zoo field change; assetRef stays a neutral string.
- No JSON.

## Pillar-A campaign status (drafting-20260619-zoo-store)
- A2 zoo store (disk persistence) — DONE (`docs/closeouts/drafting-asset-zoo-store.md`)
- A3 sockets on AssetRecord — DONE (`docs/closeouts/drafting-asset-zoo-sockets.md`)
- A4 assetRef→catalog resolver — DONE (this doc)
- Backlog (need hub go): surface validation in the controller/UI; instance re-forming for
  the engine export (Seam C) would also live in / above `edi_asset_link`.

## Pointers
- Code: `src/assetlink/AssetLinkResolve.{h,cpp}`, CMake `edi_asset_link` lib + test
- Tests: `tests/asset_link_resolve_tests.cpp` (resolve hit/miss/empty, collect dedup +
  exclusions, dangling, curated/uncurated split)
- Reads: `DraftingBlock.assetRef` (DraftingMapTypes.h:236), `BlockPlacementMetadata`
  (DraftingMapTypes.h:70-83), `ObjectMetadata.blockPlacement` (DraftingTypes.h:185),
  `findAsset`/`AssetRecord.curated` (src/zoo)
- Related: `docs/departments/edi-drafting.md`, vision
  `~/dept-bus/edi-architecture-tool-vision.md`
