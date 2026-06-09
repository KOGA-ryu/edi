#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

struct DraftingStoreResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingStoreResult accepted();
    static DraftingStoreResult rejected(DraftingResultCode code, std::string message);
};

DraftingStoreResult addObject(DraftingDocument &document, DraftingObject object);
DraftingStoreResult removeObject(DraftingDocument &document, const DraftingObjectId &id);
DraftingStoreResult updateObjectGeometry(DraftingDocument &document, const DraftingObjectId &id, DraftingGeometry geometry);
DraftingStoreResult updateObjectMetadata(DraftingDocument &document, const DraftingObjectId &id, ObjectMetadata metadata);
DraftingStoreResult updateObjectFlags(DraftingDocument &document, const DraftingObjectId &id, bool locked, bool visible);
DraftingStoreResult moveObjectToLayer(DraftingDocument &document, const DraftingObjectId &objectId, const LayerId &layerId);
DraftingStoreResult addLayer(DraftingDocument &document, DraftingLayer layer, bool makeActive = false);
DraftingStoreResult renameLayer(DraftingDocument &document, const LayerId &id, std::string name);
DraftingStoreResult setActiveLayer(DraftingDocument &document, const LayerId &id);
DraftingStoreResult moveLayer(DraftingDocument &document, const LayerId &id, int delta);
DraftingStoreResult updateLayerFlags(DraftingDocument &document, const LayerId &id, bool locked, bool visible);
DraftingStoreResult updateLayerPlotStyle(DraftingDocument &document, const LayerId &id, LayerPlotStyle plot);
DraftingStoreResult moveObject(DraftingDocument &document, const DraftingObjectId &id, double dx, double dy);

} // namespace edi::drafting
