#include "widgets/DrawingCanvasProjectedPlot.h"

#include <QVariantList>

#include <cmath>

namespace drawing_canvas {
namespace {

bool readFinite(const QVariantMap &source, const QString &field, double &target)
{
    bool ok = false;
    const double value = source.value(field).toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        return false;
    }
    target = value;
    return true;
}

std::vector<DrawingCanvasProjectedSegment> projectedSegments(const QVariantList &source)
{
    std::vector<DrawingCanvasProjectedSegment> segments;
    segments.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        const QVariantMap row = entry.toMap();
        if (row.isEmpty()) {
            continue;
        }

        DrawingCanvasProjectedSegment segment;
        if (readFinite(row, QStringLiteral("x1"), segment.x1)
            && readFinite(row, QStringLiteral("y1"), segment.y1)
            && readFinite(row, QStringLiteral("x2"), segment.x2)
            && readFinite(row, QStringLiteral("y2"), segment.y2)) {
            segments.push_back(segment);
        }
    }
    return segments;
}

} // namespace

DrawingCanvasProjectedPlotPreview projectedPlotPreview(const QVariantMap &plotSummary)
{
    const QVariantMap preview = plotSummary.value(QStringLiteral("preview")).toMap();

    DrawingCanvasProjectedPlotPreview projected;
    projected.hasPlotBounds = plotSummary.value(QStringLiteral("has_plot_bounds")).toBool();
    projected.travelSegments = projectedSegments(preview.value(QStringLiteral("travel_segments")).toList());
    projected.strokeSegments = projectedSegments(preview.value(QStringLiteral("segments")).toList());
    return projected;
}

} // namespace drawing_canvas
