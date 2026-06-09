#pragma once

#include <QString>
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

struct DrawingCanvasProjectedPlotBounds {
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct DrawingCanvasProjectedBoundsOverlay {
    bool visible = false;
    DrawingCanvasProjectedPlotBounds bounds;
    bool calibratedBoundsWarning = false;
    QString warningKind;
    QString warningObjectId;
};

struct DrawingCanvasProjectedSelectionBoundsOverlay {
    bool visible = false;
    DrawingCanvasProjectedPlotBounds bounds;
    QString status;
};

DrawingCanvasProjectedPlotPreview projectedPlotPreview(const QVariantMap &plotSummary);
DrawingCanvasProjectedBoundsOverlay projectedPlotBoundsOverlay(const QVariantMap &plotSummary);
DrawingCanvasProjectedSelectionBoundsOverlay projectedSelectionBoundsOverlay(const QVariantMap &model);

} // namespace drawing_canvas
