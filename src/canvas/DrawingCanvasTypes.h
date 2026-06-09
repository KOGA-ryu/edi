#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <vector>

namespace drawing_canvas {

struct CanvasPoint {
    double x = 0.0;
    double y = 0.0;
};

struct ScreenPoint {
    double x = 0.0;
    double y = 0.0;
};

struct CanvasBounds {
    bool ok = false;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
};

struct BoardBounds {
    double x = 0.0;
    double y = 0.0;
    double size = 1.0;
};

struct CanvasObjectView {
    QVariantMap values;

    QString id() const;
    QString kind() const;
    bool visible() const;
    double number(const QString &field, double fallback = 0.0) const;
    std::vector<CanvasPoint> points() const;
};

struct HitResult {
    bool ok = false;
    QString objectId;
    QString kind = QStringLiteral("none");
    double distance = 999.0;
};

struct SnapResult {
    double x = 0.0;
    double y = 0.0;
    QString kind = QStringLiteral("none");
    QString label = QStringLiteral("none");
    QString sourceObjectId;
    QString sourceKind;
    double stepPx = 32.0;
};

double finiteNumber(double value, double fallback);
double finiteNumber(const QVariant &value, double fallback);
double clamp01(double value);
bool isRectangleLike(const QString &kind);
CanvasPoint rotatedRectCenter(const CanvasObjectView &object);
std::vector<CanvasPoint> rotatedRectCorners(const CanvasObjectView &object);
CanvasPoint unrotatePointForRect(const CanvasObjectView &object, double x, double y);

CanvasPoint pointFromVariant(const QVariant &value);
QVariantMap pointToVariant(const CanvasPoint &point);
QVariantMap boundsToVariant(const CanvasBounds &bounds);
QVariantMap boardBoundsToVariant(const BoardBounds &bounds);
QVariantMap hitResultToVariant(const HitResult &hit);
QVariantMap snapResultToVariant(const SnapResult &snap);
QVariantMap objectToMap(const QVariant &value);
std::vector<CanvasObjectView> objectsFromVariantList(const QVariantList &objects);

} // namespace drawing_canvas
