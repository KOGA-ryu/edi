#pragma once

#include "formats/FormatResult.h"
#include "zoo/AssetZoo.h"

#include <QString>

namespace edi::io {

// Disk persistence for the asset zoo catalog. The pure codec lives in
// edi_zoo_core (encodeAssetZoo/decodeAssetZoo); this module is the ONLY Qt file
// layer over it, mirroring SettingsStore's split (pure logic vs. QFile/QSaveFile).

// Default per-user catalog path: <AppConfigLocation>/<kAssetZooFileName>, derived
// through appConfigFilePath() so the config root is decided in one place.
QString defaultAssetZooPath();

// Atomic write of the zoo (encodeAssetZoo -> QSaveFile write-temp-then-rename).
// I/O failure (empty path, mkpath, open, commit) => IoError.
edi::formats::FormatResult<bool> saveAssetZoo(const QString &path, const edi::zoo::AssetZoo &zoo);

// Load the catalog. A MISSING file is NOT an error => an EMPTY zoo (the "no
// catalog yet" baseline). An existing file is decoded verbatim, so a
// corrupt/empty/bad-magic file surfaces as the decode error.
edi::formats::FormatResult<edi::zoo::AssetZoo> loadAssetZoo(const QString &path);

} // namespace edi::io
