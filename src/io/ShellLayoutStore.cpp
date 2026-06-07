#include "ShellLayoutStore.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <utility>

ShellLayoutStore::ShellLayoutStore(QString path, QObject *parent)
    : QObject(parent),
      m_path(std::move(path)) {}

bool ShellLayoutStore::save(const QVariantMap &layout) const {
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonObject object = QJsonObject::fromVariantMap(layout);
    const QJsonDocument document(object);
    file.write(document.toJson(QJsonDocument::Indented));
    return file.commit();
}

QString ShellLayoutStore::path() const {
    return m_path;
}
