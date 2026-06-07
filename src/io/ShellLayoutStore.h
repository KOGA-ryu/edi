#pragma once

#include <QObject>
#include <QVariantMap>

class ShellLayoutStore final : public QObject {
    Q_OBJECT

public:
    explicit ShellLayoutStore(QString path, QObject *parent = nullptr);

    Q_INVOKABLE bool save(const QVariantMap &layout) const;
    Q_INVOKABLE QString path() const;

private:
    QString m_path;
};
