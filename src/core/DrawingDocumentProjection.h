#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingSnap.h"

#include <QString>
#include <QVariantMap>

#include <string>

namespace drawing_core {

QString qStringFromStdString(const std::string &value);
QVariantMap draftingObjectToCanvasProjection(const edi::drafting::DraftingObject &object);
QVariantMap draftingDocumentToModelProjection(
    const edi::drafting::DraftingDocument &document,
    const edi::drafting::DraftingSnapSettings &snapSettings,
    const edi::drafting::DraftingObject *previewObject = nullptr);

} // namespace drawing_core
