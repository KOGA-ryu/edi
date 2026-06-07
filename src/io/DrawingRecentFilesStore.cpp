#include "DrawingRecentFilesStore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
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
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
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

    QJsonArray recent;
    QJsonObject entry;
    entry.insert(QStringLiteral("path"), normalizedPath);
    entry.insert(QStringLiteral("label"), QFileInfo(normalizedPath).fileName());
    entry.insert(QStringLiteral("last_used_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    recent.append(entry);

    const QVariantList existing = load();
    for (const QVariant &value : existing) {
        const QVariantMap item = value.toMap();
        const QString existingPath = item.value(QStringLiteral("path")).toString();
        if (existingPath.isEmpty() || existingPath == normalizedPath) {
            continue;
        }
        QJsonObject object;
        object.insert(QStringLiteral("path"), existingPath);
        QString label = item.value(QStringLiteral("label")).toString();
        if (label.isEmpty()) {
            label = QFileInfo(existingPath).fileName();
        }
        object.insert(QStringLiteral("label"), label);
        object.insert(QStringLiteral("last_used_at"), item.value(QStringLiteral("last_used_at")).toString());
        recent.append(object);
        if (recent.size() >= maxRecentFiles) {
            break;
        }
    }

    QJsonObject document;
    document.insert(QStringLiteral("recent_drawings"), recent);
    if (!writeDocument(document)) {
        result.insert(QStringLiteral("message"), QStringLiteral("recent drawing write failed"));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("message"), QStringLiteral("updated recent drawings"));
    result.insert(QStringLiteral("files"), recentListFromArray(recent));
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

QVariantList DrawingRecentFilesStore::recentListFromArray(const QJsonArray &array) {
    QVariantList result;
    QStringList seen;
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        const QString path = normalizedLocalPath(object.value(QStringLiteral("path")).toString());
        if (path.isEmpty() || seen.contains(path)) {
            continue;
        }
        seen.append(path);
        QVariantMap item;
        item.insert(QStringLiteral("path"), path);
        item.insert(QStringLiteral("label"), object.value(QStringLiteral("label")).toString(QFileInfo(path).fileName()));
        item.insert(QStringLiteral("last_used_at"), object.value(QStringLiteral("last_used_at")).toString());
        result.append(item);
        if (result.size() >= maxRecentFiles) {
            break;
        }
    }
    return result;
}

bool DrawingRecentFilesStore::writeDocument(const QJsonObject &document) const {
    const QFileInfo info(m_path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        return false;
    }
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(QJsonDocument(document).toJson(QJsonDocument::Indented));
    return file.commit();
}
