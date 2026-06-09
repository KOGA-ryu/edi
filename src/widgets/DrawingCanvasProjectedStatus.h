#pragma once

#include <QString>
#include <QVariantMap>

namespace drawing_canvas {

struct DrawingCanvasProjectedStatus {
    double gridWidth = 0.0;
    double gridHeight = 0.0;
    QString gridUnitLabel;
    QString plotStatus = QStringLiteral("blocked");
    int plotWarningCount = 0;
    QString firstWarningKind;
};

DrawingCanvasProjectedStatus projectedCanvasStatus(const QVariantMap &model);

} // namespace drawing_canvas
