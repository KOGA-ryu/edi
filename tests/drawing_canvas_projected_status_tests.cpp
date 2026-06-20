#include "widgets/DrawingCanvasProjectedStatus.h"

#include <QVariantMap>

#include "EdiAssert.h"
#include <limits>

using namespace drawing_canvas;

int main()
{
    const DrawingCanvasProjectedStatus missing = projectedCanvasStatus({});
    EDI_CHECK(missing.gridWidth == 0.0);
    EDI_CHECK(missing.gridHeight == 0.0);
    EDI_CHECK(missing.gridUnitLabel.isEmpty());
    EDI_CHECK(missing.plotStatus == QStringLiteral("blocked"));
    EDI_CHECK(missing.plotWarningCount == 0);
    EDI_CHECK(missing.firstWarningKind.isEmpty());

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
    EDI_CHECK(parsed.gridWidth == 11.0);
    EDI_CHECK(parsed.gridHeight == 8.5);
    EDI_CHECK(parsed.gridUnitLabel == QStringLiteral("in"));
    EDI_CHECK(parsed.plotStatus == QStringLiteral("ready"));
    EDI_CHECK(parsed.plotWarningCount == 2);
    EDI_CHECK(parsed.firstWarningKind == QStringLiteral("outside_drawable"));

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
    EDI_CHECK(malformed.gridWidth == 0.0);
    EDI_CHECK(malformed.gridHeight == 0.0);
    EDI_CHECK(malformed.gridUnitLabel == QStringLiteral("mm"));
    EDI_CHECK(malformed.plotStatus == QStringLiteral("blocked"));
    EDI_CHECK(malformed.plotWarningCount == 0);
    EDI_CHECK(malformed.firstWarningKind == QStringLiteral("kept"));

    return 0;
}
