#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "drafting/DraftingCommands.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingSelection.h"

#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

namespace {

using namespace edi::drafting;

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

std::string toStdString(const QString &value)
{
    return value.toStdString();
}

QString toQString(const std::string &value)
{
    return QString::fromStdString(value);
}

QVariantMap pointToMap(Point2D point)
{
    return {
        {QStringLiteral("x"), point.x},
        {QStringLiteral("y"), point.y},
    };
}

QVariantMap objectToProjection(const DraftingObject &object)
{
    QVariantMap result {
        {QStringLiteral("id"), toQString(object.id)},
        {QStringLiteral("kind"), QString::fromLatin1(shapeKindName(object.kind))},
        {QStringLiteral("visible"), true},
    };

    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            result.insert(QStringLiteral("x"), geometry.point.x);
            result.insert(QStringLiteral("y"), geometry.point.y);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            result.insert(QStringLiteral("x1"), geometry.a.x);
            result.insert(QStringLiteral("y1"), geometry.a.y);
            result.insert(QStringLiteral("x2"), geometry.b.x);
            result.insert(QStringLiteral("y2"), geometry.b.y);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            result.insert(QStringLiteral("x"), geometry.origin.x);
            result.insert(QStringLiteral("y"), geometry.origin.y);
            result.insert(QStringLiteral("width"), geometry.width);
            result.insert(QStringLiteral("height"), geometry.height);
            result.insert(QStringLiteral("rotation_deg"), geometry.rotationDeg);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            result.insert(QStringLiteral("cx"), geometry.center.x);
            result.insert(QStringLiteral("cy"), geometry.center.y);
            result.insert(QStringLiteral("radius"), geometry.radius);
        } else {
            QVariantList points;
            for (Point2D point : geometry.vertices) {
                points.push_back(pointToMap(point));
            }
            result.insert(QStringLiteral("points"), points);
        }
    }, object.geometry);

    return result;
}

std::optional<DraftingObject> buildObjectForTool(const QString &toolId, const QString &objectId, Point2D start, Point2D end)
{
    DraftingShapeKind kind = DraftingShapeKind::Point;
    DraftingGeometry geometry = PointGeometry{start};
    if (toolId == QStringLiteral("point_tool")) {
        kind = DraftingShapeKind::Point;
        geometry = PointGeometry{end};
    } else if (toolId == QStringLiteral("line_tool")) {
        kind = DraftingShapeKind::Line;
        geometry = LineGeometry{start, end};
    } else if (toolId == QStringLiteral("rectangle_tool")) {
        const double left = std::min(start.x, end.x);
        const double top = std::min(start.y, end.y);
        const double right = std::max(start.x, end.x);
        const double bottom = std::max(start.y, end.y);
        kind = DraftingShapeKind::Rectangle;
        geometry = RectangleGeometry{{left, top}, right - left, bottom - top};
    } else if (toolId == QStringLiteral("circle_tool")) {
        kind = DraftingShapeKind::Circle;
        geometry = CircleGeometry{start, std::min(1.0, distance(start.x, start.y, end.x, end.y))};
    } else {
        return std::nullopt;
    }

    auto built = buildDraftingObject(toStdString(objectId), kind, std::move(geometry));
    if (!built.ok) {
        return std::nullopt;
    }
    built.object.metadata.toolProvenance = toolId.toStdString();
    return built.object;
}

double geometryHitDistance(const DraftingGeometry &geometry, double x, double y)
{
    return std::visit([&](const auto &typedGeometry) -> double {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            return distance(typedGeometry.point.x, typedGeometry.point.y, x, y);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            const double length2 = sqr(typedGeometry.b.x - typedGeometry.a.x) + sqr(typedGeometry.b.y - typedGeometry.a.y);
            if (length2 <= 0.000001) {
                return distance(typedGeometry.a.x, typedGeometry.a.y, x, y);
            }
            const double t = std::clamp(((x - typedGeometry.a.x) * (typedGeometry.b.x - typedGeometry.a.x)
                                            + (y - typedGeometry.a.y) * (typedGeometry.b.y - typedGeometry.a.y)) / length2,
                0.0,
                1.0);
            return distance(
                typedGeometry.a.x + t * (typedGeometry.b.x - typedGeometry.a.x),
                typedGeometry.a.y + t * (typedGeometry.b.y - typedGeometry.a.y),
                x,
                y);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const Bounds2D bounds = computeBounds(typedGeometry);
            const double nearestX = std::clamp(x, bounds.x, bounds.x + bounds.width);
            const double nearestY = std::clamp(y, bounds.y, bounds.y + bounds.height);
            return distance(nearestX, nearestY, x, y);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            return std::abs(distance(typedGeometry.center.x, typedGeometry.center.y, x, y) - typedGeometry.radius);
        } else {
            double best = std::numeric_limits<double>::max();
            for (Point2D point : typedGeometry.vertices) {
                best = std::min(best, distance(point.x, point.y, x, y));
            }
            return best;
        }
    }, geometry);
}

std::optional<DraftingObjectId> hitObject(const DraftingDocument &document, double x, double y)
{
    std::optional<DraftingObjectId> bestId;
    double bestDistance = 0.03;
    for (const DraftingObject &object : document.objects) {
        if (!object.visible) {
            continue;
        }
        const double hitDistance = geometryHitDistance(object.geometry, x, y);
        if (hitDistance <= bestDistance) {
            bestDistance = hitDistance;
            bestId = object.id;
        }
    }
    return bestId;
}

} // namespace

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
    , m_document(makeDraftingDocument("active_drawing", "Active Drawing"))
{
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    QVariantList objects;
    for (const DraftingObject &object : m_document.objects) {
        objects.push_back(objectToProjection(object));
    }
    return {
        {QStringLiteral("engine"), QStringLiteral("cpp_drafting_document")},
        {QStringLiteral("drawing_objects"), objects},
        {QStringLiteral("revision"), static_cast<int>(m_document.revision)},
        {QStringLiteral("validation"), QVariantList{}},
    };
}

QString DrawingDocumentController::selectedToolId() const
{
    return m_selectedToolId;
}

QString DrawingDocumentController::selectedObjectId() const
{
    return m_document.activeObjectId ? toQString(*m_document.activeObjectId) : QString();
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
    const Point2D point{x, y};

    if (m_selectedToolId == QStringLiteral("select_move")) {
        const std::optional<DraftingObjectId> objectId = hitObject(m_document, x, y);
        if (objectId) {
            applyDraftingCommand(m_document, SelectObjectCommand{*objectId});
        } else {
            clearSelection(m_document);
            ++m_document.revision;
        }
        m_hasPendingPoint = false;
        emit modelChanged();
        return;
    }

    if (m_selectedToolId == QStringLiteral("point_tool")) {
        const QString id = nextObjectId(QStringLiteral("point"), m_nextObjectSerial++);
        const auto object = buildObjectForTool(m_selectedToolId, id, point, point);
        if (object) {
            applyDraftingCommand(m_document, CreateObjectCommand{*object});
            applyDraftingCommand(m_document, SelectObjectCommand{object->id});
        }
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

    const QString id = nextObjectId(m_selectedToolId.section(QLatin1Char('_'), 0, 0), m_nextObjectSerial++);
    const auto object = buildObjectForTool(m_selectedToolId, id, {m_pendingX, m_pendingY}, point);
    m_hasPendingPoint = false;
    if (object) {
        applyDraftingCommand(m_document, CreateObjectCommand{*object});
        applyDraftingCommand(m_document, SelectObjectCommand{object->id});
    }
    emit modelChanged();
}
