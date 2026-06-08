#include "drafting/DraftingDocument.h"

#include <algorithm>

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

DraftingObject *findObject(DraftingDocument &document, const DraftingObjectId &id)
{
    auto it = std::find_if(document.objects.begin(), document.objects.end(), [&](const DraftingObject &object) {
        return object.id == id;
    });
    return it == document.objects.end() ? nullptr : &*it;
}

const DraftingObject *findObject(const DraftingDocument &document, const DraftingObjectId &id)
{
    auto it = std::find_if(document.objects.begin(), document.objects.end(), [&](const DraftingObject &object) {
        return object.id == id;
    });
    return it == document.objects.end() ? nullptr : &*it;
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
    return findObject(document, id) != nullptr;
}

} // namespace edi::drafting
