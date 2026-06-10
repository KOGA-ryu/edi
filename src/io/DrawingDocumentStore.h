#pragma once

#include <QByteArray>
#include <QObject>
#include <QUrl>
#include <QVariantMap>

class DrawingDocumentStore final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentStore(QObject *parent = nullptr);

    // Binary drawing document I/O. save() writes the raw MessagePack bytes;
    // open() returns {ok, bytes (QByteArray), message}.
    QVariantMap save(const QUrl &url, const QByteArray &bytes) const;
    QVariantMap open(const QUrl &url) const;
    QVariantMap exportText(const QUrl &url, const QString &text) const;

private:
    static QString localPath(const QUrl &url);
    static QString bundleDirectoryPath(const QString &selectedPath);
    static bool writeTextFile(const QString &path, const QString &text);
    static bool writeBinaryFile(const QString &path, const QByteArray &bytes);
    static bool readBinaryFile(const QString &path, QByteArray &bytes);
};
