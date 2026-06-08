#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

double clamp01(double value)
{
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

double sqr(double value)
{
    return value * value;
}

double distance(double ax, double ay, double bx, double by)
{
    return std::sqrt(sqr(ax - bx) + sqr(ay - by));
}

QString nextObjectId(const QString &kind, int serial)
{
    return QStringLiteral("%1_%2").arg(kind, QString::number(serial).rightJustified(4, QLatin1Char('0')));
}

QVariantMap pointObject(const QString &id, double x, double y)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), QStringLiteral("point")},
        {QStringLiteral("x"), clamp01(x)},
        {QStringLiteral("y"), clamp01(y)},
        {QStringLiteral("visible"), true},
    };
}

QVariantMap lineObject(const QString &id, double x1, double y1, double x2, double y2)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("x1"), clamp01(x1)},
        {QStringLiteral("y1"), clamp01(y1)},
        {QStringLiteral("x2"), clamp01(x2)},
        {QStringLiteral("y2"), clamp01(y2)},
        {QStringLiteral("visible"), true},
    };
}

QVariantMap rectangleObject(const QString &id, double x1, double y1, double x2, double y2)
{
    const double left = std::min(clamp01(x1), clamp01(x2));
    const double top = std::min(clamp01(y1), clamp01(y2));
    const double right = std::max(clamp01(x1), clamp01(x2));
    const double bottom = std::max(clamp01(y1), clamp01(y2));
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), QStringLiteral("rectangle")},
        {QStringLiteral("x"), left},
        {QStringLiteral("y"), top},
        {QStringLiteral("width"), right - left},
        {QStringLiteral("height"), bottom - top},
        {QStringLiteral("rotation_deg"), 0.0},
        {QStringLiteral("visible"), true},
    };
}

QVariantMap circleObject(const QString &id, double cx, double cy, double edgeX, double edgeY)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), QStringLiteral("circle")},
        {QStringLiteral("cx"), clamp01(cx)},
        {QStringLiteral("cy"), clamp01(cy)},
        {QStringLiteral("radius"), std::min(1.0, distance(clamp01(cx), clamp01(cy), clamp01(edgeX), clamp01(edgeY)))},
        {QStringLiteral("visible"), true},
    };
}

double objectHitDistance(const QVariantMap &object, double x, double y)
{
    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("point")) {
        return distance(object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble(), x, y);
    }
    if (kind == QStringLiteral("line")) {
        const double x1 = object.value(QStringLiteral("x1")).toDouble();
        const double y1 = object.value(QStringLiteral("y1")).toDouble();
        const double x2 = object.value(QStringLiteral("x2")).toDouble();
        const double y2 = object.value(QStringLiteral("y2")).toDouble();
        const double length2 = sqr(x2 - x1) + sqr(y2 - y1);
        if (length2 <= 0.000001) {
            return distance(x1, y1, x, y);
        }
        const double t = std::clamp(((x - x1) * (x2 - x1) + (y - y1) * (y2 - y1)) / length2, 0.0, 1.0);
        return distance(x1 + t * (x2 - x1), y1 + t * (y2 - y1), x, y);
    }
    if (kind == QStringLiteral("rectangle")) {
        const double left = object.value(QStringLiteral("x")).toDouble();
        const double top = object.value(QStringLiteral("y")).toDouble();
        const double right = left + object.value(QStringLiteral("width")).toDouble();
        const double bottom = top + object.value(QStringLiteral("height")).toDouble();
        const double nearestX = std::clamp(x, left, right);
        const double nearestY = std::clamp(y, top, bottom);
        return distance(nearestX, nearestY, x, y);
    }
    if (kind == QStringLiteral("circle")) {
        const double cx = object.value(QStringLiteral("cx")).toDouble();
        const double cy = object.value(QStringLiteral("cy")).toDouble();
        const double radius = object.value(QStringLiteral("radius")).toDouble();
        return std::abs(distance(cx, cy, x, y) - radius);
    }
    return std::numeric_limits<double>::max();
}

} // namespace

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
{
    m_model.insert(QStringLiteral("engine"), QStringLiteral("cpp_drawing_core_stub"));
    m_model.insert(QStringLiteral("generated_objects"), QVariantList{});
    m_model.insert(QStringLiteral("validation"), QVariantList{});
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    return m_model;
}

QString DrawingDocumentController::selectedToolId() const
{
    return m_selectedToolId;
}

QString DrawingDocumentController::selectedObjectId() const
{
    return m_selectedObjectId;
}

void DrawingDocumentController::setSelectedToolId(const QString &toolId)
{
    if (m_selectedToolId == toolId) {
        return;
    }
    m_selectedToolId = toolId;
    m_hasPendingPoint = false;
    emit modelChanged();
}

void DrawingDocumentController::clickCanvasNormalized(double x, double y)
{
    x = clamp01(x);
    y = clamp01(y);
    QVariantList objects = m_model.value(QStringLiteral("generated_objects")).toList();

    if (m_selectedToolId == QStringLiteral("select_move")) {
        QString bestId;
        double bestDistance = 0.03;
        for (const QVariant &value : objects) {
            const QVariantMap object = value.toMap();
            const double hitDistance = objectHitDistance(object, x, y);
            if (hitDistance <= bestDistance) {
                bestDistance = hitDistance;
                bestId = object.value(QStringLiteral("id")).toString();
            }
        }
        m_selectedObjectId = bestId;
        m_hasPendingPoint = false;
        emit modelChanged();
        return;
    }

    if (m_selectedToolId == QStringLiteral("point_tool")) {
        const QString id = nextObjectId(QStringLiteral("point"), m_nextObjectSerial++);
        objects.push_back(pointObject(id, x, y));
        m_selectedObjectId = id;
        m_model.insert(QStringLiteral("generated_objects"), objects);
        emit modelChanged();
        return;
    }

    if (!m_hasPendingPoint) {
        m_pendingX = x;
        m_pendingY = y;
        m_hasPendingPoint = true;
        emit modelChanged();
        return;
    }

    QVariantMap object;
    if (m_selectedToolId == QStringLiteral("line_tool")) {
        object = lineObject(nextObjectId(QStringLiteral("line"), m_nextObjectSerial++), m_pendingX, m_pendingY, x, y);
    } else if (m_selectedToolId == QStringLiteral("rectangle_tool")) {
        object = rectangleObject(nextObjectId(QStringLiteral("rectangle"), m_nextObjectSerial++), m_pendingX, m_pendingY, x, y);
    } else if (m_selectedToolId == QStringLiteral("circle_tool")) {
        object = circleObject(nextObjectId(QStringLiteral("circle"), m_nextObjectSerial++), m_pendingX, m_pendingY, x, y);
    }
    m_hasPendingPoint = false;
    if (!object.isEmpty()) {
        m_selectedObjectId = object.value(QStringLiteral("id")).toString();
        objects.push_back(object);
        m_model.insert(QStringLiteral("generated_objects"), objects);
    }
    emit modelChanged();
}
