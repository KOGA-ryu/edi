#include "widgets/DrawingCanvasProjectedGrid.h"

#include <QVariantList>

#include "widgets/DrawingCanvasValues.h"

namespace drawing_canvas {
namespace {

bool isSupportedAxis(const QString &axis)
{
    return axis == QStringLiteral("vertical") || axis == QStringLiteral("horizontal");
}

std::vector<DrawingCanvasProjectedGridLine> projectedGridLines(const QVariantList &source)
{
    std::vector<DrawingCanvasProjectedGridLine> lines;
    lines.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        const QVariantMap row = entry.toMap();
        if (row.isEmpty()) {
            continue;
        }

        DrawingCanvasProjectedGridLine line;
        line.axis = row.value(QStringLiteral("axis")).toString();
        if (!isSupportedAxis(line.axis) || !readFinite(row, QStringLiteral("position"), line.position)) {
            continue;
        }

        line.major = row.value(QStringLiteral("major")).toBool();
        lines.push_back(line);
    }
    return lines;
}

DrawingCanvasProjectedGridBounds projectedGridBounds(const QVariantMap &source)
{
    DrawingCanvasProjectedGridBounds bounds;
    bounds.visible = readFinite(source, QStringLiteral("x"), bounds.x)
        && readFinite(source, QStringLiteral("y"), bounds.y)
        && readFinite(source, QStringLiteral("width"), bounds.width)
        && readFinite(source, QStringLiteral("height"), bounds.height)
        && bounds.width >= 0.0
        && bounds.height >= 0.0;
    if (!bounds.visible) {
        return {};
    }
    return bounds;
}

DrawingCanvasProjectedGridPoint projectedGridPoint(const QVariantMap &source)
{
    DrawingCanvasProjectedGridPoint point;
    point.visible = readFinite(source, QStringLiteral("x"), point.x)
        && readFinite(source, QStringLiteral("y"), point.y);
    if (!point.visible) {
        return {};
    }
    return point;
}

} // namespace

DrawingCanvasProjectedGrid projectedGrid(const QVariantMap &model)
{
    const QVariantMap grid = model.value(QStringLiteral("grid")).toMap();
    if (grid.isEmpty()) {
        return {};
    }

    DrawingCanvasProjectedGrid projected;
    projected.lines = projectedGridLines(grid.value(QStringLiteral("lines")).toList());
    projected.drawableBounds = projectedGridBounds(grid.value(QStringLiteral("drawable_bounds")).toMap());
    projected.origin = projectedGridPoint(grid.value(QStringLiteral("origin")).toMap());
    return projected;
}

} // namespace drawing_canvas
