#include "drafting/DraftingLayerOps.h"

#include "drafting/DraftingGeometry.h"

#include <utility>

namespace edi::drafting {

DraftingLayerCreationPlan DraftingLayerCreationPlan::accepted(DraftingLayer layer, bool makeActive)
{
    DraftingLayerCreationPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.layer = std::move(layer);
    result.makeActive = makeActive;
    return result;
}

DraftingLayerCreationPlan DraftingLayerCreationPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingLayerCreationPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

LayerId nextDraftingLayerId(const DraftingDocument &document)
{
    int serial = static_cast<int>(document.layers.size()) + 1;
    LayerId id = "layer_" + std::to_string(serial);
    while (containsLayer(document, id)) {
        ++serial;
        id = "layer_" + std::to_string(serial);
    }
    return id;
}

std::string nextDraftingLayerName(const DraftingDocument &document)
{
    return "Layer " + std::to_string(document.layers.size() + 1);
}

DraftingLayerCreationPlan planCreateDraftingLayer(const DraftingDocument &document)
{
    DraftingLayer layer = makeDraftingLayer(
        nextDraftingLayerId(document),
        nextDraftingLayerName(document),
        static_cast<int>(document.layers.size()));
    if (!isValidLayerId(layer.id)) {
        return DraftingLayerCreationPlan::rejected(DraftingResultCode::LayerNotFound, "layer id is invalid");
    }
    if (!isValidLayerName(layer.name)) {
        return DraftingLayerCreationPlan::rejected(DraftingResultCode::InvalidSelectionTarget, "layer name is invalid");
    }
    return DraftingLayerCreationPlan::accepted(std::move(layer), true);
}

bool draftingObjectLayerLocked(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return layer != nullptr && layer->locked;
}

bool draftingObjectEffectivelyVisible(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return object.visible && layer != nullptr && layer->visible;
}

bool draftingObjectEffectivelyEditable(const DraftingDocument &document, const DraftingObject &object)
{
    return !object.locked
        && !draftingObjectLayerLocked(document, object)
        && draftingObjectEffectivelyVisible(document, object);
}

bool draftingObjectUsableAsBoundsSource(const DraftingDocument &document, const DraftingObject &object)
{
    return draftingObjectEffectivelyEditable(document, object)
        && object.kind != DraftingShapeKind::Guide
        && object.kind != DraftingShapeKind::ConstructionLine
        && object.kind != DraftingShapeKind::Dimension
        && isFinite(object.bounds);
}

LayerPlotStyle layerPlotStyleForPenPreset(LayerPlotStyle plot, const std::string &presetId)
{
    if (presetId == "pen_blue") {
        plot.penId = "pen_blue";
        plot.strokeColor = "#75c7ff";
    } else if (presetId == "pen_red") {
        plot.penId = "pen_red";
        plot.strokeColor = "#d98b8b";
    } else {
        plot.penId = "pen_black";
        plot.strokeColor = "#d7dde8";
    }
    return plot;
}

LayerPlotStyle layerPlotStyleForWidthPreset(LayerPlotStyle plot, const std::string &presetId)
{
    if (presetId == "fine") {
        plot.strokeWidth = 1.0;
    } else if (presetId == "bold") {
        plot.strokeWidth = 3.0;
    } else {
        plot.strokeWidth = 2.0;
    }
    return plot;
}

std::optional<int> layerMoveDeltaFromDirection(const std::string &direction)
{
    if (direction == "up") {
        return 1;
    }
    if (direction == "down") {
        return -1;
    }
    return std::nullopt;
}

} // namespace edi::drafting
