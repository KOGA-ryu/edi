#include "widgets/DrawingCanvasProjectedStatus.h"

#include "widgets/DrawingCanvasValues.h"

namespace drawing_canvas {
namespace {

int nonNegativeInt(const QVariant &value)
{
    bool ok = false;
    const int number = value.toInt(&ok);
    if (!ok || number < 0) {
        return 0;
    }
    return number;
}

} // namespace

DrawingCanvasProjectedStatus projectedCanvasStatus(const QVariantMap &model)
{
    const QVariantMap grid = model.value(QStringLiteral("grid")).toMap();
    const QVariantMap plot = model.value(QStringLiteral("plot_summary")).toMap();

    DrawingCanvasProjectedStatus status;
    status.gridWidth = finiteNumber(grid.value(QStringLiteral("width")), 0.0);
    status.gridHeight = finiteNumber(grid.value(QStringLiteral("height")), 0.0);
    status.gridUnitLabel = grid.value(QStringLiteral("unit_label")).toString();
    status.plotStatus = plot.value(QStringLiteral("status"), QStringLiteral("blocked")).toString();
    if (status.plotStatus.isEmpty()) {
        status.plotStatus = QStringLiteral("blocked");
    }
    status.plotWarningCount = nonNegativeInt(plot.value(QStringLiteral("warning_count")));
    status.firstWarningKind = plot.value(QStringLiteral("first_warning_kind")).toString();
    return status;
}

} // namespace drawing_canvas
