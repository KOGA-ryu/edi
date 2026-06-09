#pragma once

#include <QString>
#include <QVariantMap>

namespace drawing_canvas {

QVariantMap projectedObjectById(const QVariantMap &model, const QString &objectId);

} // namespace drawing_canvas
