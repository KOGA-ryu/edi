#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>

namespace edi::drafting {

struct DraftingLayerCreationPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingLayer layer;
    bool makeActive = true;

    static DraftingLayerCreationPlan accepted(DraftingLayer layer, bool makeActive = true);
    static DraftingLayerCreationPlan rejected(DraftingResultCode code, std::string message);
};

struct DraftingLayerFlagsPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    LayerId layerId;
    bool locked = false;
    bool visible = true;

    static DraftingLayerFlagsPlan accepted(LayerId layerId, bool locked, bool visible);
    static DraftingLayerFlagsPlan rejected(DraftingResultCode code, std::string message);
};

LayerId nextDraftingLayerId(const DraftingDocument &document);
std::string nextDraftingLayerName(const DraftingDocument &document);
DraftingLayerCreationPlan planCreateDraftingLayer(const DraftingDocument &document);
DraftingLayerFlagsPlan planLayerLockedUpdate(const DraftingLayer &layer, bool locked);
DraftingLayerFlagsPlan planLayerVisibleUpdate(const DraftingLayer &layer, bool visible);
bool draftingObjectLayerLocked(const DraftingDocument &document, const DraftingObject &object);
bool draftingObjectEffectivelyVisible(const DraftingDocument &document, const DraftingObject &object);
bool draftingObjectEffectivelyEditable(const DraftingDocument &document, const DraftingObject &object);
bool draftingObjectUsableAsBoundsSource(const DraftingDocument &document, const DraftingObject &object);
bool draftingLayerAcceptsNewObjects(const DraftingDocument &document, const LayerId &layerId);
bool activeDraftingLayerAcceptsNewObjects(const DraftingDocument &document);
LayerPlotStyle layerPlotStyleForPenPreset(LayerPlotStyle plot, const std::string &presetId);
LayerPlotStyle layerPlotStyleForWidthPreset(LayerPlotStyle plot, const std::string &presetId);
std::optional<int> layerMoveDeltaFromDirection(const std::string &direction);


// The stroke the painter and the plotters should use for `object`: the
// object's own values win where SET (non-empty color, positive width); the
// layer's plot style fills the rest. lineStyle and opacity are object-only
// axes and pass through. Pure.
StrokeStyle effectiveObjectStroke(const DraftingObject &object, const DraftingLayer &layer);

// The physical-pen mapping for plotter outputs: a resolved stroke color
// that exactly matches a pen preset's color selects that pen; any other
// (art) color keeps the layer's pen — a plotter cannot mix ink, so screen
// color richer than the pen set degrades to the layer's physical choice.
std::string penIdForStrokeColor(const std::string &color, const std::string &layerPenId);

} // namespace edi::drafting
