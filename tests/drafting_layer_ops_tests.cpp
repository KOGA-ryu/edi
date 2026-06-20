#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"

using namespace edi::drafting;

int main()
{
    DraftingDocument document = makeDraftingDocument("layer_ops_doc");
    EDI_CHECK(nextDraftingLayerId(document) == "layer_2");
    EDI_CHECK(nextDraftingLayerName(document) == "Layer 2");

    EDI_CHECK(addLayer(document, makeDraftingLayer("layer_2", "Existing", 1)).ok);
    EDI_CHECK(nextDraftingLayerId(document) == "layer_3");
    EDI_CHECK(nextDraftingLayerName(document) == "Layer 3");

    auto creation = planCreateDraftingLayer(document);
    EDI_CHECK(creation.ok);
    EDI_CHECK(creation.layer.id == "layer_3");
    EDI_CHECK(creation.layer.name == "Layer 3");
    EDI_CHECK(creation.layer.order == 2);
    EDI_CHECK(creation.makeActive);
    DraftingLayer flagLayer = makeDraftingLayer("flag_layer", "Flag Layer", 4);
    flagLayer.locked = false;
    flagLayer.visible = true;
    auto lockPlan = planLayerLockedUpdate(flagLayer, true);
    EDI_CHECK(lockPlan.ok);
    EDI_CHECK(lockPlan.layerId == "flag_layer");
    EDI_CHECK(lockPlan.locked);
    EDI_CHECK(lockPlan.visible);
    flagLayer.locked = true;
    flagLayer.visible = false;
    auto visiblePlan = planLayerVisibleUpdate(flagLayer, true);
    EDI_CHECK(visiblePlan.ok);
    EDI_CHECK(visiblePlan.layerId == "flag_layer");
    EDI_CHECK(visiblePlan.locked);
    EDI_CHECK(visiblePlan.visible);
    flagLayer.id.clear();
    EDI_CHECK(!planLayerLockedUpdate(flagLayer, true).ok);
    EDI_CHECK(!planLayerVisibleUpdate(flagLayer, true).ok);

    auto object = buildDraftingObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.4, 0.4}});
    EDI_CHECK(object.ok);
    EDI_CHECK(addObject(document, object.object).ok);
    EDI_CHECK(!draftingObjectLayerLocked(document, document.objects.back()));
    EDI_CHECK(draftingObjectEffectivelyVisible(document, document.objects.back()));
    EDI_CHECK(draftingObjectEffectivelyEditable(document, document.objects.back()));
    EDI_CHECK(draftingObjectUsableAsBoundsSource(document, document.objects.back()));

    document.layers.front().locked = true;
    EDI_CHECK(draftingObjectLayerLocked(document, document.objects.back()));
    EDI_CHECK(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    EDI_CHECK(!draftingObjectUsableAsBoundsSource(document, document.objects.back()));
    document.layers.front().locked = false;

    document.layers.front().visible = false;
    EDI_CHECK(!draftingObjectEffectivelyVisible(document, document.objects.back()));
    EDI_CHECK(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    document.layers.front().visible = true;

    document.objects.back().visible = false;
    EDI_CHECK(!draftingObjectEffectivelyVisible(document, document.objects.back()));
    EDI_CHECK(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    document.objects.back().visible = true;
    document.objects.back().locked = true;
    EDI_CHECK(!draftingObjectEffectivelyEditable(document, document.objects.back()));
    document.objects.back().locked = false;

    auto guideObject = buildDraftingObject("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.2});
    EDI_CHECK(guideObject.ok);
    EDI_CHECK(addObject(document, guideObject.object).ok);
    EDI_CHECK(draftingObjectEffectivelyEditable(document, document.objects.back()));
    EDI_CHECK(!draftingObjectUsableAsBoundsSource(document, document.objects.back()));
    EDI_CHECK(draftingLayerAcceptsNewObjects(document, "default"));
    EDI_CHECK(activeDraftingLayerAcceptsNewObjects(document));
    document.layers.front().locked = true;
    EDI_CHECK(!draftingLayerAcceptsNewObjects(document, "default"));
    EDI_CHECK(!activeDraftingLayerAcceptsNewObjects(document));
    document.layers.front().locked = false;
    EDI_CHECK(!draftingLayerAcceptsNewObjects(document, "missing_layer"));
    document.activeLayerId = "missing_layer";
    EDI_CHECK(!activeDraftingLayerAcceptsNewObjects(document));
    document.activeLayerId = "default";

    LayerPlotStyle plot;
    plot.plotEnabled = false;
    auto blue = layerPlotStyleForPenPreset(plot, "pen_blue");
    EDI_CHECK(!blue.plotEnabled);
    EDI_CHECK(blue.penId == "pen_blue");
    EDI_CHECK(blue.strokeColor == "#75c7ff");

    auto red = layerPlotStyleForPenPreset(plot, "pen_red");
    EDI_CHECK(red.penId == "pen_red");
    EDI_CHECK(red.strokeColor == "#d98b8b");

    auto fallbackPen = layerPlotStyleForPenPreset(plot, "missing");
    EDI_CHECK(fallbackPen.penId == "pen_black");
    EDI_CHECK(fallbackPen.strokeColor == "#d7dde8");

    auto fine = layerPlotStyleForWidthPreset(plot, "fine");
    EDI_CHECK(fine.strokeWidth == 1.0);
    auto bold = layerPlotStyleForWidthPreset(plot, "bold");
    EDI_CHECK(bold.strokeWidth == 3.0);
    auto normal = layerPlotStyleForWidthPreset(plot, "normal");
    EDI_CHECK(normal.strokeWidth == 2.0);

    auto up = layerMoveDeltaFromDirection("up");
    auto down = layerMoveDeltaFromDirection("down");
    auto missing = layerMoveDeltaFromDirection("sideways");
    EDI_CHECK(up && *up == 1);
    EDI_CHECK(down && *down == -1);
    EDI_CHECK(!missing);

    return 0;
}
