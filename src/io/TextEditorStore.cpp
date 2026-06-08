#include "TextEditorStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <utility>

TextEditorStore::TextEditorStore(QString manifestPath, QObject *parent)
    : QObject(parent)
    , m_manifestPath(std::move(manifestPath))
{
}

QVariantMap TextEditorStore::save(const QVariantList &documents, const QString &activeId, bool saveAll, const QVariantMap &editorState) const
{
    Q_UNUSED(activeId)
    Q_UNUSED(saveAll)
    Q_UNUSED(editorState)
    return {
        {QStringLiteral("ok"), false},
        {QStringLiteral("message"), QStringLiteral("text manifest format unavailable")},
        {QStringLiteral("documents"), documents},
    };
}

QString TextEditorStore::path() const
{
    return m_manifestPath;
}

QVariantMap TextEditorStore::exportBundle(const QVariantList &documents, const QString &activeId, const QVariantMap &metadata) const
{
    Q_UNUSED(documents)
    Q_UNUSED(activeId)
    Q_UNUSED(metadata)
    return {
        {QStringLiteral("ok"), false},
        {QStringLiteral("message"), QStringLiteral("text export format unavailable")},
    };
}

QVariantList TextEditorStore::load() const
{
    return {};
}

QVariantMap TextEditorStore::loadState() const
{
    return {};
}

bool TextEditorStore::saveState(const QVariantMap &editorState) const
{
    Q_UNUSED(editorState)
    return false;
}

QString TextEditorStore::cleanRelativePath(const QString &path)
{
    const QString clean = QDir::cleanPath(path.trimmed());
    if (clean.isEmpty() || QFileInfo(clean).isAbsolute() || clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../"))) {
        return {};
    }
    return clean;
}

QString TextEditorStore::safeFileStem(const QString &text)
{
    QString result = text.trimmed().toLower();
    result.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("_"));
    result = result.trimmed();
    return result.isEmpty() ? QStringLiteral("untitled") : result.left(80);
}

QString TextEditorStore::safeExportFileName(const QString &name, const QString &fallbackId)
{
    QString stem = safeFileStem(QFileInfo(name).completeBaseName());
    if (stem == QStringLiteral("untitled")) {
        stem = safeFileStem(fallbackId);
    }
    return stem + QStringLiteral(".txt");
}

QString TextEditorStore::normalizedDocumentRole(const QString &role)
{
    const QString clean = role.trimmed().toLower();
    return clean.isEmpty() ? QStringLiteral("context") : clean;
}

bool TextEditorStore::writeUtf8File(const QString &path, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

QString TextEditorStore::sha256Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

QVariantMap TextEditorStore::normalizeEditorState(const QVariantMap &source)
{
    return source;
}
