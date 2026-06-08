#pragma once

#include "drafting/DraftingTypes.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace edi::drafting {

struct DraftingObject {
    DraftingObjectId id;
    DraftingShapeKind kind = DraftingShapeKind::Point;
    DraftingGeometry geometry = PointGeometry{};
    StrokeStyle stroke;
    FillStyle fill;
    StyleId styleId;
    Transform2D transform;
    Bounds2D bounds;
    LayerId layerId = "default";
    ObjectMetadata metadata;
    bool locked = false;
    bool visible = true;
};

struct DraftingLayer {
    LayerId id = "default";
    std::string name = "Default";
    int order = 0;
    bool visible = true;
    bool locked = false;
};

struct DraftingDocument {
    DraftingDocumentId id;
    std::string title;
    std::vector<DraftingLayer> layers;
    std::vector<DraftingObject> objects;
    std::vector<DraftingObjectId> selectedObjectIds;
    std::optional<DraftingObjectId> activeObjectId;
    std::uint64_t revision = 0;
};

struct DraftingObjectBuildResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingObject object;

    static DraftingObjectBuildResult accepted(DraftingObject object);
    static DraftingObjectBuildResult rejected(DraftingResultCode code, std::string message);
};

DraftingDocument makeDraftingDocument(DraftingDocumentId id, std::string title = {});
DraftingObject makeDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry);
DraftingObjectBuildResult buildDraftingObject(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry);
DraftingLayer makeDraftingLayer(LayerId id, std::string name, int order = 0);
DraftingLayer makeDefaultLayer();
std::optional<std::size_t> objectIndexById(const DraftingDocument &document, const DraftingObjectId &id);
DraftingObject *findObject(DraftingDocument &document, const DraftingObjectId &id);
const DraftingObject *findObject(const DraftingDocument &document, const DraftingObjectId &id);
std::optional<std::size_t> layerIndexById(const DraftingDocument &document, const LayerId &id);
DraftingLayer *findLayer(DraftingDocument &document, const LayerId &id);
const DraftingLayer *findLayer(const DraftingDocument &document, const LayerId &id);
bool containsObject(const DraftingDocument &document, const DraftingObjectId &id);
bool containsLayer(const DraftingDocument &document, const LayerId &id);

} // namespace edi::drafting
