#include "drafting/DraftingDocument.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <tuple>
#include <utility>

namespace edi::drafting {

namespace {

constexpr std::size_t kMetadataShortTextLimit = 128;
constexpr std::size_t kMetadataMeasurementNoteLimit = 512;

bool isPrintableAscii(std::string_view value)
{
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch >= 0x20 && ch <= 0x7e;
    });
}

DraftingMetadataValidationResult validateMetadataText(
    std::string_view fieldName,
    std::string_view value,
    std::size_t limit)
{
    if (value.size() > limit) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            std::string(fieldName) + " is too long");
    }
    if (!isPrintableAscii(value)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            std::string(fieldName) + " must be printable ASCII");
    }
    return DraftingMetadataValidationResult::accepted();
}

bool isDigitAt(std::string_view value, std::size_t index)
{
    return index < value.size() && std::isdigit(static_cast<unsigned char>(value[index])) != 0;
}

bool hasCharAt(std::string_view value, std::size_t index, char expected)
{
    return index < value.size() && value[index] == expected;
}

bool isUtcTimestampShape(std::string_view value)
{
    if (value.empty()) {
        return true;
    }
    if (value.size() != 20) {
        return false;
    }

    return isDigitAt(value, 0)
        && isDigitAt(value, 1)
        && isDigitAt(value, 2)
        && isDigitAt(value, 3)
        && hasCharAt(value, 4, '-')
        && isDigitAt(value, 5)
        && isDigitAt(value, 6)
        && hasCharAt(value, 7, '-')
        && isDigitAt(value, 8)
        && isDigitAt(value, 9)
        && hasCharAt(value, 10, 'T')
        && isDigitAt(value, 11)
        && isDigitAt(value, 12)
        && hasCharAt(value, 13, ':')
        && isDigitAt(value, 14)
        && isDigitAt(value, 15)
        && hasCharAt(value, 16, ':')
        && isDigitAt(value, 17)
        && isDigitAt(value, 18)
        && hasCharAt(value, 19, 'Z');
}

} // namespace

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

DraftingMetadataValidationResult DraftingMetadataValidationResult::accepted()
{
    return {true, DraftingResultCode::None, {}};
}

DraftingMetadataValidationResult DraftingMetadataValidationResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message)};
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

DraftingMetadataValidationResult validateObjectMetadata(const ObjectMetadata &metadata)
{
    if (metadata.schemaVersion == 0) {
        return DraftingMetadataValidationResult::rejected(DraftingResultCode::InvalidMetadata, "metadata schema version is required");
    }

    for (const auto &[fieldName, value, limit] : {
             std::tuple<std::string_view, std::string_view, std::size_t>{"author", metadata.author, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"source", metadata.source, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"tool provenance", metadata.toolProvenance, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"measurement note", metadata.measurementNote, kMetadataMeasurementNoteLimit},
         }) {
        auto textValidation = validateMetadataText(fieldName, value, limit);
        if (!textValidation.ok) {
            return textValidation;
        }
    }

    auto createdAtTextValidation = validateMetadataText("created at", metadata.createdAt, kMetadataShortTextLimit);
    if (!createdAtTextValidation.ok) {
        return createdAtTextValidation;
    }
    if (!isUtcTimestampShape(metadata.createdAt)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "created at must use YYYY-MM-DDTHH:MM:SSZ");
    }

    return DraftingMetadataValidationResult::accepted();
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
