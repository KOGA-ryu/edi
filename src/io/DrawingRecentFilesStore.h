#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class DrawingRecentFilesStore final : public QObject {
    Q_OBJECT

public:
    explicit DrawingRecentFilesStore(QString path, QObject *parent = nullptr);

    Q_INVOKABLE QVariantList load() const;
    Q_INVOKABLE QVariantMap add(const QString &path) const;
    Q_INVOKABLE QString path() const;

private:
    static constexpr int maxRecentFiles = 12;
    QString m_path;

    static QString normalizedLocalPath(const QString &path);
    static QVariantList recentListFromArray(const QJsonArray &array);
    bool writeDocument(const QJsonObject &document) const;
};
