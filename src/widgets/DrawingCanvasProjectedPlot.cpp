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

DrawingCanvasProjectedBoundsOverlay projectedPlotBoundsOverlay(const QVariantMap &plotSummary)
{
    DrawingCanvasProjectedBoundsOverlay overlay;
    if (!plotSummary.value(QStringLiteral("has_plot_bounds")).toBool()) {
        return overlay;
    }

    const QVariantMap bounds = plotSummary.value(QStringLiteral("plot_bounds")).toMap();
    overlay.visible = readFinite(bounds, QStringLiteral("x"), overlay.bounds.x)
        && readFinite(bounds, QStringLiteral("y"), overlay.bounds.y)
        && readFinite(bounds, QStringLiteral("width"), overlay.bounds.width)
        && readFinite(bounds, QStringLiteral("height"), overlay.bounds.height);
    if (!overlay.visible) {
        return {};
    }

    const QVariantList warnings = plotSummary.value(QStringLiteral("warnings")).toList();
    for (const QVariant &warningValue : warnings) {
        const QVariantMap warning = warningValue.toMap();
        if (warning.value(QStringLiteral("kind")).toString() == QStringLiteral("calibrated_plot_out_of_drawable_bounds")) {
            overlay.calibratedBoundsWarning = true;
            break;
        }
    }

    overlay.warningKind = plotSummary.value(QStringLiteral("first_warning_kind")).toString();
    overlay.warningObjectId = plotSummary.value(QStringLiteral("first_warning_object_id")).toString();
    return overlay;
}

} // namespace drawing_canvas
