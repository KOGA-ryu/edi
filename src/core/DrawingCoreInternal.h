#pragma once

#include "DrawingObjectModel.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
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
    QJsonArray commandLog;
    QJsonArray generatedObjects;
    QJsonArray errors;
};

struct Bounds {
    bool ok = false;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
};

ShapeKind objectShapeKind(const QString &kind);
ShapeKind objectShapeKind(const QJsonObject &object);

double numberAt(const QJsonObject &object, const QString &key, double fallback = 0.0);
QString stringAt(const QJsonObject &object, const QString &key, const QString &fallback = QString());
bool pendingPointActive(const QJsonObject &model);
int generatedObjectCount(const QJsonObject &model);
bool undoableInteractiveCommand(const QJsonObject &command);
QJsonArray arrayAt(const QJsonObject &object, const QString &key);
QJsonArray stringListToJsonArray(const QStringList &values);
QStringList stringListFromArray(const QJsonArray &values);

QJsonArray pointArray(double x, double y);
QJsonArray translatedPointArray(const QJsonArray &point, double dx, double dy);
QJsonArray translatedPointList(const QJsonArray &points, double dx, double dy);
void includePoint(Bounds &bounds, double x, double y);
void includePointList(Bounds &bounds, const QJsonArray &points);
Bounds normalizedBounds(const QJsonObject &object);
Bounds normalizedBoundsForObjects(const QJsonArray &objects, const QStringList &objectIds);
double clampedMoveDelta(double delta, double minValue, double maxValue);
double normalizedToPixels(double value, int canvasPx);
double degreesToRadians(double degrees);
double commandDeltaFromCommand(const State &state, const QJsonObject &command, const QString &axis, double fallback);
void translateObject(QJsonObject &object, double dxN, double dyN, double dxPx, double dyPx, int canvasPx);
void translateObjectWithState(QJsonObject &object, const State &state, double dxN, double dyN);
Point snapPoint(State &state, double x, double y);
Point pointFromArray(State &state, const QJsonArray &array);
Point pointFromCommand(State &state, const QJsonObject &command);
double distancePx(const Point &a, const Point &b);
double clampedPx(double value, int canvasPx);
double positivePx(double value, double fallback = 1.0);
void rebuildRectangle(QJsonObject &object, int canvasPx, double xPx, double yPx, double widthPx, double heightPx);
void rebuildPolygon(QJsonObject &object, int canvasPx, double cxPx, double cyPx, double radiusPx, int sides, double rotationDeg);

bool generatedObjectExists(const State &state, const QString &objectId);
void selectObject(State &state, const QString &objectId);
void selectObjects(State &state, const QStringList &objectIds);
QString nextId(const State &state, const QString &kind);
void pushObject(State &state, QJsonObject object);
void deleteObject(State &state, const QString &objectId);
void deleteObjects(State &state, const QStringList &objectIds);
QString pushClonedObject(State &state, QJsonObject object, const QString &sourceId, double dxN, double dyN, const QString &sourceKey);
void duplicateObject(State &state, const QString &objectId, double dxN, double dyN);
void duplicateObjects(State &state, const QStringList &objectIds, double dxN, double dyN);
void pasteObject(State &state, const QJsonObject &snapshot, double dxN, double dyN);
void pasteObjects(State &state, const QJsonArray &snapshots, double dxN, double dyN);
void moveObject(State &state, const QString &objectId, double dxN, double dyN);
void moveObjects(State &state, const QStringList &objectIds, double dxN, double dyN);
void updateObjectField(State &state, const QString &objectId, const QString &field, double value);
bool isEditableObjectMetadataField(const QString &field);
QJsonArray normalizedMetadataTags(const QJsonValue &value);
void setObjectMetadataField(State &state, const QString &objectId, const QString &field, const QJsonValue &value);

bool parameterNumber(const QJsonValue &value, double &result);
QString normalizedHexColor(const QString &value);
QString normalizedLineStyle(const QString &value);
void applyActiveStyle(State &state, QJsonObject &object);
void setToolParameter(State &state, const QString &parameter, const QJsonValue &value);
void addPointObject(State &state, const Point &point, const QString &kind, const QString &label, const QString &detail);
void addPoint(State &state, const Point &point);
void addLineObject(State &state, const Point &start, const Point &end, const QString &kind, const QString &label, const QString &detail);
void addLine(State &state, const Point &start, const Point &end);
void addPolyline(State &state, const QJsonArray &rawPoints);
void addCircle(State &state, const Point &center, const QJsonObject &command);
void addArc(State &state, const Point &center, const QJsonObject &command);
void addRectangleObject(State &state, const Point &start, const Point &end, const QString &kind, const QString &label, const QString &detail);
void addRectangle(State &state, const Point &start, const Point &end);
void addPolygon(State &state, const Point &center, const QJsonObject &command);
void runTwoPointTool(State &state, const Point &point, const std::function<void(const Point &, const Point &)> &complete);
void handleClickCanvas(State &state, const QJsonObject &command);
bool runCommand(State &state, const QJsonObject &command);

QJsonObject objectCounts(const QJsonArray &objects);
QJsonObject pendingPointObject(const Point &point);
QJsonArray validationRows(const State &state);
QJsonObject buildModel(const State &state);

QString svgNumber(double value);
QString pointsToSvg(const QJsonArray &points);
QString svgEscaped(const QString &value);
QString svgDashArray(const QString &lineStyle, double strokeWidth);
QString svgDataAttribute(const QJsonObject &object, const QString &field, const QString &attribute);
QString svgMetadataAttributes(const QJsonObject &object);
QString svgCommonAttributes(const QJsonObject &object, bool fillAllowed, const QString &fallbackStroke = QStringLiteral("#f4d46f"));
QString svgPointAttributes(const QJsonObject &object, const QString &fallbackStroke = QStringLiteral("#f4d46f"));
void appendSerializedObjectToSvg(QString &svg, const QJsonObject &object);

} // namespace drawing_core
