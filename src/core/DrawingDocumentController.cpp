#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include <algorithm>
#include <cmath>

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
{
    m_model.insert(QStringLiteral("engine"), QStringLiteral("cpp_drawing_core_stub"));
    m_model.insert(QStringLiteral("generated_objects"), QVariantList{});
    m_model.insert(QStringLiteral("validation"), QVariantList{});
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    return m_model;
}

QString DrawingDocumentController::selectedToolId() const
{
    return m_selectedToolId;
}

QString DrawingDocumentController::selectedObjectId() const
{
    return m_selectedObjectId;
}

void DrawingDocumentController::clickCanvasNormalized(double x, double y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    emit modelChanged();
}
