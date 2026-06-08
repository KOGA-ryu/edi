#include "DrawingDocumentStore.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

DrawingDocumentStore::DrawingDocumentStore(QObject *parent)
    : QObject(parent)
{
}

QVariantMap DrawingDocumentStore::save(const QUrl &url, const QVariantMap &model) const
{
    Q_UNUSED(model)
    return {
        {QStringLiteral("ok"), false},
        {QStringLiteral("message"), localPath(url).isEmpty() ? QStringLiteral("path missing") : QStringLiteral("drawing save format unavailable")},
    };
}

QVariantMap DrawingDocumentStore::open(const QUrl &url) const
{
    return {
        {QStringLiteral("ok"), false},
        {QStringLiteral("message"), localPath(url).isEmpty() ? QStringLiteral("path missing") : QStringLiteral("drawing open format unavailable")},
    };
}

QVariantMap DrawingDocumentStore::exportSvg(const QUrl &url, const QString &svg) const
{
    const QString path = localPath(url);
    if (path.isEmpty()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QStringLiteral("path missing")}};
    }
    return {
        {QStringLiteral("ok"), writeTextFile(path, svg)},
        {QStringLiteral("message"), QStringLiteral("svg export attempted")},
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
