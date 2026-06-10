#include "widgets/DrawingCanvasValues.h"

#include <cmath>

namespace drawing_canvas {

double finiteNumber(double value, double fallback)
{
    return std::isfinite(value) ? value : fallback;
}

double finiteNumber(const QVariant &value, double fallback)
{
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && std::isfinite(number) ? number : fallback;
}

bool readFinite(const QVariantMap &source, const QString &field, double &target)
{
    if (!source.contains(field)) {
        return false;
    }
    bool ok = false;
    const double value = source.value(field).toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        return false;
    }
    target = value;
    return true;
}

} // namespace drawing_canvas
