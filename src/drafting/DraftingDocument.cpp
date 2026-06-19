#include "drafting/DraftingDocument.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
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

namespace {

// Recover the trailing numeric suffix of an "<prefix>_NNNN" id (0 if none).
int idTrailingSerial(const std::string &id)
{
    std::size_t end = id.size();
    std::size_t begin = end;
    while (begin > 0 && std::isdigit(static_cast<unsigned char>(id[begin - 1]))) {
        --begin;
    }
    if (begin == end) {
        return 0;
    }
    try {
        return std::stoi(id.substr(begin, end - begin));
    } catch (...) {
        return 0;
    }
}

} // namespace

int highestDocumentIdSerial(const DraftingDocument &document)
{
    int highest = 0;
    for (const auto &object : document.objects) {
        highest = std::max(highest, idTrailingSerial(object.id));
    }
    for (const auto &plug : document.plugs) {
        highest = std::max(highest, idTrailingSerial(plug.id));
    }
    for (const auto &connection : document.connections) {
        highest = std::max(highest, idTrailingSerial(connection.id));
    }
    for (const auto &block : document.blocks) {
        highest = std::max(highest, idTrailingSerial(block.id));
    }
    // Phase-1 slice 3c: nodes carry minted ids ("node_NNNN") that share the same
    // monotonic serial space as objects/plugs/blocks. Omitting this scan would let
    // a freshly-opened document with nodes re-mint colliding ids (the C0/C2 lesson:
    // EVERY id-bearing vector must be scanned — missing one is a silent collision).
    for (const auto &node : document.nodes) {
        highest = std::max(highest, idTrailingSerial(node.id));
    }
    return highest;
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

std::optional<Bounds2D> documentObjectsBounds(const DraftingDocument &document)
{
    // Fold every object's cached bounds with the same helper the rest of the core
    // uses (so an empty/degenerate object never widens the box incorrectly). An empty
    // document has no extent — nullopt, NOT a zero rect — so the caller can no-op.
    if (document.objects.empty()) {
        return std::nullopt;
    }
    Bounds2D bounds = document.objects.front().bounds;
    for (std::size_t i = 1; i < document.objects.size(); ++i) {
        bounds = includeBounds(bounds, document.objects[i].bounds);
    }
    return bounds;
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

const DraftingObject *activeObject(const DraftingDocument &document)
{
    if (!document.activeObjectId) {
        return nullptr;
    }
    return findObject(document, *document.activeObjectId);
}

const DraftingObject *activeObjectOfKind(const DraftingDocument &document, DraftingShapeKind kind)
{
    const DraftingObject *object = activeObject(document);
    if (object == nullptr || object->kind != kind) {
        return nullptr;
    }
    return object;
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

std::unordered_set<DraftingObjectId> objectIdSet(const DraftingDocument &document)
{
    std::unordered_set<DraftingObjectId> ids;
    ids.reserve(document.objects.size());
    for (const DraftingObject &object : document.objects) {
        ids.insert(object.id);
    }
    return ids;
}

} // namespace edi::drafting
