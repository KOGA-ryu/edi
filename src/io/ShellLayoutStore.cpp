#include "ShellLayoutStore.h"

#include <utility>

ShellLayoutStore::ShellLayoutStore(QString path, QObject *parent)
    : QObject(parent)
    , m_path(std::move(path))
{
}

bool ShellLayoutStore::save(const QVariantMap &layout) const
{
    Q_UNUSED(layout)
    return false;
}

QString ShellLayoutStore::path() const
{
    return m_path;
}
