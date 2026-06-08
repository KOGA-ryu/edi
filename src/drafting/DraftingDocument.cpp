#include "drafting/DraftingDocument.h"

#include <algorithm>
#include <utility>

namespace edi::drafting {

DraftingObject makeDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    DraftingObject object;
    object.id = std::move(id);
    object.kind = kind;
    object.geometry = std::move(geometry);
    return object;
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
