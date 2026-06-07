#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <QVariantMap>

class DrawingDocumentStore final : public QObject {
    Q_OBJECT

public:
    explicit DrawingDocumentStore(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap save(const QUrl &url, const QVariantMap &model) const;
    Q_INVOKABLE QVariantMap open(const QUrl &url) const;
    Q_INVOKABLE QVariantMap exportSvg(const QUrl &url, const QString &svg) const;
    Q_INVOKABLE QVariantMap exportBlenderSvgBundle(const QUrl &url, const QString &svg, const QVariantMap &model) const;

private:
    static QString localPath(const QUrl &url);
    static QString bundleDirectoryPath(const QString &selectedPath);
    static bool writeTextFile(const QString &path, const QString &text);
};
