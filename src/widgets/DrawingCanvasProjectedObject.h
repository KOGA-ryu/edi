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

struct DrawingCanvasProjectedStyle {
    QString strokeColor = QStringLiteral("#d7dde8");
    double strokeWidth = 2.0;
    QString lineStyle = QStringLiteral("solid"); // solid | dash | dot
    double strokeOpacity = 1.0;                  // 0..1, applied as pen alpha
};

struct DrawingCanvasProjectedPointObject {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
};

struct DrawingCanvasProjectedLine {
    bool ok = false;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
};

struct DrawingCanvasProjectedRectangle {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    double rotationDeg = 0.0;
    double cornerRadius = 0.0; // N4: > 0 rounds the corners
    double inset = 0.0;        // N4: > 0 draws an inner frame outline
};

struct DrawingCanvasProjectedCircle {
    bool ok = false;
    double cx = 0.0;
    double cy = 0.0;
    double radius = 0.0;
};

struct DrawingCanvasProjectedPolygon {
    bool ok = false;
    std::vector<DrawingCanvasProjectedPoint> points;
};

enum class DrawingCanvasProjectedGuideOrientation {
    Vertical,
    Horizontal
};

struct DrawingCanvasProjectedGuide {
    bool ok = false;
    DrawingCanvasProjectedGuideOrientation orientation = DrawingCanvasProjectedGuideOrientation::Vertical;
    double position = 0.0;
    bool locked = false;
    QString color = QStringLiteral("#83aeca");
    QString dashStyle = QStringLiteral("dash");
    bool showLabel = true;
    QString label;
};

struct DrawingCanvasProjectedDimension {
    bool ok = false;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double dimensionX1 = 0.0;
    double dimensionY1 = 0.0;
    double dimensionX2 = 0.0;
    double dimensionY2 = 0.0;
    double labelX = 0.0;
    double labelY = 0.0;
    bool showLabel = true;
    QString label;
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
DrawingCanvasProjectedStyle projectedObjectStyle(const QVariantMap &object);
DrawingCanvasProjectedPointObject projectedPointObject(const QVariantMap &object);
DrawingCanvasProjectedLine projectedLine(const QVariantMap &object);
DrawingCanvasProjectedRectangle projectedRectangle(const QVariantMap &object);
DrawingCanvasProjectedCircle projectedCircle(const QVariantMap &object);
DrawingCanvasProjectedPolygon projectedPolygon(const QVariantMap &object);
DrawingCanvasProjectedGuide projectedGuide(const QVariantMap &object);
DrawingCanvasProjectedDimension projectedDimension(const QVariantMap &object);
std::vector<DrawingCanvasProjectedPoint> projectedObjectPoints(const QVariantMap &object);
std::vector<DrawingCanvasProjectedHandle> projectedObjectHandles(const QVariantMap &object);

} // namespace drawing_canvas
