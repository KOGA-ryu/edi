#include "runtime/DrawingRuntimeCore.h"

#include <iostream>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "failed: " << message << '\n';
        std::exit(1);
    }
}

bool containsId(const QVariantList &rows, const QString &id)
{
    for (const QVariant &row : rows) {
        if (row.toMap().value("id").toString() == id) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    DrawingToolCatalog catalog;
    const QVariantList toolModes = catalog.toolModes();
    require(toolModes.size() == 19, "tool catalog keeps all drawing modes");
    require(containsId(toolModes, "anchor_points"), "tool catalog includes anchor_points");
    require(containsId(toolModes, "regular_polygon"), "tool catalog includes regular_polygon");

    const QVariantMap settings = catalog.toolSettingsById();
    require(settings.contains("circle_arc"), "settings include circle_arc");
    require(settings.value("circle_arc").toList().size() == 4, "circle_arc has stable setting rows");

    const QVariantList sidebars = catalog.sidebarSections();
    require(sidebars.size() == 11, "sidebar sections count is stable");
    require(sidebars.first().toMap().value("id").toString() == "draw", "first sidebar section is draw");

    DrawingRuntimeRows rows;
    require(rows.editNumber(1.23456) == "1.235", "editNumber rounds to 3 decimals");
    require(rows.sidebarRowClickable({{"action", "tool"}}), "action-backed sidebar rows are clickable");
    require(!rows.sidebarRowClickable({{"action", ""}}), "empty-action sidebar rows are not clickable");

    DrawingInteractionRuntime interaction;
    QVariantMap metrics = interaction.beginMetricsInteraction(interaction.initialMetricsState(), "dragging_object", 1000, {
        {"revision", 20},
        {"selectedCount", 1},
        {"visibleObjectCount", 24},
    });
    metrics = interaction.recordMetricsPointerMove(metrics);
    metrics = interaction.recordMetricsPointerMove(metrics);
    metrics = interaction.recordMetricsSnap(metrics, 2);
    metrics = interaction.recordMetricsHitTest(metrics);
    metrics = interaction.recordMetricsControllerMutation(metrics, "move_selected");
    metrics = interaction.recordMetricsRenderRequest(metrics);
    const QVariantMap finishedMetrics = interaction.finishMetricsInteraction(metrics, 1425, {
        {"revision", 21},
        {"selectedCount", 1},
        {"visibleObjectCount", 24},
    });
    const QVariantMap record = finishedMetrics.value("record").toMap();
    require(!finishedMetrics.value("state").toMap().value("active").toBool(), "finish clears active metrics state");
    require(record.value("mode").toString() == "dragging_object", "metrics preserve mode");
    require(record.value("durationMs").toInt() == 425, "metrics compute duration");
    require(record.value("pointerMoves").toInt() == 2, "metrics count pointer moves");
    require(record.value("snapResolutions").toInt() == 2, "metrics count snap resolutions");
    require(record.value("revisionDelta").toInt() == 1, "metrics compute revision delta");
    require(record.value("events").toList().size() == 6, "metrics retain event trace");

    const QVariantMap idleMetrics = interaction.recordMetricsPointerMove(interaction.initialMetricsState());
    require(idleMetrics.value("pointerMoves").toInt() == 0, "inactive metrics ignore pointer moves");

    QVariantMap canceledMetrics = interaction.beginMetricsInteraction(interaction.initialMetricsState(), "dragging_handle", 50, {
        {"revision", 3},
        {"selectedCount", 1},
        {"visibleObjectCount", 4},
    });
    canceledMetrics = interaction.recordMetricsHandlePlan(canceledMetrics);
    const QVariantMap canceledResult = interaction.cancelMetricsInteraction(canceledMetrics, 75, {
        {"revision", 3},
        {"selectedCount", 1},
        {"visibleObjectCount", 4},
    });
    require(canceledResult.value("record").toMap().value("canceled").toBool(), "cancel marks metrics record");
    require(canceledResult.value("record").toMap().value("handlePlans").toInt() == 1, "cancel preserves metrics counts");

    const QVariantMap budgetOk = interaction.assertWithinBudget(record, {
        {"mode", "dragging_object"},
        {"maxDurationMs", 500},
        {"maxPointerMoves", 2},
        {"maxControllerMutations", 1},
        {"maxRenderRequests", 1},
        {"maxHitTests", 1},
        {"maxSnapResolutions", 2},
        {"revisionDelta", 1},
    });
    require(budgetOk.value("ok").toBool(), "matching budget passes");
    const QVariantMap budgetBad = interaction.assertWithinBudget(record, {
        {"mode", "dragging_handle"},
        {"maxPointerMoves", 1},
        {"revisionDelta", 0},
    });
    require(!budgetBad.value("ok").toBool(), "exceeded budget fails");
    require(budgetBad.value("failures").toStringList().size() == 3, "budget reports expected failures");

    QVariantMap telemetry = interaction.beginTelemetryInteraction(interaction.initialTelemetryState(), "dragging_handle", 1000, {
        {"revision", 20},
        {"selectedCount", 1},
        {"visibleObjectCount", 24},
    });
    telemetry = interaction.recordTelemetryHitTest(telemetry);
    telemetry = interaction.recordTelemetryPointerMove(telemetry);
    telemetry = interaction.recordTelemetrySnap(telemetry);
    telemetry = interaction.recordTelemetryHandlePlan(telemetry);
    telemetry = interaction.recordTelemetryControllerMutation(telemetry, "update_handle_field");
    telemetry = interaction.recordTelemetryRenderRequest(telemetry);
    const QVariantMap finishedTelemetry = interaction.finishTelemetryInteraction(telemetry, 1120, {
        {"revision", 21},
        {"selectedCount", 1},
        {"visibleObjectCount", 24},
    });
    const QVariantList events = finishedTelemetry.value("events").toList();
    require(!finishedTelemetry.value("state").toMap().value("active").toBool(), "finish clears telemetry state");
    require(events.size() == 8, "telemetry finish includes begin, events, and finish");
    require(events.first().toMap().value("type").toString() == "begin", "telemetry begins with begin event");
    require(events.last().toMap().value("type").toString() == "finish", "telemetry ends with finish event");
    require(finishedTelemetry.value("state").toMap().value("completedEvents").toList().size() == events.size(), "telemetry keeps completed stream");

    const QVariantMap idleTelemetry = interaction.recordTelemetryPointerMove(interaction.initialTelemetryState());
    require(idleTelemetry.value("events").toList().isEmpty(), "inactive telemetry ignores events");

    QVariantMap cancelTelemetry = interaction.beginTelemetryInteraction(interaction.initialTelemetryState(), "marquee_select", 50, {
        {"revision", 3},
        {"selectedCount", 0},
        {"visibleObjectCount", 4},
    });
    cancelTelemetry = interaction.recordTelemetryPointerMove(cancelTelemetry, 2);
    const QVariantMap telemetryCanceled = interaction.cancelTelemetryInteraction(cancelTelemetry, 75, {
        {"revision", 3},
        {"selectedCount", 0},
        {"visibleObjectCount", 4},
    });
    require(telemetryCanceled.value("events").toList().last().toMap().value("type").toString() == "cancel", "telemetry cancel ends stream with cancel");
    require(telemetryCanceled.value("events").toList().value(1).toMap().value("count").toInt() == 2, "telemetry preserves explicit event count");

    return 0;
}
