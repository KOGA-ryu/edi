#pragma once

#include <QString>
#include <QVariantMap>

#include <vector>

namespace drawing_canvas {

struct CanvasPoint {
    double x = 0.0;
    double y = 0.0;
};

struct ScreenPoint {
    double x = 0.0;
    double y = 0.0;
};

struct CanvasObjectView {
    QVariantMap values;

    std::vector<CanvasPoint> points() const;
};

double finiteNumber(double value, double fallback);
double finiteNumber(const QVariant &value, double fallback);
CanvasPoint pointFromVariant(const QVariant &value);
QVariantMap pointToVariant(const CanvasPoint &point);

} // namespace drawing_canvas
