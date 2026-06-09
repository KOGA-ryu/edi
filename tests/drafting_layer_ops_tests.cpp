#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingStore.h"

#include <cassert>

using namespace edi::drafting;

int main()
{
    DraftingDocument document = makeDraftingDocument("layer_ops_doc");
    assert(nextDraftingLayerId(document) == "layer_2");
    assert(nextDraftingLayerName(document) == "Layer 2");

    assert(addLayer(document, makeDraftingLayer("layer_2", "Existing", 1)).ok);
    assert(nextDraftingLayerId(document) == "layer_3");
    assert(nextDraftingLayerName(document) == "Layer 3");

    auto creation = planCreateDraftingLayer(document);
    assert(creation.ok);
    assert(creation.layer.id == "layer_3");
    assert(creation.layer.name == "Layer 3");
    assert(creation.layer.order == 2);
    assert(creation.makeActive);

    auto object = buildDraftingObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.4, 0.4}});
    assert(object.ok);
    assert(addObject(document, object.object).ok);
    assert(!draftingObjectLayerLocked(document, document.objects.back()));
    assert(draftingObjectEffectivelyVisible(document, document.objects.back()));
    assert(draftingObjectEffectivelyEditable(document, document.objects.back()));
    assert(draftingObjectUsableAsBoundsSource(document, document.objects.back()));

    document.layers.front().locked = true;
    assert(draftingObjectLayerLocked(document, document.objects.back()));
    assert(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    assert(!draftingObjectUsableAsBoundsSource(document, document.objects.back()));
    document.layers.front().locked = false;

    document.layers.front().visible = false;
    assert(!draftingObjectEffectivelyVisible(document, document.objects.back()));
    assert(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    document.layers.front().visible = true;

    document.objects.back().visible = false;
    assert(!draftingObjectEffectivelyVisible(document, document.objects.back()));
    assert(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    document.objects.back().visible = true;
    document.objects.back().locked = true;
    assert(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    document.objects.back().locked = false;

    auto guideObject = buildDraftingObject("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.2});
    assert(guideObject.ok);
    assert(addObject(document, guideObject.object).ok);
    assert(draftingObjectEffectivelyEditable(document, document.objects.back()));
    assert(!draftingObjectUsableAsBoundsSource(document, document.objects.back()));
    assert(draftingLayerAcceptsNewObjects(document, "default"));
    assert(activeDraftingLayerAcceptsNewObjects(document));
    document.layers.front().locked = true;
    assert(!draftingLayerAcceptsNewObjects(document, "default"));
    assert(!activeDraftingLayerAcceptsNewObjects(document));
    document.layers.front().locked = false;
    assert(!draftingLayerAcceptsNewObjects(document, "missing_layer"));
    document.activeLayerId = "missing_layer";
    assert(!activeDraftingLayerAcceptsNewObjects(document));
    document.activeLayerId = "default";

    LayerPlotStyle plot;
    plot.plotEnabled = false;
    auto blue = layerPlotStyleForPenPreset(plot, "pen_blue");
    assert(!blue.plotEnabled);
    assert(blue.penId == "pen_blue");
    assert(blue.strokeColor == "#75c7ff");

    auto red = layerPlotStyleForPenPreset(plot, "pen_red");
    assert(red.penId == "pen_red");
    assert(red.strokeColor == "#d98b8b");

    auto fallbackPen = layerPlotStyleForPenPreset(plot, "missing");
    assert(fallbackPen.penId == "pen_black");
    assert(fallbackPen.strokeColor == "#d7dde8");

    auto fine = layerPlotStyleForWidthPreset(plot, "fine");
    assert(fine.strokeWidth == 1.0);
    auto bold = layerPlotStyleForWidthPreset(plot, "bold");
    assert(bold.strokeWidth == 3.0);
    auto normal = layerPlotStyleForWidthPreset(plot, "normal");
    assert(normal.strokeWidth == 2.0);

    auto up = layerMoveDeltaFromDirection("up");
    auto down = layerMoveDeltaFromDirection("down");
    auto missing = layerMoveDeltaFromDirection("sideways");
    assert(up && *up == 1);
    assert(down && *down == -1);
    assert(!missing);

    return 0;
}
