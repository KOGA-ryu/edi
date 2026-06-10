#include "core/RecipeController.h"

#include <QCoreApplication>
#include <QString>

#include <cassert>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    RecipeController controller;
    int changes = 0;
    QObject::connect(&controller, &RecipeController::recipeChanged, [&changes]() { ++changes; });

    // Accepted edits delegate to the pure ops and emit exactly one change.
    assert(controller.addStep(QStringLiteral("cube")));
    assert(changes == 1);
    assert(controller.document().steps.size() == 1);
    assert(controller.lastError().isEmpty());

    // Rejected edits emit nothing and surface the pure layer's message.
    assert(!controller.addStep(QStringLiteral("router")));
    assert(changes == 1);
    assert(!controller.lastError().isEmpty());
    assert(controller.document().steps.size() == 1);

    // A success after a failure clears the error.
    assert(controller.addStep(QStringLiteral("bevel")));
    assert(changes == 2);
    assert(controller.lastError().isEmpty());

    // Param edits flow through the same gate.
    assert(controller.setParamLiteral(0, QStringLiteral("size_x"), 3.0));
    assert(changes == 3);
    assert(controller.document().steps[0].params[0].value == 3.0);
    assert(!controller.setParamLiteral(0, QStringLiteral("nope"), 1.0));
    assert(changes == 3);

    assert(controller.bindParam(0, QStringLiteral("size_y"), QStringLiteral("plank"), QStringLiteral("width")));
    assert(changes == 4);
    assert(controller.document().steps[0].params[1].source == edi::recipe::ParamSource::Measurement);

    // Negative indexes are guarded before they reach size_t land.
    assert(!controller.removeStep(-1));
    assert(!controller.moveStep(-1, 0));
    assert(!controller.setParamLiteral(-2, QStringLiteral("size_x"), 1.0));
    assert(changes == 4);

    // The grammar still holds through the Qt layer.
    assert(!controller.moveStep(1, 0));
    assert(controller.removeStep(1));
    assert(changes == 5);

    return 0;
}
