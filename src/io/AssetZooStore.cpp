#include "io/AssetZooStore.h"

#include "io/SettingsStore.h" // appConfigFilePath — the single config-root decision
#include "zoo/AssetZooSerialize.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace edi::io {

namespace {

// The catalog filename is DATA, not an inline literal scattered across the
// store. ".edizoo" is the zoo's own extension, distinct from drawings (.edidraw)
// and settings (.toml), so the catalog file is self-identifying on disk.
// QStringLiteral needs a string-literal token at its call site, so the name is a
// builder function rather than a stored QString constant.
QString assetZooFileName()
{
    return QStringLiteral("edi-zoo.edizoo");
}

} // namespace

QString defaultAssetZooPath()
{
    return appConfigFilePath(assetZooFileName());
}

edi::formats::FormatResult<bool> saveAssetZoo(const QString &path, const edi::zoo::AssetZoo &zoo)
{
    using edi::formats::FormatResult;
    using edi::formats::FormatResultCode;

    if (path.trimmed().isEmpty()) {
        return FormatResult<bool>::failure(path.toStdString(), FormatResultCode::IoError,
                                           "asset zoo path is empty");
    }

    // Mirror DrawingDocumentStore::writeBinaryFile: ensure the parent dir exists,
    // then write atomically (QSaveFile commits via temp-then-rename).
    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        return FormatResult<bool>::failure(path.toStdString(), FormatResultCode::IoError,
                                           "could not create asset zoo directory");
    }

    const edi::formats::ByteBuffer buffer = edi::zoo::encodeAssetZoo(zoo);
    const QByteArray bytes(reinterpret_cast<const char *>(buffer.data()),
                           static_cast<int>(buffer.size()));

    QSaveFile file(path);
    // Binary payload — NO QIODevice::Text (it would mangle newline bytes).
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return FormatResult<bool>::failure(path.toStdString(), FormatResultCode::IoError,
                                           "could not open asset zoo file for writing");
    }
    file.write(bytes);
    if (!file.commit()) {
        return FormatResult<bool>::failure(path.toStdString(), FormatResultCode::IoError,
                                           "could not commit asset zoo file");
    }
    return FormatResult<bool>::success(true);
}

edi::formats::FormatResult<edi::zoo::AssetZoo> loadAssetZoo(const QString &path)
{
    using edi::formats::FormatResult;
    using edi::formats::FormatResultCode;
    using edi::zoo::AssetZoo;

    // A genuinely absent file is the "no catalog yet" baseline, not a failure —
    // an empty zoo is the correct starting point. An existing-but-broken file is
    // handled below by decodeAssetZoo (which surfaces empty/bad-magic as errors).
    if (!QFileInfo::exists(path)) {
        return FormatResult<AssetZoo>::success(AssetZoo{});
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return FormatResult<AssetZoo>::failure(path.toStdString(), FormatResultCode::IoError,
                                               "could not open asset zoo file for reading");
    }
    const QByteArray raw = file.readAll();
    const edi::formats::ByteBuffer bytes(
        reinterpret_cast<const std::uint8_t *>(raw.constData()),
        reinterpret_cast<const std::uint8_t *>(raw.constData()) + raw.size());
    // Decode verbatim: a zero-byte or bad-magic EXISTING file is corruption, so
    // its decode error must propagate (do NOT collapse to an empty zoo).
    return edi::zoo::decodeAssetZoo(bytes, path.toStdString());
}

} // namespace edi::io
