#include "widgets/DrawingCanvasObjectPainter.h"

#include "widgets/DrawingCanvasChromeDims.h"
#include "widgets/DrawingCanvasProjectedObject.h"
#include "widgets/DrawingCanvasValues.h"
#include "widgets/DrawingCanvasViewport.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPolygonF>

#include <cmath>
#include <map>
#include <utility>
#include <vector>

namespace drawing_canvas {
namespace {

QLineF screenLine(const DrawingCanvasObjectPainterContext &context, const DrawingCanvasProjectedLine &line)
{
    return QLineF(
        drawing_canvas::canvasToScreen(context.board, line.x1, line.y1),
        drawing_canvas::canvasToScreen(context.board, line.x2, line.y2));
}

// N4: draw a rectangle honoring its variant params. cornerRadius/inset are
// in canvas units, converted to screen pixels via the board scale so they
// track zoom. Rotation rotates the painter about the rect center, so rounded
// + rotated composes for free. inset > 0 adds an inner outline (the frame's
// hole), its corner radius shrunk by the wall so concentric corners stay
// parallel.
void drawRectangleShape(QPainter &painter, const DrawingCanvasObjectPainterContext &context,
                        const DrawingCanvasProjectedRectangle &rectangle)
{
    const QPointF origin = drawing_canvas::canvasToScreen(context.board, rectangle.x, rectangle.y);
    const QPointF extent = drawing_canvas::canvasToScreen(
        context.board, rectangle.x + rectangle.width, rectangle.y + rectangle.height);
    const QRectF rect = QRectF(origin, extent).normalized();
    // One canvas unit -> pixels: the x-span of a unit step on the board.
    const QPointF unitX = drawing_canvas::canvasToScreen(context.board, rectangle.x + 1.0, rectangle.y);
    const double scale = std::abs(unitX.x() - origin.x());
    const double radius = std::max(0.0, rectangle.cornerRadius) * scale;
    const double inset = std::max(0.0, rectangle.inset) * scale;

    const bool rotated = std::abs(rectangle.rotationDeg) > 0.000001;
    if (rotated) {
        painter.save();
        painter.translate(rect.center());
        painter.rotate(rectangle.rotationDeg);
        painter.translate(-rect.center());
    }
    if (radius > 0.0) {
        painter.drawRoundedRect(rect, radius, radius);
    } else {
        painter.drawRect(rect);
    }
    // The frame's inner edge: only when it fits inside the wall on both axes.
    if (inset > 0.0 && rect.width() > 2.0 * inset && rect.height() > 2.0 * inset) {
        const QRectF inner = rect.adjusted(inset, inset, -inset, -inset);
        const double innerRadius = std::max(0.0, radius - inset);
        if (innerRadius > 0.0) {
            painter.drawRoundedRect(inner, innerRadius, innerRadius);
        } else {
            painter.drawRect(inner);
        }
    }
    if (rotated) {
        painter.restore();
    }
}

// N2: two short strokes back from the tip (p2) form the arrowhead. Drawn in
// SCREEN space with a fixed pixel size, so the head stays legible at any
// zoom — a head scaled with the geometry would vanish on a short line.
void drawArrowHead(QPainter &painter, const QLineF &screen)
{
    constexpr double headLength = kArrowHeadLengthPx;
    constexpr double headSpread = 0.45; // radians from the shaft, ~26 degrees
    const QPointF tip = screen.p2();
    const double shaftAngle = std::atan2(screen.p1().y() - tip.y(), screen.p1().x() - tip.x());
    painter.drawLine(tip, tip + QPointF(headLength * std::cos(shaftAngle + headSpread),
                                        headLength * std::sin(shaftAngle + headSpread)));
    painter.drawLine(tip, tip + QPointF(headLength * std::cos(shaftAngle - headSpread),
                                        headLength * std::sin(shaftAngle - headSpread)));
}

void drawSelectedHandles(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context)
{
    const std::vector<DrawingCanvasProjectedHandle> projectedHandles = projectedObjectHandles(object);
    if (projectedHandles.empty()) {
        return;
    }

    painter.setPen(QPen(context.palette.handleOutline, kHandleOutlinePenPx));
    for (const DrawingCanvasProjectedHandle &handle : projectedHandles) {
        const QPointF point = drawing_canvas::canvasToScreen(context.board, handle.x, handle.y);
        if (handle.hasAnchor) {
            const QPointF anchor = drawing_canvas::canvasToScreen(context.board, handle.anchorX, handle.anchorY);
            painter.drawLine(anchor, point);
        }
        const QRectF rect(point.x() - handle.sizePx * 0.5, point.y() - handle.sizePx * 0.5, handle.sizePx, handle.sizePx);
        painter.setBrush(handle.editable ? context.palette.selection : context.palette.handleFrozen);
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

void drawGuideIntersections(QPainter &painter, const std::vector<DrawingCanvasSceneItem> &scene, const DrawingCanvasObjectPainterContext &context)
{
    std::vector<double> vertical;
    std::vector<double> horizontal;
    for (const DrawingCanvasSceneItem &item : scene) {
        if (item.summary.kind != QStringLiteral("guide") || !item.summary.visible || !item.guide.ok) {
            continue;
        }
        if (item.guide.orientation == DrawingCanvasProjectedGuideOrientation::Horizontal) {
            horizontal.push_back(item.guide.position);
        } else {
            vertical.push_back(item.guide.position);
        }
    }
    if (vertical.empty() || horizontal.empty()) {
        return;
    }

    painter.save();
    QColor marker = context.palette.guideIntersection;
    marker.setAlpha(120);
    painter.setPen(QPen(marker, kSnapMarkerPenPx));
    painter.setBrush(withAlpha(marker, 30));
    for (double x : vertical) {
        for (double y : horizontal) {
            const QPointF point = drawing_canvas::canvasToScreen(context.board, x, y);
            painter.drawEllipse(point, kSnapDotRadiusPx, kSnapDotRadiusPx);
            drawCrosshair(painter, point, kSnapCrosshairHalfPx);
        }
    }
    painter.restore();
}

std::vector<QPointF> wallCornerMiterFill(const QPointF &p, const QPointF &tThis, const QPointF &tNbr,
                                         double hThis, double hNbr, double miterLimit)
{
    // Perpendicular (rotate +90deg) of each into-wall direction. tThis/tNbr are
    // assumed unit; the band's two long edges are offset by +/- half-thickness
    // along these normals.
    const QPointF nThis(-tThis.y(), tThis.x());
    const QPointF nNbr(-tNbr.y(), tNbr.x());
    const auto dot = [](const QPointF &a, const QPointF &b) { return a.x() * b.x() + a.y() * b.y(); };
    const auto cross = [](const QPointF &a, const QPointF &b) { return a.x() * b.y() - a.y() * b.x(); };

    // The OUTER corner of each band is the cap corner on the side AWAY from the
    // other wall (the convex side of the turn, where the square caps leave a
    // notch). nThis points "outward" from this wall when it faces away from the
    // neighbour's direction, i.e. dot(nThis, tNbr) < 0.
    const QPointF outerThis = (dot(nThis, tNbr) < 0.0) ? p + nThis * hThis : p - nThis * hThis;
    const QPointF outerNbr = (dot(nNbr, tThis) < 0.0) ? p + nNbr * hNbr : p - nNbr * hNbr;

    // The miter apex is where the two outer edges (rays from the outer corners
    // along each into-wall direction) cross. Near-parallel edges mean the walls
    // are ~collinear (no real corner) — nothing to fill.
    const double denom = cross(tThis, tNbr);
    if (std::abs(denom) < 1e-6) {
        return {};
    }
    const QPointF delta = outerNbr - outerThis;
    const double along = cross(delta, tNbr) / denom;
    const QPointF apex = outerThis + tThis * along;

    // The 4-pt miter quad [outerThis, apex, outerNbr, p] is the correct fill only
    // while it stays CONVEX. Two failure modes collapse it to a bevel instead:
    //   - a very acute corner pushes the apex far from p (a long spike) past the
    //     miter limit;
    //   - UNEQUAL band half-thicknesses at an obtuse corner fold the quad into a
    //     self-intersecting bow-tie, which QPainter's default OddEven fill would
    //     leave HOLED — the opposite of closing the notch.
    // The bevel chord [outerThis, outerNbr, p] always covers the notch (a clean
    // chamfer) without overhanging, so it is the safe fallback for both.
    const double miterLen = std::hypot(apex.x() - p.x(), apex.y() - p.y());
    const QPointF e0 = apex - outerThis;
    const QPointF e1 = outerNbr - apex;
    const QPointF e2 = p - outerNbr;
    const QPointF e3 = outerThis - p;
    bool turnedLeft = false;
    bool turnedRight = false;
    for (const double turn : {cross(e0, e1), cross(e1, e2), cross(e2, e3), cross(e3, e0)}) {
        if (turn > 1e-9) {
            turnedLeft = true;
        } else if (turn < -1e-9) {
            turnedRight = true;
        }
    }
    const bool convex = !(turnedLeft && turnedRight);
    if (!convex || miterLen > miterLimit * std::max(hThis, hNbr)) {
        return {outerThis, outerNbr, p};
    }
    return {outerThis, apex, outerNbr, p};
}

// Quantise an endpoint to a grid coarse enough to absorb float noise but fine
// enough never to merge genuinely distinct corners (canvas units are ~[0,1]).
static long long wallJoinKeyComponent(double v)
{
    return static_cast<long long>(std::llround(v * 1.0e5));
}

void annotateWallJoins(std::vector<DrawingCanvasSceneItem> &scene)
{
    // One record per wall endpoint: where it is, which wall/end it belongs to,
    // and the wall's OTHER endpoint + thickness (so a neighbour can reconstruct
    // the join without re-reading the scene).
    struct Endpoint {
        long long kx;
        long long ky;
        std::size_t index; // scene index of the owning wall
        bool isA;          // true = endpoint a, false = endpoint b
        double farX;
        double farY;
        double thickness;
    };
    std::vector<Endpoint> endpoints;
    std::map<std::pair<long long, long long>, std::vector<std::size_t>> byPosition;
    for (std::size_t i = 0; i < scene.size(); ++i) {
        const DrawingCanvasProjectedWall &w = scene[i].wall;
        if (!w.ok) {
            continue;
        }
        const long long ax = wallJoinKeyComponent(w.ax), ay = wallJoinKeyComponent(w.ay);
        const long long bx = wallJoinKeyComponent(w.bx), by = wallJoinKeyComponent(w.by);
        byPosition[{ax, ay}].push_back(endpoints.size());
        endpoints.push_back({ax, ay, i, true, w.bx, w.by, w.thickness});
        byPosition[{bx, by}].push_back(endpoints.size());
        endpoints.push_back({bx, by, i, false, w.ax, w.ay, w.thickness});
    }

    // A clean corner is a position shared by EXACTLY TWO endpoints of DIFFERENT
    // walls. v1 only miters that simple case; T-junctions / crossings (3+ at a
    // point) and a wall's own coincident ends keep square caps — a pairwise pass
    // would draw conflicting partial miters there. Grouping by position first
    // makes the count explicit (and keeps this O(n log n), not O(n^2)). The lower
    // scene index paints the corner so the fill is drawn exactly once.
    for (const auto &[position, group] : byPosition) {
        if (group.size() != 2) {
            continue;
        }
        const Endpoint &e0 = endpoints[group[0]];
        const Endpoint &e1 = endpoints[group[1]];
        if (e0.index == e1.index) {
            continue; // one wall's own two ends (a zero-length wall) — not a join
        }
        const Endpoint &lo = e0.index < e1.index ? e0 : e1;
        const Endpoint &hi = e0.index < e1.index ? e1 : e0;
        DrawingCanvasWallJoin &join = lo.isA ? scene[lo.index].wall.joinA : scene[lo.index].wall.joinB;
        join.drawCorner = true;
        join.neighborFarX = hi.farX;
        join.neighborFarY = hi.farY;
        join.neighborThickness = hi.thickness;
    }
}

DrawingCanvasSceneItem buildCanvasSceneItem(const QVariantMap &object)
{
    // The extraction half of the old drawObject, run once per document
    // mutation instead of once per object per frame. Reuses the same
    // projected* readers, so the typed scene cannot drift from the map path.
    DrawingCanvasSceneItem item;
    item.summary = projectedObjectSummary(object);
    if (!item.summary.visible) {
        return item;
    }
    const QString &kind = item.summary.kind;
    if (kind == QStringLiteral("guide")) {
        item.guide = projectedGuide(object);
    } else if (kind == QStringLiteral("construction_line") || kind == QStringLiteral("line")) {
        item.line = projectedLine(object);
        item.endArrow = object.value(QStringLiteral("end_arrow")).toBool();
        item.startArrow = object.value(QStringLiteral("start_arrow")).toBool();
    } else if (kind == QStringLiteral("dimension")) {
        item.dimension = projectedDimension(object);
    } else if (kind == QStringLiteral("point")) {
        item.point = projectedPointObject(object);
    } else if (kind == QStringLiteral("rectangle")) {
        item.rectangle = projectedRectangle(object);
    } else if (kind == QStringLiteral("circle")) {
        item.circle = projectedCircle(object);
    } else if (kind == QStringLiteral("wall")) {
        // Two-click thick line: centerline a->b (canvas units, keys ax/ay/bx/by)
        // plus a scalar band thickness (key thickness), mirroring projectedLine's
        // a/b and projectedCircle's radius. A wall is NOT a points-list shape, so
        // it must not join the polygon arm below.
        item.wall = projectedWall(object);
    } else if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon") || kind == QStringLiteral("arc") || kind == QStringLiteral("ellipse") || kind == QStringLiteral("spline")) {
        item.polygon = projectedPolygon(object);
    } else if (kind == QStringLiteral("text")) {
        // The projection emits px/py (top-left, canvas units), height (cap
        // height, canvas units, default 0.04) and content (the QString).
        // Extracted inline like the other small per-kind readers.
        item.text.ok = true;
        item.text.x = finiteNumber(object.value(QStringLiteral("px")), 0.0);
        item.text.y = finiteNumber(object.value(QStringLiteral("py")), 0.0);
        item.text.height = finiteNumber(object.value(QStringLiteral("height")), 0.04);
        item.text.content = object.value(QStringLiteral("content")).toString();
    }
    item.style = projectedObjectStyle(object);
    item.source = object;
    return item;
}

void drawSceneItem(QPainter &painter, const DrawingCanvasSceneItem &item, const DrawingCanvasObjectPainterContext &context)
{
    const DrawingCanvasProjectedObjectSummary &summary = item.summary;
    if (!summary.visible) {
        return;
    }

    const QString &kind = summary.kind;
    const bool selected = !context.selectedObjectId.isEmpty() && summary.id == context.selectedObjectId;

    if (kind == QStringLiteral("guide")) {
        const DrawingCanvasProjectedGuide &guide = item.guide;
        if (!guide.ok) {
            return;
        }
        QColor guideColor(guide.color);
        if (!guideColor.isValid()) {
            guideColor = context.palette.guideFallback;
        }
        if (selected) {
            guideColor = guideColor.lighter(120);
        } else if (guide.locked) {
            guideColor = context.palette.guideLocked;
        }
        guideColor.setAlpha(selected ? 230 : 165);
        Qt::PenStyle guideStyle = Qt::DashLine;
        if (guide.dashStyle == QStringLiteral("solid")) {
            guideStyle = Qt::SolidLine;
        } else if (guide.dashStyle == QStringLiteral("dot")) {
            guideStyle = Qt::DotLine;
        }
        QPen guidePen(guideColor, selected ? kGuideSelectedPenPx : kGuidePenPx, guideStyle);
        guidePen.setCapStyle(Qt::RoundCap);
        painter.save();
        painter.setPen(guidePen);
        painter.setBrush(Qt::NoBrush);
        if (guide.orientation == DrawingCanvasProjectedGuideOrientation::Horizontal) {
            painter.drawLine(drawing_canvas::canvasToScreen(context.board, 0.0, guide.position), drawing_canvas::canvasToScreen(context.board, 1.0, guide.position));
            if (guide.showLabel && !guide.label.isEmpty()) {
                painter.setPen(guideColor);
                painter.drawText(drawing_canvas::canvasToScreen(context.board, 0.0, guide.position) + QPointF(kGuideLabelOffsetXPx, kGuideLabelHorizLabelOffsetYPx), guide.label);
            }
        } else {
            painter.drawLine(drawing_canvas::canvasToScreen(context.board, guide.position, 0.0), drawing_canvas::canvasToScreen(context.board, guide.position, 1.0));
            if (guide.showLabel && !guide.label.isEmpty()) {
                painter.setPen(guideColor);
                painter.drawText(drawing_canvas::canvasToScreen(context.board, guide.position, 0.0) + QPointF(kGuideLabelOffsetXPx, kGuideLabelVertLabelOffsetYPx), guide.label);
            }
        }
        painter.restore();
        return;
    }

    if (kind == QStringLiteral("construction_line")) {
        const DrawingCanvasProjectedLine &line = item.line;
        if (!line.ok) {
            return;
        }
        QPen constructionPen(selected ? context.palette.selection : context.palette.construction, selected ? kConstructionSelectedPenPx : kConstructionPenPx, Qt::DotLine);
        constructionPen.setCapStyle(Qt::RoundCap);
        painter.setPen(constructionPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(screenLine(context, line));
        return;
    }

    if (kind == QStringLiteral("dimension")) {
        const DrawingCanvasProjectedDimension &dimension = item.dimension;
        if (!dimension.ok) {
            return;
        }
        const QPointF a = drawing_canvas::canvasToScreen(context.board, dimension.x1, dimension.y1);
        const QPointF b = drawing_canvas::canvasToScreen(context.board, dimension.x2, dimension.y2);
        const QPointF dimA = drawing_canvas::canvasToScreen(context.board, dimension.dimensionX1, dimension.dimensionY1);
        const QPointF dimB = drawing_canvas::canvasToScreen(context.board, dimension.dimensionX2, dimension.dimensionY2);
        const QPointF label = drawing_canvas::canvasToScreen(context.board, dimension.labelX, dimension.labelY);
        const QColor dimensionColor = selected ? context.palette.selection : context.palette.dimension;
        QPen dimensionPen(dimensionColor, selected ? kDimLeaderPenPx : kDimTickPenPx);
        dimensionPen.setCapStyle(Qt::RoundCap);
        dimensionPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(dimensionPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(a, dimA);
        painter.drawLine(b, dimB);
        painter.drawLine(dimA, dimB);
        painter.drawLine(dimA + QPointF(-kDimCrossHalfPx, -kDimCrossHalfPx), dimA + QPointF(kDimCrossHalfPx, kDimCrossHalfPx));
        painter.drawLine(dimA + QPointF(-kDimCrossHalfPx, kDimCrossHalfPx), dimA + QPointF(kDimCrossHalfPx, -kDimCrossHalfPx));
        painter.drawLine(dimB + QPointF(-kDimCrossHalfPx, -kDimCrossHalfPx), dimB + QPointF(kDimCrossHalfPx, kDimCrossHalfPx));
        painter.drawLine(dimB + QPointF(-kDimCrossHalfPx, kDimCrossHalfPx), dimB + QPointF(kDimCrossHalfPx, -kDimCrossHalfPx));
        if (dimension.showLabel) {
            const QString text = dimension.label;
            const QRect textBounds = painter.fontMetrics().boundingRect(text);
            QRectF labelRect(
                label.x() - textBounds.width() / 2.0 - kDimLabelPadXPx,
                label.y() - textBounds.height() - kDimLabelTopOffsetPx,
                textBounds.width() + kDimLabelPadWidthPx,
                textBounds.height() + kDimLabelPadHeightPx);
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(context.palette.backdrop, selected ? 230 : 190));
            painter.drawRoundedRect(labelRect, kDimLabelCornerRadiusPx, kDimLabelCornerRadiusPx);
            painter.setPen(dimensionColor);
            painter.drawText(labelRect, Qt::AlignCenter, text);
        }
        return;
    }

    const DrawingCanvasProjectedStyle &style = item.style;
    QPen pen(
        selected ? context.palette.selection : QColor(style.strokeColor),
        selected ? drawing_canvas::kSelectedStrokePenPx : style.strokeWidth);
    if (!selected) {
        // Per-object opacity rides as pen alpha. Selection stays fully
        // opaque on purpose: a near-invisible object must still light up
        // when picked, or it cannot be found to edit. The plot-blocked
        // override below also stays opaque — warnings outrank styling.
        QColor faded = pen.color();
        faded.setAlphaF(style.strokeOpacity);
        pen.setColor(faded);
    }
    if (summary.plotBlocked) {
        pen.setColor(context.palette.safetyWarning);
    }
    // Per-object line style, the guide vocabulary applied to shapes.
    if (style.lineStyle == QStringLiteral("dash")) {
        pen.setStyle(Qt::DashLine);
    } else if (style.lineStyle == QStringLiteral("dot")) {
        pen.setStyle(Qt::DotLine);
    }
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    // Fill applies to CLOSED shapes only (rectangle, circle, ellipse-as-polygon,
    // polygon). The brush sits behind the outline because setBrush precedes the
    // drawRect/drawEllipse/drawPolygon calls below. Gated on fillOpacity > 0 and a
    // valid colour, so every existing object (opacity 0) keeps Qt::NoBrush and is
    // unchanged. Selection/plot-blocked tint the pen, never the fill. The point
    // kind sets its own brush below — its body IS the fill.
    const bool closedShape = kind == QStringLiteral("rectangle")
        || kind == QStringLiteral("circle")
        || kind == QStringLiteral("ellipse")
        || kind == QStringLiteral("polygon");
    QColor fillColor(style.fillColor);
    if (closedShape && style.fillOpacity > 0.0 && fillColor.isValid()) {
        fillColor.setAlphaF(style.fillOpacity);
        painter.setBrush(fillColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }

    if (kind == QStringLiteral("point")) {
        // The point is its FILL — a faded ring around an opaque disc would
        // make opacity a lie for the one kind whose body is brush-painted.
        // Selection and plot-blocked stay opaque, same rules as the pen.
        const DrawingCanvasProjectedPointObject &pointObject = item.point;
        if (!pointObject.ok) {
            return;
        }
        const QPointF point = drawing_canvas::canvasToScreen(context.board, pointObject.x, pointObject.y);
        QColor fill = selected ? context.palette.selection : context.palette.pointFill;
        if (!selected && !summary.plotBlocked) {
            fill.setAlphaF(style.strokeOpacity);
        }
        painter.setBrush(fill);
        painter.drawEllipse(point, kPointFillRadiusPx, kPointFillRadiusPx);
    } else if (kind == QStringLiteral("line")) {
        const DrawingCanvasProjectedLine &line = item.line;
        if (!line.ok) {
            return;
        }
        const QLineF screen = screenLine(context, line);
        painter.drawLine(screen);
        if (item.endArrow) {
            drawArrowHead(painter, screen);
        }
        if (item.startArrow) {
            // A head at the line's start: the same routine over the reversed
            // line, so its "end" lands on point a, pointing outward.
            drawArrowHead(painter, QLineF(screen.p2(), screen.p1()));
        }
    } else if (kind == QStringLiteral("rectangle")) {
        const DrawingCanvasProjectedRectangle &rectangle = item.rectangle;
        if (!rectangle.ok) {
            return;
        }
        drawRectangleShape(painter, context, rectangle);
    } else if (kind == QStringLiteral("circle")) {
        const DrawingCanvasProjectedCircle &circle = item.circle;
        if (!circle.ok) {
            return;
        }
        const QPointF center = drawing_canvas::canvasToScreen(context.board, circle.cx, circle.cy);
        const double radius = circle.radius * context.board.width();
        painter.drawEllipse(center, radius, radius);
    } else if (kind == QStringLiteral("wall")) {
        // A wall is a THICK LINE drawn as an oriented BAND: the centerline a->b
        // (like the line arm) widened by half its thickness on each side. The
        // half-thickness offset is scaled by board.width() to track zoom,
        // mirroring the circle arm's `radius * context.board.width()`.
        const DrawingCanvasProjectedWall &wall = item.wall;
        if (!wall.ok) {
            return;
        }
        const QPointF a = drawing_canvas::canvasToScreen(context.board, wall.ax, wall.ay);
        const QPointF b = drawing_canvas::canvasToScreen(context.board, wall.bx, wall.by);
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        const double length = std::hypot(dx, dy);
        const double halfPx = 0.5 * wall.thickness * context.board.width();
        // Perpendicular to the a->b unit vector (rotate +90deg), scaled to the
        // half-thickness in pixels. A degenerate (zero-length) wall has no
        // direction, so the unit vector falls back to +x and the band collapses
        // to a thin perpendicular sliver of length thickness — it does not
        // vanish, but a double-click-in-place is not a meaningful band.
        const double ux = length > 0.0 ? dx / length : 1.0;
        const double uy = length > 0.0 ? dy / length : 0.0;
        const QPointF offset(-uy * halfPx, ux * halfPx);
        // Free ends keep a SQUARE cap: the corners sit at a/b +/- the
        // perpendicular offset, so the band ends flat. Where this wall JOINS
        // another at a shared endpoint (M1.2), the annotateWallJoins pass flagged
        // that end, and below we add a mitered corner-fill so the two bands read
        // as one band instead of two square caps leaving an outer notch.
        QPolygonF band;
        band << (a + offset) << (b + offset) << (b - offset) << (a - offset);
        // M1.3 neutral render type: solid/door fill the band; window draws it
        // HOLLOW (outline only — a see-through opening); secret draws it DASHED
        // and hollow (hidden to a player, visible to the GM). The type is a
        // drawing concern only; the engine assigns any rule across Seam B.
        const bool filledBand = wall.type != QStringLiteral("window") && wall.type != QStringLiteral("secret");
        painter.save();
        QPen bandPen = painter.pen();
        bandPen.setJoinStyle(Qt::MiterJoin); // square corners on the band outline
        bandPen.setCapStyle(Qt::SquareCap);
        if (wall.type == QStringLiteral("secret")) {
            bandPen.setStyle(Qt::DashLine);
        }
        painter.setPen(bandPen);
        // A solid wall reads as a SOLID band (the stroke colour laid across its
        // whole width); a hollow type strokes the outline only.
        painter.setBrush(filledBand ? QBrush(bandPen.color()) : Qt::NoBrush);
        painter.drawPolygon(band);
        // Corner miters (M1.2): fill the outer notch at each joined end with the
        // same pen/brush, so the union of the two bands + this wedge is one
        // clean mitered corner. `into` points from the shared endpoint into THIS
        // wall; the neighbour's far point reconstructs its direction + thickness.
        const auto drawJoinCorner = [&](const QPointF &p, const QPointF &into, const DrawingCanvasWallJoin &join) {
            if (!join.drawCorner) {
                return;
            }
            const QPointF nf = drawing_canvas::canvasToScreen(context.board, join.neighborFarX, join.neighborFarY);
            const double ndx = nf.x() - p.x();
            const double ndy = nf.y() - p.y();
            const double nlen = std::hypot(ndx, ndy);
            if (nlen <= 0.0) {
                return;
            }
            const QPointF tNbr(ndx / nlen, ndy / nlen);
            const double hNbr = 0.5 * join.neighborThickness * context.board.width();
            const std::vector<QPointF> fill = wallCornerMiterFill(p, into, tNbr, halfPx, hNbr, 4.0);
            if (fill.size() >= 3) {
                QPolygonF wedge;
                for (const QPointF &pt : fill) {
                    wedge << pt;
                }
                painter.drawPolygon(wedge);
            }
        };
        // Only filled types miter: a solid corner wedge over a hollow/dashed
        // band would read wrong, so window/secret keep their square caps.
        if (filledBand) {
            drawJoinCorner(a, QPointF(ux, uy), wall.joinA);
            drawJoinCorner(b, QPointF(-ux, -uy), wall.joinB);
        }
        painter.restore();
    } else if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon") || kind == QStringLiteral("arc") || kind == QStringLiteral("ellipse") || kind == QStringLiteral("spline")) {
        const DrawingCanvasProjectedPolygon &projected = item.polygon;
        if (!projected.ok) {
            return;
        }
        QPolygonF polygon;
        for (const DrawingCanvasProjectedPoint &point : projected.points) {
            polygon.push_back(drawing_canvas::canvasToScreen(context.board, point.x, point.y));
        }
        if (kind == QStringLiteral("polygon") || kind == QStringLiteral("ellipse")) {
            painter.drawPolygon(polygon);
        } else {
            // Arc, polyline, and spline are open chains.
            painter.drawPolyline(polygon);
        }
    } else if (kind == QStringLiteral("text")) {
        const DrawingCanvasProjectedTextObject &text = item.text;
        if (!text.ok) {
            return;
        }
        // Cap height in canvas units -> pixel size via the board's pixel
        // height, clamped so a tiny/zoomed-out label still renders one pixel
        // tall instead of vanishing into a zero-size font.
        const double pixelSize = std::max(1.0, text.height * context.board.height());
        QFont font = painter.font();
        font.setPixelSize(static_cast<int>(std::lround(pixelSize)));
        const QPointF p = drawing_canvas::canvasToScreen(context.board, text.x, text.y);
        // Save/restore the font so the next scene item paints with the default
        // again; the painter's pen is already the styled stroke colour above,
        // which is exactly the colour we want the glyphs in.
        painter.save();
        painter.setFont(font);
        const QFontMetricsF fm(font);
        // The projected position is the glyph box TOP-LEFT; drawText's anchor
        // is the baseline, so drop by the ascent to put the box top at p.
        painter.drawText(QPointF(p.x(), p.y() + fm.ascent()), text.content);
        painter.restore();
    }

    if (context.plotDiagnostics && summary.plotBlocked && summary.bounds.ok) {
        QRectF warningRect = drawing_canvas::boundsToScreenRect(
            context.board, summary.bounds.x, summary.bounds.y, summary.bounds.width, summary.bounds.height);
        if (warningRect.width() < kPlotWarnBoxMinPx || warningRect.height() < kPlotWarnBoxMinPx) {
            warningRect = warningRect.adjusted(-kPlotWarnBoxAdjustPx, -kPlotWarnBoxAdjustPx, kPlotWarnBoxAdjustPx, kPlotWarnBoxAdjustPx);
        }
        painter.setPen(QPen(context.palette.safetyWarning, kPlotWarnBoxPenPx, Qt::DashLine));
        painter.setBrush(withAlpha(context.palette.safetyWarning, 24));
        painter.drawRect(warningRect);
        if (!summary.plotWarningKind.isEmpty()) {
            painter.setPen(context.palette.safetyWarning);
            painter.drawText(warningRect.topLeft() + QPointF(kPlotWarnLabelOffsetPx, -kPlotWarnLabelOffsetPx), summary.plotWarningKind);
        }
    }

    if (selected) {
        drawSelectedHandles(painter, item.source, context);
    }
}

void drawPreviewObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context)
{
    painter.save();
    QPen pen(context.palette.preview, kPreviewPenPx, Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QString kind = projectedObjectSummary(object).kind;
    if (kind == QStringLiteral("line")) {
        const DrawingCanvasProjectedLine line = projectedLine(object);
        if (line.ok) {
            const QLineF screen = screenLine(context, line);
            painter.drawLine(screen);
            if (object.value(QStringLiteral("end_arrow")).toBool()) {
                drawArrowHead(painter, screen); // preview the head while dragging
            }
        }
    } else if (kind == QStringLiteral("construction_line")) {
        const DrawingCanvasProjectedLine line = projectedLine(object);
        if (!line.ok) {
            painter.restore();
            return;
        }
        QPen constructionPen(context.palette.preview, kPreviewConstructionPenPx, Qt::DotLine);
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
        painter.drawText(drawing_canvas::canvasToScreen(context.board, dimension.labelX, dimension.labelY) + QPointF(kDimLabelOffsetPx, -kDimLabelOffsetPx), dimension.label);
    } else if (kind == QStringLiteral("rectangle")) {
        const DrawingCanvasProjectedRectangle rectangle = projectedRectangle(object);
        if (rectangle.ok) {
            drawRectangleShape(painter, context, rectangle); // preview rounded/frame too
        }
    } else if (kind == QStringLiteral("circle")) {
        const DrawingCanvasProjectedCircle circle = projectedCircle(object);
        if (circle.ok) {
            const QPointF center = drawing_canvas::canvasToScreen(context.board, circle.cx, circle.cy);
            const double radius = circle.radius * context.board.width();
            painter.drawEllipse(center, radius, radius);
        }
    } else if (kind == QStringLiteral("wall")) {
        // Live two-click preview: the same oriented band as the committed path,
        // under the dashed preview pen. thickness comes from the pending tool
        // option (default 0.1) carried into the preview object.
        const DrawingCanvasProjectedWall wall = projectedWall(object);
        if (wall.ok) {
            const QPointF a = drawing_canvas::canvasToScreen(context.board, wall.ax, wall.ay);
            const QPointF b = drawing_canvas::canvasToScreen(context.board, wall.bx, wall.by);
            const double dx = b.x() - a.x();
            const double dy = b.y() - a.y();
            const double length = std::hypot(dx, dy);
            const double halfPx = 0.5 * wall.thickness * context.board.width();
            const double ux = length > 0.0 ? dx / length : 1.0;
            const double uy = length > 0.0 ? dy / length : 0.0;
            const QPointF offset(-uy * halfPx, ux * halfPx);
            QPolygonF band;
            band << (a + offset) << (b + offset) << (b - offset) << (a - offset);
            painter.drawPolygon(band); // dashed preview pen, Qt::NoBrush set at top
        }
    } else if (kind == QStringLiteral("arc") || kind == QStringLiteral("spline")) {
        // Both preview as open chains of the projected (sampled) points.
        const DrawingCanvasProjectedPolygon projected = projectedPolygon(object);
        if (projected.ok) {
            QPolygonF chain;
            for (const DrawingCanvasProjectedPoint &point : projected.points) {
                chain.push_back(drawing_canvas::canvasToScreen(context.board, point.x, point.y));
            }
            painter.drawPolyline(chain);
        }
    } else if (kind == QStringLiteral("ellipse")) {
        const DrawingCanvasProjectedPolygon projected = projectedPolygon(object);
        if (projected.ok) {
            QPolygonF chain;
            for (const DrawingCanvasProjectedPoint &point : projected.points) {
                chain.push_back(drawing_canvas::canvasToScreen(context.board, point.x, point.y));
            }
            painter.drawPolygon(chain); // ellipse is a closed loop, unlike the arc
        }
    }
    painter.restore();
}

} // namespace drawing_canvas
