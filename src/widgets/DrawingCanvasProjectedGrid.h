#pragma once

#include <QString>
#include <QVariantMap>

#include <vector>

namespace drawing_canvas {

struct DrawingCanvasProjectedGridLine {
    QString axis;
    double position = 0.0;
    bool major = false;
};

struct DrawingCanvasProjectedGridBounds {
    bool visible = false;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct DrawingCanvasProjectedGridPoint {
    bool visible = false;
    double x = 0.0;
    double y = 0.0;
};

struct DrawingCanvasProjectedGrid {
    std::vector<DrawingCanvasProjectedGridLine> lines;
    DrawingCanvasProjectedGridBounds drawableBounds;
    DrawingCanvasProjectedGridPoint origin;
};

DrawingCanvasProjectedGrid projectedGrid(const QVariantMap &model);

} // namespace drawing_canvas
