# Closeout — the asset-zoo disk store (the file layer for the catalog)

> Freezes a boundary so future work does not re-litigate it.

- **Boundary**: WHERE the asset zoo persists to disk, and the missing-vs-corrupt contract.
- **Department**: edi-drafting
- **Campaign**: drafting-20260619-zoo-store (pillar A — the asset zoo, the backbone)
- **Date**: 2026-06-19
- **Commit**: `e121b9f` on `dept/drafting` (not pushed — hub owns the origin bridge)

## The decision
The asset zoo's disk persistence lives in **`src/io/AssetZooStore.{h,cpp}`**, namespace
`edi::io`, as free functions over the plain `edi::zoo::AssetZoo` struct:
- `defaultAssetZooPath()` → `appConfigFilePath("edi-zoo.edizoo")` (per-user, via
  `QStandardPaths::AppConfigLocation`; no hardcoded absolute path).
- `saveAssetZoo(path, zoo) -> FormatResult<bool>` — **atomic** (`QSaveFile`
  write-temp-then-rename + `commit`), mkpath the parent, binary flags
  `WriteOnly|Truncate` (no `Text`), `encodeAssetZoo` bytes.
- `loadAssetZoo(path) -> FormatResult<AssetZoo>` — **missing file ⇒ empty zoo, ok=true**
  ("no catalog yet"); **existing file ⇒ `decodeAssetZoo` verbatim** so corrupt surfaces.

`edi_zoo_core` stays **Qt-free** (links only `edi_format_core`); the Qt file layer is
ONLY in `src/io`. `FormatResultCode` gained an additive `IoError` enumerator for
disk-level failures.

## Why (the reasoning that must NOT be re-argued)
- **src/io, not src/zoo**: file I/O needs Qt (QFile/QSaveFile/QStandardPaths); putting it
  in the zoo core would poison its no-Qt link line. Same split as
  SettingsStore/DrawingDocumentStore. The core stays a pure data+codec library.
- **Atomic via QSaveFile**: a plain truncate+write can leave a half-written, corrupt
  catalog on a mid-write crash. QSaveFile makes the swap all-or-nothing.
- **Missing ≠ corrupt**: a genuinely absent file is the legitimate baseline (empty zoo).
  An existing-but-unreadable file is data loss and must surface — this deliberately does
  NOT copy `loadSettingsFromPath`'s habit of collapsing unparseable into empty. A
  zero-byte existing file is therefore an **error** (exists-but-empty == corruption); the
  test pins it.
- **Path through `appConfigFilePath`**: SettingsStore documents it as the single place the
  config root is decided — every store routes through it so the layout has one chokepoint.

## The contract (what future work must respect)
- Keep `edi_zoo_core` Qt-free; any new disk/Qt behavior for the zoo goes in `src/io`.
- Reads stay **additive + tolerant** at the codec layer (no EDIM envelope/version bump) —
  later `AssetRecord` fields (sockets, the metadata grid) load into old files by default.
- `loadAssetZoo` must keep forwarding decode errors verbatim — do not collapse a corrupt
  file to an empty zoo.
- New disk-failure paths use `FormatResultCode::IoError`.

## Out of scope / explicitly NOT allowed
- No controller/UI wiring yet — no consumer reads/writes the catalog through this store
  (that arrives with the assetRef↔catalog wiring slice).
- No JSON; MessagePack only, via the existing codec.

## Queued next in this campaign (need hub go)
1. **Sockets** on `AssetRecord` — named attach points (`name` · `type` · local `anchor`
   Point2D); additive (no version bump).
2. **assetRef ↔ catalog wiring** — make a drawing's `DraftingBlock.assetRef` resolve/
   validate against the zoo (closes the dangling-string loop).

## Pointers
- Code: `src/io/AssetZooStore.{h,cpp}`, `src/formats/FormatResult.{h,cpp}` (IoError)
- Tests: `tests/asset_zoo_store_tests.cpp` (round-trip / missing=empty / zero-byte=error /
  bad-magic=error / mkpath), registered in `CMakeLists.txt`
- Reuses: `src/zoo/AssetZooSerialize.*` (codec), `src/io/SettingsStore.*`
  (`appConfigFilePath`), `src/io/DrawingDocumentStore.cpp` (atomic-write reference)
- Related charters: `docs/departments/edi-drafting.md`; vision in
  `~/dept-bus/edi-architecture-tool-vision.md`
