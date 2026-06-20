#include "widgets/DrawingCanvasProjectedPlot.h"

#include <QVariantList>
#include <QVariantMap>

#include "EdiAssert.h"
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

    EDI_CHECK(parsed.hasPlotBounds);
    EDI_CHECK(parsed.travelSegments.size() == 1);
    EDI_CHECK(parsed.travelSegments[0].x1 == 0.1);
    EDI_CHECK(parsed.travelSegments[0].y1 == 0.2);
    EDI_CHECK(parsed.travelSegments[0].x2 == 0.3);
    EDI_CHECK(parsed.travelSegments[0].y2 == 0.4);
    EDI_CHECK(parsed.strokeSegments.size() == 1);
    EDI_CHECK(parsed.strokeSegments[0].x1 == 0.5);
    EDI_CHECK(parsed.strokeSegments[0].y1 == 0.6);
    EDI_CHECK(parsed.strokeSegments[0].x2 == 0.7);
    EDI_CHECK(parsed.strokeSegments[0].y2 == 0.8);

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
    EDI_CHECK(filtered.travelSegments.empty());
    EDI_CHECK(filtered.strokeSegments.empty());
    EDI_CHECK(!filtered.hasPlotBounds);

    const DrawingCanvasProjectedPlotPreview missing = projectedPlotPreview({});
    EDI_CHECK(missing.travelSegments.empty());
    EDI_CHECK(missing.strokeSegments.empty());
    EDI_CHECK(!missing.hasPlotBounds);

    const DrawingCanvasProjectedPlotPreview boundsOnly = projectedPlotPreview(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
    });
    EDI_CHECK(boundsOnly.travelSegments.empty());
    EDI_CHECK(boundsOnly.strokeSegments.empty());
    EDI_CHECK(boundsOnly.hasPlotBounds);

    const DrawingCanvasProjectedBoundsOverlay invisible = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), false},
        {QStringLiteral("plot_bounds"), bounds(0.1, 0.2, 0.3, 0.4)},
    });
    EDI_CHECK(!invisible.visible);

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
    EDI_CHECK(overlay.visible);
    EDI_CHECK(overlay.bounds.x == 0.1);
    EDI_CHECK(overlay.bounds.y == 0.2);
    EDI_CHECK(overlay.bounds.width == 0.3);
    EDI_CHECK(overlay.bounds.height == 0.4);
    EDI_CHECK(overlay.calibratedBoundsWarning);
    EDI_CHECK(overlay.warningKind == QStringLiteral("plot_outside_bounds"));
    EDI_CHECK(overlay.warningObjectId == QStringLiteral("object_1"));

    const DrawingCanvasProjectedBoundsOverlay badBounds = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
        {QStringLiteral("plot_bounds"), bounds(0.1, 0.2, std::numeric_limits<double>::quiet_NaN(), 0.4)},
        {QStringLiteral("warnings"), QVariantList{
            QVariantMap{{QStringLiteral("kind"), QStringLiteral("calibrated_plot_out_of_drawable_bounds")}},
        }},
    });
    EDI_CHECK(!badBounds.visible);
    EDI_CHECK(!badBounds.calibratedBoundsWarning);
    EDI_CHECK(badBounds.warningKind.isEmpty());

    const DrawingCanvasProjectedBoundsOverlay noWarning = projectedPlotBoundsOverlay(QVariantMap{
        {QStringLiteral("has_plot_bounds"), true},
        {QStringLiteral("plot_bounds"), bounds(0.0, 0.0, 1.0, 1.0)},
        {QStringLiteral("warnings"), QVariantList{
            QStringLiteral("ignored"),
            QVariantMap{},
        }},
    });
    EDI_CHECK(noWarning.visible);
    EDI_CHECK(!noWarning.calibratedBoundsWarning);
    EDI_CHECK(noWarning.warningKind.isEmpty());

    const DrawingCanvasProjectedSelectionBoundsOverlay noSelection = projectedSelectionBoundsOverlay(QVariantMap{
        {QStringLiteral("has_selection_plot_bounds"), false},
        {QStringLiteral("selection_plot_bounds"), bounds(0.1, 0.2, 0.3, 0.4)},
    });
    EDI_CHECK(!noSelection.visible);

    const DrawingCanvasProjectedSelectionBoundsOverlay selection = projectedSelectionBoundsOverlay(QVariantMap{
        {QStringLiteral("has_selection_plot_bounds"), true},
        {QStringLiteral("selection_plot_bounds"), bounds(0.2, 0.3, 0.4, 0.5)},
        {QStringLiteral("selection_plot_bounds_status"), QStringLiteral("inside")},
    });
    EDI_CHECK(selection.visible);
    EDI_CHECK(selection.bounds.x == 0.2);
    EDI_CHECK(selection.bounds.y == 0.3);
    EDI_CHECK(selection.bounds.width == 0.4);
    EDI_CHECK(selection.bounds.height == 0.5);
    EDI_CHECK(selection.status == QStringLiteral("inside"));

    const DrawingCanvasProjectedSelectionBoundsOverlay badSelection = projectedSelectionBoundsOverlay(QVariantMap{
        {QStringLiteral("has_selection_plot_bounds"), true},
        {QStringLiteral("selection_plot_bounds"), bounds(0.2, 0.3, 0.4, std::numeric_limits<double>::quiet_NaN())},
        {QStringLiteral("selection_plot_bounds_status"), QStringLiteral("inside")},
    });
    EDI_CHECK(!badSelection.visible);
    EDI_CHECK(badSelection.status.isEmpty());

    const DrawingCanvasProjectedSelectionBoundsOverlay missingStatus = projectedSelectionBoundsOverlay(QVariantMap{
        {QStringLiteral("has_selection_plot_bounds"), true},
        {QStringLiteral("selection_plot_bounds"), bounds(0.0, 0.0, 1.0, 1.0)},
    });
    EDI_CHECK(missingStatus.visible);
    EDI_CHECK(missingStatus.status.isEmpty());

    return 0;
}
