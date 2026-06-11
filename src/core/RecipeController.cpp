#include "core/RecipeController.h"

using namespace edi::recipe;

RecipeController::RecipeController(QObject *parent)
    : QObject(parent)
{
    m_document.id = "recipe";
    m_document.name = "Recipe";
}

bool RecipeController::apply(const RecipeOpResult &result)
{
    if (!result.ok) {
        m_lastError = QString::fromStdString(result.message);
        return false;
    }
    m_lastError.clear();
    emit recipeChanged();
    return true;
}

bool RecipeController::addStep(const QString &shaperId)
{
    return apply(addShaperStep(m_document, shaperId.toStdString()));
}

bool RecipeController::removeStep(int index)
{
    if (index < 0) {
        return false;
    }
    return apply(removeShaperStep(m_document, static_cast<std::size_t>(index)));
}

bool RecipeController::moveStep(int from, int to)
{
    if (from < 0 || to < 0) {
        return false;
    }
    return apply(moveShaperStep(m_document, static_cast<std::size_t>(from), static_cast<std::size_t>(to)));
}

bool RecipeController::setParamLiteral(int stepIndex, const QString &paramId, double value)
{
    if (stepIndex < 0) {
        return false;
    }
    return apply(edi::recipe::setParamLiteral(m_document, static_cast<std::size_t>(stepIndex),
        paramId.toStdString(), value));
}

bool RecipeController::bindParam(int stepIndex, const QString &paramId,
                                 const QString &objectId, const QString &field)
{
    if (stepIndex < 0) {
        return false;
    }
    return apply(bindParamToMeasurement(m_document, static_cast<std::size_t>(stepIndex),
        paramId.toStdString(), {objectId.toStdString(), field.toStdString()}));
}

bool RecipeController::setStepProfile(int stepIndex, const QString &objectId)
{
    if (stepIndex < 0) {
        return false;
    }
    return apply(edi::recipe::setStepProfile(m_document, static_cast<std::size_t>(stepIndex),
        objectId.toStdString()));
}
