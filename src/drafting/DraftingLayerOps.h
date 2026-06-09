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

LayerId nextDraftingLayerId(const DraftingDocument &document);
std::string nextDraftingLayerName(const DraftingDocument &document);
DraftingLayerCreationPlan planCreateDraftingLayer(const DraftingDocument &document);
bool draftingObjectLayerLocked(const DraftingDocument &document, const DraftingObject &object);
bool draftingObjectEffectivelyVisible(const DraftingDocument &document, const DraftingObject &object);
bool draftingObjectEffectivelyEditable(const DraftingDocument &document, const DraftingObject &object);
bool draftingObjectUsableAsBoundsSource(const DraftingDocument &document, const DraftingObject &object);
LayerPlotStyle layerPlotStyleForPenPreset(LayerPlotStyle plot, const std::string &presetId);
LayerPlotStyle layerPlotStyleForWidthPreset(LayerPlotStyle plot, const std::string &presetId);
std::optional<int> layerMoveDeltaFromDirection(const std::string &direction);

} // namespace edi::drafting
