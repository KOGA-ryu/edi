#include "widgets/DrawingCanvasObjectPainter.h"

#include "widgets/DrawingCanvasProjectedObject.h"
#include "widgets/DrawingCanvasViewport.h"

#include <QPainter>
#include <QPolygonF>

#include <cmath>

namespace drawing_canvas {
namespace {

QLineF screenLine(const DrawingCanvasObjectPainterContext &context, const DrawingCanvasProjectedLine &line)
{
    return QLineF(
        drawing_canvas::canvasToScreen(context.board, line.x1, line.y1),
        drawing_canvas::canvasToScreen(context.board, line.x2, line.y2));
}

void drawSelectedHandles(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context)
{
    const std::vector<DrawingCanvasProjectedHandle> projectedHandles = projectedObjectHandles(object);
    if (projectedHandles.empty()) {
        return;
    }

    painter.setPen(QPen(QColor("#1d1f26"), 2));
    for (const DrawingCanvasProjectedHandle &handle : projectedHandles) {
        const QPointF point = drawing_canvas::canvasToScreen(context.board, handle.x, handle.y);
        if (handle.hasAnchor) {
            const QPointF anchor = drawing_canvas::canvasToScreen(context.board, handle.anchorX, handle.anchorY);
            painter.drawLine(anchor, point);
        }
        const QRectF rect(point.x() - handle.sizePx * 0.5, point.y() - handle.sizePx * 0.5, handle.sizePx, handle.sizePx);
        painter.setBrush(handle.editable ? QColor("#f6c65b") : QColor("#79828f"));
        if (handle.shape == DrawingCanvasProjectedHandleShape::Square) {
            painter.drawRect(rect);
        } else if (handle.shape == DrawingCanvasProjectedHandleShape::Diamond) {
            QPolygonF diamond;
            diamond << QPointF(point.x(), point.y() - handle.sizePx * 0.55)
                    << QPointF(point.x() + handle.sizePx * 0.55, point.y())
                    << QPointF(point.x(), point.y() + handle.sizePx * 0.55)
                    << QPointF(point.x() - handle.sizePx * 0.55, point.y());
            painter.drawPolygon(diamond);
        } else {
            painter.drawEllipse(rect);
        }
    }
}

} // namespace

QColor withAlpha(const QColor &color, int alpha)
{
    return QColor(color.red(), color.green(), color.blue(), alpha);
}

void drawCrosshair(QPainter &painter, const QPointF &point, double extent)
{
    painter.drawLine(QPointF(point.x() - extent, point.y()), QPointF(point.x() + extent, point.y()));
    painter.drawLine(QPointF(point.x(), point.y() - extent), QPointF(point.x(), point.y() + extent));
}

void drawGuideIntersections(QPainter &painter, const QVariantList &objects, const DrawingCanvasObjectPainterContext &context)
{
    std::vector<double> vertical;
    std::vector<double> horizontal;
    for (const QVariant &value : objects) {
        const QVariantMap object = value.toMap();
        const DrawingCanvasProjectedObjectSummary summary = projectedObjectSummary(object);
        if (summary.kind != QStringLiteral("guide") || !summary.visible) {
            continue;
        }
        const DrawingCanvasProjectedGuide guide = projectedGuide(object);
        if (!guide.ok) {
            continue;
        }
        if (guide.orientation == DrawingCanvasProjectedGuideOrientation::Horizontal) {
            horizontal.push_back(guide.position);
        } else {
            vertical.push_back(guide.position);
        }
    }
    if (vertical.empty() || horizontal.empty()) {
        return;
    }

    painter.save();
    QColor marker("#b7d7e8");
    marker.setAlpha(120);
    painter.setPen(QPen(marker, 1.0));
    painter.setBrush(withAlpha(marker, 30));
    for (double x : vertical) {
        for (double y : horizontal) {
            const QPointF point = drawing_canvas::canvasToScreen(context.board, x, y);
            painter.drawEllipse(point, 3.0, 3.0);
            drawCrosshair(painter, point, 5.0);
        }
    }
    painter.restore();
}

void drawObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context)
{
    const DrawingCanvasProjectedObjectSummary summary = projectedObjectSummary(object);
    if (!summary.visible) {
        return;
    }

    const QString &kind = summary.kind;
    const bool selected = !context.selectedObjectId.isEmpty() && summary.id == context.selectedObjectId;

    if (kind == QStringLiteral("guide")) {
        const DrawingCanvasProjectedGuide guide = projectedGuide(object);
        if (!guide.ok) {
            return;
        }
        QColor guideColor(guide.color);
        if (!guideColor.isValid()) {
            guideColor = QColor("#83aeca");
        }
        if (selected) {
            guideColor = guideColor.lighter(120);
        } else if (guide.locked) {
            guideColor = QColor("#6f8295");
        }
        guideColor.setAlpha(selected ? 230 : 165);
        Qt::PenStyle guideStyle = Qt::DashLine;
        if (guide.dashStyle == QStringLiteral("solid")) {
            guideStyle = Qt::SolidLine;
        } else if (guide.dashStyle == QStringLiteral("dot")) {
            guideStyle = Qt::DotLine;
        }
        QPen guidePen(guideColor, selected ? 2.0 : 1.25, guideStyle);
        guidePen.setCapStyle(Qt::RoundCap);
        painter.save();
        painter.setPen(guidePen);
        painter.setBrush(Qt::NoBrush);
        if (guide.orientation == DrawingCanvasProjectedGuideOrientation::Horizontal) {
            painter.drawLine(drawing_canvas::canvasToScreen(context.board, 0.0, guide.position), drawing_canvas::canvasToScreen(context.board, 1.0, guide.position));
            if (guide.showLabel && !guide.label.isEmpty()) {
                painter.setPen(guideColor);
                painter.drawText(drawing_canvas::canvasToScreen(context.board, 0.0, guide.position) + QPointF(8.0, -6.0), guide.label);
            }
        } else {
            painter.drawLine(drawing_canvas::canvasToScreen(context.board, guide.position, 0.0), drawing_canvas::canvasToScreen(context.board, guide.position, 1.0));
            if (guide.showLabel && !guide.label.isEmpty()) {
                painter.setPen(guideColor);
                painter.drawText(drawing_canvas::canvasToScreen(context.board, guide.position, 0.0) + QPointF(8.0, 16.0), guide.label);
            }
        }
        painter.restore();
        return;
    }

    if (kind == QStringLiteral("construction_line")) {
        const DrawingCanvasProjectedLine line = projectedLine(object);
        if (!line.ok) {
            return;
        }
        QPen constructionPen(selected ? QColor("#f6c65b") : QColor("#9fb2c7"), selected ? 2 : 1, Qt::DotLine);
        constructionPen.setCapStyle(Qt::RoundCap);
        painter.setPen(constructionPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(screenLine(context, line));
        return;
    }

    if (kind == QStringLiteral("dimension")) {
        const DrawingCanvasProjectedDimension dimension = projectedDimension(object);
        if (!dimension.ok) {
            return;
        }
        const QPointF a = drawing_canvas::canvasToScreen(context.board, dimension.x1, dimension.y1);
        const QPointF b = drawing_canvas::canvasToScreen(context.board, dimension.x2, dimension.y2);
        const QPointF dimA = drawing_canvas::canvasToScreen(context.board, dimension.dimensionX1, dimension.dimensionY1);
        const QPointF dimB = drawing_canvas::canvasToScreen(context.board, dimension.dimensionX2, dimension.dimensionY2);
        const QPointF label = drawing_canvas::canvasToScreen(context.board, dimension.labelX, dimension.labelY);
        const QColor dimensionColor = selected ? QColor("#f6c65b") : QColor("#b6d28f");
        QPen dimensionPen(dimensionColor, selected ? 2.0 : 1.5);
        dimensionPen.setCapStyle(Qt::RoundCap);
        dimensionPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(dimensionPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(a, dimA);
        painter.drawLine(b, dimB);
        painter.drawLine(dimA, dimB);
        painter.drawLine(dimA + QPointF(-6.0, -6.0), dimA + QPointF(6.0, 6.0));
        painter.drawLine(dimA + QPointF(-6.0, 6.0), dimA + QPointF(6.0, -6.0));
        painter.drawLine(dimB + QPointF(-6.0, -6.0), dimB + QPointF(6.0, 6.0));
        painter.drawLine(dimB + QPointF(-6.0, 6.0), dimB + QPointF(6.0, -6.0));
        if (dimension.showLabel) {
            const QString text = dimension.label;
            const QRect textBounds = painter.fontMetrics().boundingRect(text);
            QRectF labelRect(
                label.x() - textBounds.width() / 2.0 - 6.0,
                label.y() - textBounds.height() - 8.0,
                textBounds.width() + 12.0,
                textBounds.height() + 8.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(23, 25, 31, selected ? 230 : 190));
            painter.drawRoundedRect(labelRect, 4.0, 4.0);
            painter.setPen(dimensionColor);
            painter.drawText(labelRect, Qt::AlignCenter, text);
        }
        return;
    }

    const DrawingCanvasProjectedStyle style = projectedObjectStyle(object);
    QPen pen(
        selected ? QColor("#f6c65b") : QColor(style.strokeColor),
        selected ? 3.0 : style.strokeWidth);
    if (summary.plotBlocked) {
        pen.setColor(QColor("#d98b8b"));
    }
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    if (kind == QStringLiteral("point")) {
        const DrawingCanvasProjectedPointObject pointObject = projectedPointObject(object);
        if (!pointObject.ok) {
            return;
        }
        const QPointF point = drawing_canvas::canvasToScreen(context.board, pointObject.x, pointObject.y);
        painter.setBrush(selected ? QColor("#f6c65b") : QColor("#d7dde8"));
        painter.drawEllipse(point, 4.0, 4.0);
    } else if (kind == QStringLiteral("line")) {
        const DrawingCanvasProjectedLine line = projectedLine(object);
        if (!line.ok) {
            return;
        }
        painter.drawLine(screenLine(context, line));
    } else if (kind == QStringLiteral("rectangle")) {
        const DrawingCanvasProjectedRectangle rectangle = projectedRectangle(object);
        if (!rectangle.ok) {
            return;
        }
        const QPointF origin = drawing_canvas::canvasToScreen(context.board, rectangle.x, rectangle.y);
        const QPointF extent = drawing_canvas::canvasToScreen(context.board, rectangle.x + rectangle.width, rectangle.y + rectangle.height);
        QRectF rect(origin, extent);
        if (std::abs(rectangle.rotationDeg) > 0.000001) {
            painter.save();
            painter.translate(rect.center());
            painter.rotate(rectangle.rotationDeg);
            painter.translate(-rect.center());
            painter.drawRect(rect);
            painter.restore();
        } else {
            painter.drawRect(rect.normalized());
        }
    } else if (kind == QStringLiteral("circle")) {
        const DrawingCanvasProjectedCircle circle = projectedCircle(object);
        if (!circle.ok) {
            return;
        }
        const QPointF center = drawing_canvas::canvasToScreen(context.board, circle.cx, circle.cy);
        const double radius = circle.radius * context.board.width();
        painter.drawEllipse(center, radius, radius);
    } else if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon") || kind == QStringLiteral("arc")) {
        const DrawingCanvasProjectedPolygon projected = projectedPolygon(object);
        if (!projected.ok) {
            return;
        }
        QPolygonF polygon;
        for (const DrawingCanvasProjectedPoint &point : projected.points) {
            polygon.push_back(drawing_canvas::canvasToScreen(context.board, point.x, point.y));
        }
        if (kind == QStringLiteral("polygon")) {
            painter.drawPolygon(polygon);
        } else {
            // Arc and polyline are open chains.
            painter.drawPolyline(polygon);
        }
    }

    if (summary.plotBlocked && summary.bounds.ok) {
        QRectF warningRect = drawing_canvas::boundsToScreenRect(
            context.board, summary.bounds.x, summary.bounds.y, summary.bounds.width, summary.bounds.height);
        if (warningRect.width() < 12.0 || warningRect.height() < 12.0) {
            warningRect = warningRect.adjusted(-6.0, -6.0, 6.0, 6.0);
        }
        painter.setPen(QPen(QColor("#d98b8b"), 1.5, Qt::DashLine));
        painter.setBrush(QColor(217, 139, 139, 24));
        painter.drawRect(warningRect);
        if (!summary.plotWarningKind.isEmpty()) {
            painter.setPen(QColor("#d98b8b"));
            painter.drawText(warningRect.topLeft() + QPointF(6.0, -6.0), summary.plotWarningKind);
        }
    }

    if (selected) {
        drawSelectedHandles(painter, object, context);
    }
}

void drawPreviewObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context)
{
    painter.save();
    QPen pen(QColor("#75c7ff"), 2, Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QString kind = projectedObjectSummary(object).kind;
    if (kind == QStringLiteral("line")) {
        const DrawingCanvasProjectedLine line = projectedLine(object);
        if (line.ok) {
            painter.drawLine(screenLine(context, line));
        }
    } else if (kind == QStringLiteral("construction_line")) {
        const DrawingCanvasProjectedLine line = projectedLine(object);
        if (!line.ok) {
            painter.restore();
            return;
        }
        QPen constructionPen(QColor("#75c7ff"), 1.5, Qt::DotLine);
        constructionPen.setCapStyle(Qt::RoundCap);
        painter.setPen(constructionPen);
        painter.drawLine(screenLine(context, line));
    } else if (kind == QStringLiteral("dimension")) {
        const DrawingCanvasProjectedDimension dimension = projectedDimension(object);
        if (!dimension.ok) {
            painter.restore();
            return;
        }
        const QPointF a = drawing_canvas::canvasToScreen(context.board, dimension.x1, dimension.y1);
        const QPointF b = drawing_canvas::canvasToScreen(context.board, dimension.x2, dimension.y2);
        const QPointF dimA = drawing_canvas::canvasToScreen(context.board, dimension.dimensionX1, dimension.dimensionY1);
        const QPointF dimB = drawing_canvas::canvasToScreen(context.board, dimension.dimensionX2, dimension.dimensionY2);
        painter.drawLine(a, dimA);
        painter.drawLine(b, dimB);
        painter.drawLine(dimA, dimB);
        painter.drawText(drawing_canvas::canvasToScreen(context.board, dimension.labelX, dimension.labelY) + QPointF(6.0, -6.0), dimension.label);
    } else if (kind == QStringLiteral("rectangle")) {
        const DrawingCanvasProjectedRectangle rectangle = projectedRectangle(object);
        if (rectangle.ok) {
            painter.drawRect(drawing_canvas::boundsToScreenRect(
                context.board, rectangle.x, rectangle.y, rectangle.width, rectangle.height));
        }
    } else if (kind == QStringLiteral("circle")) {
        const DrawingCanvasProjectedCircle circle = projectedCircle(object);
        if (circle.ok) {
            const QPointF center = drawing_canvas::canvasToScreen(context.board, circle.cx, circle.cy);
            const double radius = circle.radius * context.board.width();
            painter.drawEllipse(center, radius, radius);
        }
    } else if (kind == QStringLiteral("arc")) {
        const DrawingCanvasProjectedPolygon projected = projectedPolygon(object);
        if (projected.ok) {
            QPolygonF chain;
            for (const DrawingCanvasProjectedPoint &point : projected.points) {
                chain.push_back(drawing_canvas::canvasToScreen(context.board, point.x, point.y));
            }
            painter.drawPolyline(chain);
        }
    }
    painter.restore();
}

} // namespace drawing_canvas
