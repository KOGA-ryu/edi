#include "widgets/DrawingCanvasProjectedStatus.h"

#include <QVariantMap>

#include <cassert>
#include <limits>

using namespace drawing_canvas;

int main()
{
    const DrawingCanvasProjectedStatus missing = projectedCanvasStatus({});
    assert(missing.gridWidth == 0.0);
    assert(missing.gridHeight == 0.0);
    assert(missing.gridUnitLabel.isEmpty());
    assert(missing.plotStatus == QStringLiteral("blocked"));
    assert(missing.plotWarningCount == 0);
    assert(missing.firstWarningKind.isEmpty());

    const DrawingCanvasProjectedStatus parsed = projectedCanvasStatus(QVariantMap{
        {QStringLiteral("grid"), QVariantMap{
            {QStringLiteral("width"), 11.0},
            {QStringLiteral("height"), 8.5},
            {QStringLiteral("unit_label"), QStringLiteral("in")},
        }},
        {QStringLiteral("plot_summary"), QVariantMap{
            {QStringLiteral("status"), QStringLiteral("ready")},
            {QStringLiteral("warning_count"), 2},
            {QStringLiteral("first_warning_kind"), QStringLiteral("outside_drawable")},
        }},
    });
    assert(parsed.gridWidth == 11.0);
    assert(parsed.gridHeight == 8.5);
    assert(parsed.gridUnitLabel == QStringLiteral("in"));
    assert(parsed.plotStatus == QStringLiteral("ready"));
    assert(parsed.plotWarningCount == 2);
    assert(parsed.firstWarningKind == QStringLiteral("outside_drawable"));

    const DrawingCanvasProjectedStatus malformed = projectedCanvasStatus(QVariantMap{
        {QStringLiteral("grid"), QVariantMap{
            {QStringLiteral("width"), std::numeric_limits<double>::quiet_NaN()},
            {QStringLiteral("height"), QStringLiteral("bad")},
            {QStringLiteral("unit_label"), QStringLiteral("mm")},
        }},
        {QStringLiteral("plot_summary"), QVariantMap{
            {QStringLiteral("status"), QString()},
            {QStringLiteral("warning_count"), -8},
            {QStringLiteral("first_warning_kind"), QStringLiteral("kept")},
        }},
    });
    assert(malformed.gridWidth == 0.0);
    assert(malformed.gridHeight == 0.0);
    assert(malformed.gridUnitLabel == QStringLiteral("mm"));
    assert(malformed.plotStatus == QStringLiteral("blocked"));
    assert(malformed.plotWarningCount == 0);
    assert(malformed.firstWarningKind == QStringLiteral("kept"));

    return 0;
}
