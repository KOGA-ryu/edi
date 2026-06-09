#pragma once

#include <QVariantMap>
#include <QString>

#include <vector>

namespace drawing_canvas {

struct DrawingCanvasProjectedPoint {
    double x = 0.0;
    double y = 0.0;
};

struct DrawingCanvasProjectedBounds {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct DrawingCanvasProjectedObjectSummary {
    QString id;
    QString kind;
    bool visible = true;
    bool plotBlocked = false;
    QString plotWarningKind;
    DrawingCanvasProjectedBounds bounds;
};

enum class DrawingCanvasProjectedHandleShape {
    Circle,
    Square,
    Diamond
};

struct DrawingCanvasProjectedHandle {
    QString id;
    double x = 0.0;
    double y = 0.0;
    bool editable = true;
    bool hasAnchor = false;
    double anchorX = 0.0;
    double anchorY = 0.0;
    double sizePx = 8.0;
    double hitTolerancePx = 14.0;
    DrawingCanvasProjectedHandleShape shape = DrawingCanvasProjectedHandleShape::Circle;
};

DrawingCanvasProjectedObjectSummary projectedObjectSummary(const QVariantMap &object);
std::vector<DrawingCanvasProjectedPoint> projectedObjectPoints(const QVariantMap &object);
std::vector<DrawingCanvasProjectedHandle> projectedObjectHandles(const QVariantMap &object);

} // namespace drawing_canvas
