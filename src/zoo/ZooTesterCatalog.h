#pragma once

#include "zoo/AssetZoo.h"

#include <vector>

namespace edi::zoo {

// The realize chain's CURATED tester catalog — the forge→bake→mint handoff's
// fixture, as DATA. It is a pure declarative table of AssetRecords (no Qt, no
// persistence, no I/O), so a test can assert the exact records the manifest will
// carry WITHOUT spinning a QCoreApplication or touching disk. The Qt persistence
// (saveAssetZoo + exportZooToToon to files) is a separate CLI seam — the same
// pure-core / Qt-shell split the whole codebase keeps (cf. SettingsStore: pure
// logic vs. QFile layer).
//
// Each record's `meshRef` is a CATALOG-RELATIVE POSIX key (e.g.
// "meshes/star6.blend"), NOT an absolute path: the realizer later resolves it
// against the manifest's directory (`--asset-dir`), so the fixture stays portable
// across checkouts. Only CURATED records are minted here — the manifest exporter
// emits curated-only, so an uncurated row would never reach the realizer anyway.
//
// R1c is DEPTH-FIRST: it proves the whole forge→bake→mint→persist→manifest chain
// on ONE shape (star6). R1d grows this table to the other five tester shapes.
std::vector<AssetRecord> testerAssetCatalog();

} // namespace edi::zoo
