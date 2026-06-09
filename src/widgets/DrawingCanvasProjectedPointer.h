#pragma once

#include <QString>
#include <QVariantMap>

namespace drawing_canvas {

struct DrawingCanvasProjectedPointer {
    bool visible = false;
    double snappedX = 0.0;
    double snappedY = 0.0;
    QString kind;
    QString source;
    bool insideDrawable = false;
    QString label;
    QString sourceObjectId;
};

struct DrawingCanvasProjectedGuideDragSnapIntent {
    bool visible = false;
    double rawX = 0.0;
    double rawY = 0.0;
    double snappedX = 0.0;
    double snappedY = 0.0;
    QString label = QStringLiteral("anchor");
    QString sourceObjectId;
    bool intersection = false;
};

DrawingCanvasProjectedPointer projectedPointer(const QVariantMap &model);
DrawingCanvasProjectedGuideDragSnapIntent projectedGuideDragSnapIntent(const QVariantMap &model);

} // namespace drawing_canvas
