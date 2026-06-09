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

    return 0;
}
