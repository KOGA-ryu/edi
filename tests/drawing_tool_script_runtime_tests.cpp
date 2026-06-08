#include "runtime/DrawingRuntimeCore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <iostream>

namespace {

void require(bool condition, const QString &message)
{
    if (!condition) {
        std::cerr << "failed: " << message.toStdString() << '\n';
        std::exit(1);
    }
}

QVariantMap readFixture(const QString &name)
{
    QFile file(QDir(QStringLiteral(PROJECT_SOURCE_DIR)).filePath(QStringLiteral("tests/fixtures/drawing_tool_scripts/") + name));
    require(file.open(QIODevice::ReadOnly), QString("fixture opens: %1").arg(name));
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    require(document.isObject(), QString("fixture is object: %1").arg(name));
    return document.object().toVariantMap();
}

QVariantList workflowEntries()
{
    const QVariantList workflows = readFixture("workflow_manifest.json").value("workflows").toList();
    QVariantList result;
    for (const QVariant &value : workflows) {
        const QVariantMap entry = value.toMap();
        if (!entry.value("fixture").toString().isEmpty()) {
            result.push_back(entry);
        }
    }
    return result;
}

QString failureText(const QVariantMap &result)
{
    QStringList failures = result.value("failures").toStringList();
    for (const QVariant &failure : result.value("failures").toList()) {
        failures.push_back(failure.toString());
    }
    return failures.join(";");
}

} // namespace

int main()
{
    DrawingToolScriptRuntime runtime;
    const QVariantMap library = readFixture("shared_canvas_library.json");

    const QVariantMap missing = runtime.validateScript({});
    require(!missing.value("ok").toBool(), "empty script fails validation");
    require(failureText(missing).contains("script missing name"), "validation requires name");

    QVariantMap invalidScript;
    invalidScript.insert("name", "bad_drag");
    invalidScript.insert("steps", QVariantList{
        QVariantMap{{"type", "dragHandle"}, {"object", "latest"}, {"to", QVariantMap{{"x", 1}, {"y", 2}}}},
        QVariantMap{{"use", "missingFragment"}},
    });
    const QVariantMap invalid = runtime.validateScript(invalidScript);
    require(!invalid.value("ok").toBool(), "invalid script fails");
    require(failureText(invalid).contains("dragHandle requires handleId"), "validation explains missing handleId: " + failureText(invalid));
    require(failureText(invalid).contains("unknown fragment"), "validation explains missing fragment: " + failureText(invalid));

    const QVariantMap linePlan = runtime.executionPlan(readFixture("line_create_basic.json"), library);
    require(linePlan.value("ok").toBool(), "line fixture builds execution plan");
    const QVariantMap line = linePlan.value("plan").toMap();
    require(line.value("steps").toList().size() == 3, "line fragment expands to three steps");
    require(line.value("steps").toList().value(1).toMap().value("point").toMap().value("x").toInt() == 128, "named point resolves x");
    require(line.value("metricsByMode").toMap().value("draw_click").toMap().value("maxRenderRequests").toInt() == 1, "named budget resolves");

    const QVariantMap handlePlan = runtime.executionPlan(readFixture("line_drag_end_handle.json"), library);
    require(handlePlan.value("ok").toBool(), "line handle drag fixture builds execution plan");
    const QVariantMap drag = handlePlan.value("plan").toMap().value("steps").toList().last().toMap();
    require(drag.value("type").toString() == "dragHandle", "last line drag step drags handle");
    require(drag.value("object").toString() == "latest", "drag handle preserves object alias");
    require(drag.value("handleId").toString() == "line_end", "drag handle preserves handle id");
    require(drag.value("to").toMap().value("x").toInt() == 320, "drag handle target resolves");
    require(handlePlan.value("plan").toMap().value("metricsByMode").toMap().value("dragging_handle").toMap().value("maxMutationsPerPointerMove").toDouble() == 2.1, "handle drag budget resolves");

    const QVariantMap arcPlan = runtime.executionPlan(readFixture("arc_create_basic.json"), library);
    require(arcPlan.value("ok").toBool(), "arc fixture builds execution plan");
    const QVariantList arcSteps = arcPlan.value("plan").toMap().value("steps").toList();
    require(arcSteps.size() == 6, "arc expands parameter steps and clicks");
    require(arcSteps.value(1).toMap().value("parameter").toString() == "circle_arc_mode", "arc mode parameter is explicit");
    require(arcSteps.value(1).toMap().value("value").toString() == "arc", "arc mode value is preserved");

    const QVariantList workflows = workflowEntries();
    require(workflows.size() >= 13, "workflow manifest includes control workflows");
    for (const QVariant &workflowValue : workflows) {
        const QVariantMap workflow = workflowValue.toMap();
        require(!workflow.value("kind").toString().isEmpty(), "workflow kind is present");
        require(!workflow.value("category").toString().isEmpty(), "workflow category is present");
        require(!workflow.value("tags").toList().isEmpty(), "workflow tags are present");
        const QString fixture = workflow.value("fixture").toString();
        const QVariantMap plan = runtime.executionPlan(readFixture(fixture), library);
        require(plan.value("ok").toBool(), fixture + " builds execution plan");
        require(!plan.value("plan").toMap().value("steps").toList().isEmpty(), fixture + " has executable steps");
        const QVariantMap driver = runtime.driverPlan(plan);
        require(driver.value("ok").toBool(), fixture + " builds driver plan");
        require(!driver.value("plan").toMap().value("ops").toList().isEmpty(), fixture + " emits driver ops");
    }

    const QVariantList dragOps = runtime.stepDriverOps({
        {"type", "dragHandle"},
        {"object", "latest"},
        {"handleId", "line_end"},
        {"from", QVariantMap{{"x", 256}, {"y", 128}}},
        {"to", QVariantMap{{"x", 320}, {"y", 160}}},
        {"pointerMoves", 4},
    });
    require(dragOps.size() == 8, "drag handle emits target, pointer path, and release");
    require(dragOps.value(0).toMap().value("op").toString() == "targetHandle", "drag handle targets first");
    require(dragOps.value(3).toMap().value("x").toInt() == 272, "drag interpolation first x is one quarter");
    require(dragOps.value(7).toMap().value("op").toString() == "pointerUp", "drag releases pointer");

    const QVariantMap cycle = runtime.expandedSteps({
        {"name", "cycle"},
        {"fragments", QVariantMap{
            {"a", QVariantList{QVariantMap{{"use", "b"}}}},
            {"b", QVariantList{QVariantMap{{"use", "a"}}}},
        }},
        {"steps", QVariantList{QVariantMap{{"use", "a"}}}},
    });
    require(!cycle.value("ok").toBool(), "fragment cycle fails expansion");
    require(failureText(cycle).contains("fragment cycle"), "cycle failure is explicit");

    return 0;
}
