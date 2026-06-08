#include "DrawingRecentFilesStore.h"

#include <QFileInfo>
#include <QUrl>

#include <utility>

DrawingRecentFilesStore::DrawingRecentFilesStore(QString path, QObject *parent)
    : QObject(parent)
    , m_path(std::move(path))
{
}

QVariantList DrawingRecentFilesStore::load() const
{
    return {};
}

QVariantMap DrawingRecentFilesStore::add(const QString &path) const
{
    const QString normalizedPath = normalizedLocalPath(path);
    return {
        {QStringLiteral("ok"), !normalizedPath.isEmpty()},
        {QStringLiteral("message"), normalizedPath.isEmpty() ? QStringLiteral("recent drawing path missing") : QStringLiteral("recent file accepted in memory")},
        {QStringLiteral("files"), QVariantList{normalizedPath}},
    };
}

QString DrawingRecentFilesStore::path() const
{
    return m_path;
}

QString DrawingRecentFilesStore::normalizedLocalPath(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    const QUrl url(trimmed);
    QString localPath = url.isLocalFile() ? url.toLocalFile() : trimmed;
    if (localPath.startsWith(QStringLiteral("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    } else {
        localPath = QUrl::fromPercentEncoding(localPath.toUtf8());
    }
    return QFileInfo(localPath).absoluteFilePath();
}
