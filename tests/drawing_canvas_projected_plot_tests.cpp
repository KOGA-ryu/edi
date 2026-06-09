#include "widgets/DrawingCanvasProjectedPlot.h"

#include <QVariantList>
#include <QVariantMap>

#include <cassert>
#include <limits>

using namespace drawing_canvas;

namespace {

QVariantMap segment(double x1, double y1, double x2, double y2)
{
    return QVariantMap{
        {QStringLiteral("x1"), x1},
        {QStringLiteral("y1"), y1},
        {QStringLiteral("x2"), x2},
        {QStringLiteral("y2"), y2},
    };
}

QVariantMap plotSummary(const QVariantList &travelSegments, const QVariantList &strokeSegments, bool hasPlotBounds = false)
{
    return QVariantMap{
        {QStringLiteral("has_plot_bounds"), hasPlotBounds},
        {QStringLiteral("preview"), QVariantMap{
            {QStringLiteral("travel_segments"), travelSegments},
            {QStringLiteral("segments"), strokeSegments},
        }},
    };
}

QVariantMap bounds(double x, double y, double width, double height)
{
    return QVariantMap{
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
    };
}

} // namespace

int main()
{
    const DrawingCanvasProjectedPlotPreview parsed = projectedPlotPreview(plotSummary(
        QVariantList{
            segment(0.1, 0.2, 0.3, 0.4),
            QStringLiteral("ignored"),
            QVariantMap{},
        },
        QVariantList{
            segment(0.5, 0.6, 0.7, 0.8),
        },
        true));

    assert(parsed.hasPlotBounds);
    assert(parsed.travelSegments.size() == 1);
    assert(parsed.travelSegments[0].x1 == 0.1);
    assert(parsed.travelSegments[0].y1 == 0.2);
    assert(parsed.travelSegments[0].x2 == 0.3);
    assert(parsed.travelSegments[0].y2 == 0.4);
    assert(parsed.strokeSegments.size() == 1);
    assert(parsed.strokeSegments[0].x1 == 0.5);
    assert(parsed.strokeSegments[0].y1 == 0.6);
    assert(parsed.strokeSegments[0].x2 == 0.7);
    assert(parsed.strokeSegments[0].y2 == 0.8);

    const DrawingCanvasProjectedPlotPreview filtered = projectedPlotPreview(plotSummary(
        QVariantList{
            QVariantMap{
                {QStringLiteral("x1"), 0.1},
                {QStringLiteral("y1"), 0.2},
                {QStringLiteral("x2"), std::numeric_limits<double>::quiet_NaN()},
                {QStringLiteral("y2"), 0.4},
            },
            QVariantMap{
                {QStringLiteral("x1"), 0.1},
                {QStringLiteral("y1"), QStringLiteral("bad")},
                {QStringLiteral("x2"), 0.3},
                {QStringLiteral("y2"), 0.4},
            },
        },
        QVariantList{
            QVariantMap{
                {QStringLiteral("x1"), 0.5},
                {QStringLiteral("y1"), 0.6},
                {QStringLiteral("x2"), 0.7},
            },
        }));
    assert(filtered.travelSegments.empty());
    assert(filtered.strokeSegments.empty());
    assert(!filtered.hasPlotBounds);

    const DrawingCanvasProjectedPlotPreview missing = projectedPlotPreview({});
    assert(missing.travelSegments.empty());
    assert(missing.strokeSegments.empty());
    assert(!missing.hasPlotBounds);

    const DrawingCanvasProjectedPlotPreview boundsOnly = projectedPlotPreview(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
    });
    assert(boundsOnly.travelSegments.empty());
    assert(boundsOnly.strokeSegments.empty());
    assert(boundsOnly.hasPlotBounds);

    const DrawingCanvasProjectedBoundsOverlay invisible = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), false},
        {QStringLiteral("plot_bounds"), bounds(0.1, 0.2, 0.3, 0.4)},
    });
    assert(!invisible.visible);

    const DrawingCanvasProjectedBoundsOverlay overlay = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
        {QStringLiteral("plot_bounds"), bounds(0.1, 0.2, 0.3, 0.4)},
        {QStringLiteral("warnings"), QVariantList{
            QStringLiteral("ignored"),
            QVariantMap{{QStringLiteral("kind"), QStringLiteral("other_warning")}},
            QVariantMap{{QStringLiteral("kind"), QStringLiteral("calibrated_plot_out_of_drawable_bounds")}},
        }},
        {QStringLiteral("first_warning_kind"), QStringLiteral("plot_outside_bounds")},
        {QStringLiteral("first_warning_object_id"), QStringLiteral("object_1")},
    });
    assert(overlay.visible);
    assert(overlay.bounds.x == 0.1);
    assert(overlay.bounds.y == 0.2);
    assert(overlay.bounds.width == 0.3);
    assert(overlay.bounds.height == 0.4);
    assert(overlay.calibratedBoundsWarning);
    assert(overlay.warningKind == QStringLiteral("plot_outside_bounds"));
    assert(overlay.warningObjectId == QStringLiteral("object_1"));

    const DrawingCanvasProjectedBoundsOverlay badBounds = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
        {QStringLiteral("plot_bounds"), bounds(0.1, 0.2, std::numeric_limits<double>::quiet_NaN(), 0.4)},
        {QStringLiteral("warnings"), QVariantList{
            QVariantMap{{QStringLiteral("kind"), QStringLiteral("calibrated_plot_out_of_drawable_bounds")}},
        }},
    });
    assert(!badBounds.visible);
    assert(!badBounds.calibratedBoundsWarning);
    assert(badBounds.warningKind.isEmpty());

    const DrawingCanvasProjectedBoundsOverlay noWarning = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
        {QStringLiteral("plot_bounds"), bounds(0.0, 0.0, 1.0, 1.0)},
        {QStringLiteral("warnings"), QVariantList{
            QStringLiteral("ignored"),
            QVariantMap{},
        }},
    });
    assert(noWarning.visible);
    assert(!noWarning.calibratedBoundsWarning);
    assert(noWarning.warningKind.isEmpty());

    return 0;
}
