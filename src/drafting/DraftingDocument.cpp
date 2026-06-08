#include "drafting/DraftingDocument.h"

#include <algorithm>
#include <utility>

namespace edi::drafting {

DraftingLayer makeDefaultLayer()
{
    return {};
}

DraftingDocument makeDraftingDocument(DraftingDocumentId id, std::string title)
{
    DraftingDocument document;
    document.id = std::move(id);
    document.title = title.empty() ? document.id : std::move(title);
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

DraftingLayer *findLayer(DraftingDocument &document, const LayerId &id)
{
    auto it = std::find_if(document.layers.begin(), document.layers.end(), [&](const DraftingLayer &layer) {
        return layer.id == id;
    });
    return it == document.layers.end() ? nullptr : &*it;
}

const DraftingLayer *findLayer(const DraftingDocument &document, const LayerId &id)
{
    auto it = std::find_if(document.layers.begin(), document.layers.end(), [&](const DraftingLayer &layer) {
        return layer.id == id;
    });
    return it == document.layers.end() ? nullptr : &*it;
}

bool containsObject(const DraftingDocument &document, const DraftingObjectId &id)
{
    return objectIndexById(document, id).has_value();
}

} // namespace edi::drafting
