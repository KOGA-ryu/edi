#include "drafting/DraftingDocument.h"

#include "drafting/DraftingGeometry.h"

#include <iomanip>
#include <sstream>
#include <utility>

namespace edi::drafting {

DraftingObjectBuildResult DraftingObjectBuildResult::accepted(DraftingObject object)
{
    DraftingObjectBuildResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.object = std::move(object);
    return result;
}

DraftingObjectBuildResult DraftingObjectBuildResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingObjectBuildResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

DraftingObject makeDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    DraftingObject object;
    object.id = std::move(id);
    object.kind = kind;
    object.geometry = std::move(geometry);
    return object;
}

DraftingObjectBuildResult validateDraftingObjectShape(const DraftingObject &object)
{
    if (!isValidDraftingObjectId(object.id)) {
        return DraftingObjectBuildResult::rejected(DraftingResultCode::EmptyObjectId, "object id is required");
    }
    if (!kindMatchesGeometry(object.kind, object.geometry)) {
        return DraftingObjectBuildResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }
    const auto geometryValidation = validateGeometry(object.geometry);
    if (!geometryValidation.ok) {
        return DraftingObjectBuildResult::rejected(geometryValidation.code, geometryValidation.message);
    }

    return DraftingObjectBuildResult::accepted(object);
}

DraftingObjectBuildResult buildDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    DraftingObject object = makeDraftingObject(std::move(id), kind, std::move(geometry));
    auto validation = validateDraftingObjectShape(object);
    if (!validation.ok) {
        return validation;
    }
    return DraftingObjectBuildResult::accepted(std::move(object));
}

DraftingObjectId draftingObjectIdForSerial(const std::string &prefix, int serial)
{
    std::ostringstream stream;
    stream << prefix << '_' << std::setfill('0') << std::setw(4) << serial;
    return stream.str();
}

DraftingLayer makeDraftingLayer(LayerId id, std::string name, int order)
{
    DraftingLayer layer;
    layer.id = std::move(id);
    if (isValidLayerName(name)) {
        layer.name = std::move(name);
    } else if (isValidLayerId(layer.id)) {
        layer.name = layer.id;
    } else {
        layer.name.clear();
    }
    layer.order = order;
    return layer;
}

DraftingLayer makeDefaultLayer()
{
    return makeDraftingLayer("default", "Default", 0);
}

DraftingDocument makeDraftingDocument(DraftingDocumentId id, std::string title)
{
    DraftingDocument document;
    document.id = std::move(id);
    if (isValidDraftingDocumentTitle(title)) {
        document.title = std::move(title);
    } else if (isValidDraftingDocumentId(document.id)) {
        document.title = document.id;
    }
    document.layers.push_back(makeDefaultLayer());
    return document;
}

std::optional<std::size_t> objectIndexById(const DraftingDocument &document, const DraftingObjectId &id)
{
    for (std::size_t index = 0; index < document.objects.size(); ++index) {
        if (document.objects[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

DraftingObject *findObject(DraftingDocument &document, const DraftingObjectId &id)
{
    const auto index = objectIndexById(document, id);
    return index ? &document.objects[*index] : nullptr;
}

const DraftingObject *findObject(const DraftingDocument &document, const DraftingObjectId &id)
{
    const auto index = objectIndexById(document, id);
    return index ? &document.objects[*index] : nullptr;
}

std::optional<std::size_t> layerIndexById(const DraftingDocument &document, const LayerId &id)
{
    for (std::size_t index = 0; index < document.layers.size(); ++index) {
        if (document.layers[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

DraftingLayer *findLayer(DraftingDocument &document, const LayerId &id)
{
    const auto index = layerIndexById(document, id);
    return index ? &document.layers[*index] : nullptr;
}

const DraftingLayer *findLayer(const DraftingDocument &document, const LayerId &id)
{
    const auto index = layerIndexById(document, id);
    return index ? &document.layers[*index] : nullptr;
}

bool containsObject(const DraftingDocument &document, const DraftingObjectId &id)
{
    return objectIndexById(document, id).has_value();
}

bool containsLayer(const DraftingDocument &document, const LayerId &id)
{
    return layerIndexById(document, id).has_value();
}

} // namespace edi::drafting
