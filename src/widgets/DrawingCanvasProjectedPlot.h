#pragma once

#include <QVariantMap>

#include <vector>

namespace drawing_canvas {

struct DrawingCanvasProjectedSegment {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
};

struct DrawingCanvasProjectedPlotPreview {
    std::vector<DrawingCanvasProjectedSegment> travelSegments;
    std::vector<DrawingCanvasProjectedSegment> strokeSegments;
    bool hasPlotBounds = false;
};

DrawingCanvasProjectedPlotPreview projectedPlotPreview(const QVariantMap &plotSummary);

} // namespace drawing_canvas
