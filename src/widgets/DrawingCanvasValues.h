#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace drawing_canvas {

double finiteNumber(double value, double fallback);
double finiteNumber(const QVariant &value, double fallback);
bool readFinite(const QVariantMap &source, const QString &field, double &target);

} // namespace drawing_canvas
