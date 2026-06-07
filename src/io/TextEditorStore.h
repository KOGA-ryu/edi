#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QVariantList>
#include <QVariantMap>

class TextEditorStore final : public QObject {
    Q_OBJECT

public:
    explicit TextEditorStore(QString manifestPath, QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap save(const QVariantList &documents, const QString &activeId, bool saveAll,
                                const QVariantMap &editorState = QVariantMap()) const;
    Q_INVOKABLE QString path() const;
    Q_INVOKABLE QVariantMap exportBundle(const QVariantList &documents, const QString &activeId, const QVariantMap &metadata) const;
    QVariantList load() const;
    Q_INVOKABLE QVariantMap loadState() const;
    Q_INVOKABLE bool saveState(const QVariantMap &editorState) const;

private:
    static QString cleanRelativePath(const QString &path);
    static QString safeFileStem(const QString &text);
    static QString safeExportFileName(const QString &name, const QString &fallbackId);
    static QString normalizedDocumentRole(const QString &role);
    static bool writeUtf8File(const QString &path, const QString &text);
    static QString sha256Hex(const QByteArray &data);
    static QJsonObject packetFileRecord(const QString &path, const QString &sha256, qsizetype bytes);
    static QVariantMap normalizeEditorState(const QVariantMap &source);

    QString m_manifestPath;
};
