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

DrawingCanvasProjectedPointer projectedPointer(const QVariantMap &model);

} // namespace drawing_canvas
