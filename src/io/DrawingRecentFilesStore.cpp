#include "DrawingRecentFilesStore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QSaveFile>
#include <QUrl>
#include <utility>

DrawingRecentFilesStore::DrawingRecentFilesStore(QString path, QObject *parent)
    : QObject(parent),
      m_path(std::move(path)) {}

QVariantList DrawingRecentFilesStore::load() const {
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    return recentListFromArray(document.object().value(QStringLiteral("recent_drawings")).toArray());
}

QVariantMap DrawingRecentFilesStore::add(const QString &path) const {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("message"), QStringLiteral("recent drawing unavailable"));
    result.insert(QStringLiteral("files"), load());

    const QString normalizedPath = normalizedLocalPath(path);
    if (normalizedPath.isEmpty()) {
        result.insert(QStringLiteral("message"), QStringLiteral("recent drawing path missing"));
        return result;
    }

QString DrawingRecentFilesStore::path() const {
    return m_path;
}

QString DrawingRecentFilesStore::normalizedLocalPath(const QString &path) {
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

