#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingSnap.h"

#include <QString>
#include <QVariantMap>

#include <string>

namespace drawing_core {

QString qStringFromStdString(const std::string &value);
// includeEditorFields: numeric spinner specs + physical-unit metadata for
// the geometry editor — built per request because only the ACTIVE object's
// editor consumes them, and building them for every object dominated the
// bulk-document projection cost.
QVariantMap draftingObjectToCanvasProjection(
    const edi::drafting::DraftingObject &object,
    const edi::drafting::DraftingGridProjection *grid = nullptr,
    bool includeEditorFields = true);
QVariantMap draftingDocumentToModelProjection(
    const edi::drafting::DraftingDocument &document,
    const edi::drafting::DraftingSnapSettings &snapSettings,
    const edi::drafting::DraftingGridProjection *grid = nullptr,
    const edi::drafting::DraftingObject *previewObject = nullptr);

} // namespace drawing_core
