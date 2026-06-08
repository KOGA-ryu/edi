#pragma once

#include "DrawingObjectModel.h"
#include <QString>
#include <QStringList>

#include <functional>

namespace drawing_core {

inline constexpr int kDefaultCanvasPx = 512;
inline constexpr double kPi = 3.141592653589793238462643383279502884;
inline constexpr const char *kScriptLayer = "layer_09_script_geometry";

struct Point {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
    double nx = 0.0;
    double ny = 0.0;
};

struct State {
    QString scriptId = "unnamed_script";
    QString selectedTool = "anchor_points";
    QString selectedLayer = kScriptLayer;
    QString selectedObject;
    QStringList selectedObjects;
    bool gridSnap = true;
    int gridStepPx = 32;
    int canvasPx = kDefaultCanvasPx;
    QString circleArcMode = "circle";
    double circleArcStartAngleDeg = 0.0;
    double circleArcEndAngleDeg = 90.0;
    int regularPolygonSides = 6;
    double regularPolygonRotationDeg = 30.0;
    QString lineVariant = "straight";
    QString lineStyle = "solid";
    QString strokeColor = "#f4d46f";
    QString fillColor;
    double lineThickness = 2.0;
    double strokeOpacity = 1.0;
    Point pendingPoint;
    DrawingStore store;
};

struct Bounds {
    bool ok = false;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
};

