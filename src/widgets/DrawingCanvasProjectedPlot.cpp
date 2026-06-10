#include "widgets/DrawingCanvasProjectedPlot.h"

#include <QVariantList>

#include "widgets/DrawingCanvasValues.h"

namespace drawing_canvas {
namespace {

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

bool readBounds(const QVariantMap &source, DrawingCanvasProjectedPlotBounds &target)
{
    return readFinite(source, QStringLiteral("x"), target.x)
        && readFinite(source, QStringLiteral("y"), target.y)
        && readFinite(source, QStringLiteral("width"), target.width)
        && readFinite(source, QStringLiteral("height"), target.height);
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
    overlay.visible = readBounds(bounds, overlay.bounds);
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

DrawingCanvasProjectedSelectionBoundsOverlay projectedSelectionBoundsOverlay(const QVariantMap &model)
{
    DrawingCanvasProjectedSelectionBoundsOverlay overlay;
    if (!model.value(QStringLiteral("has_selection_plot_bounds")).toBool()) {
        return overlay;
    }

    const QVariantMap bounds = model.value(QStringLiteral("selection_plot_bounds")).toMap();
    overlay.visible = readBounds(bounds, overlay.bounds);
    if (!overlay.visible) {
        return {};
    }

    overlay.status = model.value(QStringLiteral("selection_plot_bounds_status")).toString();
    return overlay;
}

} // namespace drawing_canvas
