#include "DrawingDocumentStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

DrawingDocumentStore::DrawingDocumentStore(QObject *parent)
    : QObject(parent)
{
}

QVariantMap DrawingDocumentStore::save(const QUrl &url, const QByteArray &bytes) const
{
    const QString path = localPath(url);
    if (path.isEmpty()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QStringLiteral("path missing")}};
    }
    const bool ok = writeBinaryFile(path, bytes);
    return {
        {QStringLiteral("ok"), ok},
        {QStringLiteral("message"), ok ? QStringLiteral("drawing saved") : QStringLiteral("drawing save failed")},
    };
}

QVariantMap DrawingDocumentStore::open(const QUrl &url) const
{
    const QString path = localPath(url);
    if (path.isEmpty()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QStringLiteral("path missing")}};
    }
    QByteArray bytes;
    if (!readBinaryFile(path, bytes)) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QStringLiteral("drawing open failed")}};
    }
    return {
        {QStringLiteral("ok"), true},
        {QStringLiteral("bytes"), bytes},
        {QStringLiteral("message"), QStringLiteral("drawing opened")},
    };
}

QVariantMap DrawingDocumentStore::exportText(const QUrl &url, const QString &text) const
{
    const QString path = localPath(url);
    if (path.isEmpty()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QStringLiteral("path missing")}};
    }
    return {
        {QStringLiteral("ok"), writeTextFile(path, text)},
        {QStringLiteral("message"), QStringLiteral("text export attempted")},
    };
}

QString DrawingDocumentStore::localPath(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

QString DrawingDocumentStore::bundleDirectoryPath(const QString &selectedPath)
{
    return QFileInfo(selectedPath).absolutePath();
}

bool DrawingDocumentStore::writeTextFile(const QString &path, const QString &text)
{
    if (path.trimmed().isEmpty()) {
        return false;
    }
    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return file.commit();
}

bool DrawingDocumentStore::writeBinaryFile(const QString &path, const QByteArray &bytes)
{
    if (path.trimmed().isEmpty()) {
        return false;
    }
    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(bytes);
    return file.commit();
}

bool DrawingDocumentStore::readBinaryFile(const QString &path, QByteArray &bytes)
{
    if (path.trimmed().isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    bytes = file.readAll();
    return true;
}
