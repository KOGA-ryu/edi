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
    static void copyIfPresent(QJsonObject &target, const QJsonObject &source, const QString &key);
    static QJsonObject objectStyleManifest(const QJsonObject &object);
    static QJsonObject objectCoordinateManifest(const QJsonObject &object);
    static QJsonArray objectTagsManifest(const QJsonObject &object);
    static QJsonObject objectMetadataManifest(const QJsonObject &object);
    static QJsonObject objectManifestEntry(const QJsonObject &object);
    static QJsonObject blenderSvgBundleManifest(const QString &bundlePath, const QJsonObject &model);
    static bool hasNonEmptyString(const QJsonObject &object, const QString &key);
    static bool hasNonEmptyTags(const QJsonObject &object);
    static bool isSupportedManifestObjectType(const QString &type);
    static QJsonObject blenderSvgBundleExportReport(const QString &bundlePath,
                                                   const QString &svgPath,
                                                   const QString &manifestPath,
                                                   const QString &scriptPath,
                                                   const QString &readmePath,
                                                   const QString &reportPath,
                                                   const QString &reportTextPath,
                                                   const QString &verifyPath,
                                                   const QJsonObject &manifest);
    static QString blenderSvgBundleExportReportText(const QJsonObject &report);
};
