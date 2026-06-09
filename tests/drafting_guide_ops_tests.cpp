#include "drafting/DraftingGuideOps.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

using namespace edi::drafting;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    assert(sameGuide({GuideOrientation::Vertical, 0.5}, {GuideOrientation::Vertical, 0.5000005}));
    assert(!sameGuide({GuideOrientation::Vertical, 0.5}, {GuideOrientation::Horizontal, 0.5}));
    assert(isGuideObject(object("guide_kind", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.1})));
    assert(!isGuideObject(object("point_kind", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}})));

    DraftingDocument document = makeDraftingDocument("guide_ops_doc");
    assert(addLayer(document, makeDraftingLayer("hidden", "Hidden", 2)).ok);
    assert(addObject(document, object("guide_v", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25})).ok);
    assert(addObject(document, object("guide_h", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.75})).ok);
    DraftingObject hiddenGuide = object("hidden_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.9});
    hiddenGuide.layerId = "hidden";
    assert(addObject(document, hiddenGuide).ok);
    assert(updateLayerFlags(document, "hidden", false, false).ok);

    auto existing = existingGuideId(document, GuideGeometry{GuideOrientation::Vertical, 0.25});
    assert(existing);
    assert(*existing == "guide_v");
    assert(!existingGuideId(document, GuideGeometry{GuideOrientation::Vertical, 0.5}));

    auto nearestVertical = nearestVisibleGuidePosition(document, GuideOrientation::Vertical, 0.3);
    assert(nearestVertical);
    assert(near(*nearestVertical, 0.25));
    auto nearestHorizontal = nearestVisibleGuidePosition(document, GuideOrientation::Horizontal, 0.6);
    assert(nearestHorizontal);
    assert(near(*nearestHorizontal, 0.75));
    assert(!nearestVisibleGuidePosition(document, GuideOrientation::Vertical, std::numeric_limits<double>::infinity()));

    Bounds2D drawable{0.1, 0.2, 0.6, 0.4};
    auto origin = moveGuideToDrawable({GuideOrientation::Vertical, 0.5}, drawable, DraftingGuideDrawablePlacement::Origin);
    assert(origin.ok);
    assert(near(origin.geometry.position, 0.1));
    auto center = moveGuideToDrawable({GuideOrientation::Horizontal, 0.5}, drawable, DraftingGuideDrawablePlacement::Center);
    assert(center.ok);
    assert(near(center.geometry.position, 0.4));
    auto max = moveGuideToDrawable({GuideOrientation::Vertical, 0.5}, drawable, DraftingGuideDrawablePlacement::Max);
    assert(max.ok);
    assert(near(max.geometry.position, 0.7));

    auto positiveOffset = offsetGuide({GuideOrientation::Vertical, 0.5}, "positive", 0.05, 0.1, 2.0);
    assert(positiveOffset.ok);
    assert(near(positiveOffset.geometry.position, 0.6));
    auto negativeOffset = offsetGuide({GuideOrientation::Horizontal, 0.5}, "negative", 0.05, 0.1, 0.5);
    assert(negativeOffset.ok);
    assert(near(negativeOffset.geometry.position, 0.45));
    assert(!offsetGuide({GuideOrientation::Horizontal, 0.5}, "sideways", 0.05, 0.1, 1.0).ok);

    Bounds2D bounds{0.2, 0.3, 0.4, 0.5};
    auto left = guideFromBoundsPlacement(bounds, "left");
    assert(left.ok);
    assert(left.geometry.orientation == GuideOrientation::Vertical);
    assert(near(left.geometry.position, 0.2));
    auto bottom = guideFromBoundsPlacement(bounds, "bottom");
    assert(bottom.ok);
    assert(bottom.geometry.orientation == GuideOrientation::Horizontal);
    assert(near(bottom.geometry.position, 0.8));
    assert(!guideFromBoundsPlacement(bounds, "missing").ok);

    auto rightOffset = offsetGuideFromBoundsPlacement(bounds, "right", 0.05, 0.1);
    assert(rightOffset.ok);
    assert(rightOffset.geometry.orientation == GuideOrientation::Vertical);
    assert(near(rightOffset.geometry.position, 0.65));
    auto centerYMinus = offsetGuideFromBoundsPlacement(bounds, "center_y_minus", 0.05, 0.1);
    assert(centerYMinus.ok);
    assert(centerYMinus.geometry.orientation == GuideOrientation::Horizontal);
    assert(near(centerYMinus.geometry.position, 0.45));
    assert(!offsetGuideFromBoundsPlacement(bounds, "missing", 0.05, 0.1).ok);

    auto drawableBoundsPreset = guidePresetForDrawable("drawable_bounds", drawable);
    assert(drawableBoundsPreset.ok);
    assert(drawableBoundsPreset.guides.size() == 4);
    assert(drawableBoundsPreset.guides[0].geometry.orientation == GuideOrientation::Vertical);
    assert(near(drawableBoundsPreset.guides[0].geometry.position, 0.1));
    assert(drawableBoundsPreset.guides[0].label == "drawable left");
    assert(drawableBoundsPreset.guides[0].color == "#f6c65b");
    GuideVisualMetadata drawableLeftVisual = guidePresetVisualMetadata(drawableBoundsPreset.guides[0]);
    assert(drawableLeftVisual.label == "drawable left");
    assert(drawableLeftVisual.color == "#f6c65b");
    assert(drawableLeftVisual.dashStyle == "dash");
    assert(drawableLeftVisual.showLabel);
    assert(drawableBoundsPreset.guides[3].geometry.orientation == GuideOrientation::Horizontal);
    assert(near(drawableBoundsPreset.guides[3].geometry.position, 0.6));

    auto centerlinePreset = guidePresetForDrawable("drawable_centerlines", drawable);
    assert(centerlinePreset.ok);
    assert(centerlinePreset.guides.size() == 2);
    assert(near(centerlinePreset.guides[0].geometry.position, 0.4));
    assert(near(centerlinePreset.guides[1].geometry.position, 0.4));

    auto thirdsPreset = guidePresetForDrawable("thirds", drawable);
    assert(thirdsPreset.ok);
    assert(thirdsPreset.guides.size() == 4);
    assert(near(thirdsPreset.guides[0].geometry.position, 0.3));
    assert(near(thirdsPreset.guides[2].geometry.position, 0.3333333333333333));

    auto quartersPreset = guidePresetForDrawable("quarters", drawable);
    assert(quartersPreset.ok);
    assert(quartersPreset.guides.size() == 6);
    assert(near(quartersPreset.guides[2].geometry.position, 0.55));
    assert(near(quartersPreset.guides[5].geometry.position, 0.5));

    auto marginPreset = guidePresetForDrawable("margin_safe", drawable);
    assert(marginPreset.ok);
    assert(marginPreset.guides.size() == 6);
    assert(marginPreset.guides[0].label == "safe left");
    assert(marginPreset.guides[0].color == "#d98b8b");
    assert(!guidePresetForDrawable("missing", drawable).ok);
    assert(!guidePresetForDrawable("drawable_bounds", Bounds2D{0.0, 0.0, 0.0, 1.0}).ok);

    auto builtGuide = buildDraftingGuideObject(
        "built_guide",
        GuideGeometry{GuideOrientation::Vertical, 0.42},
        "layout",
        "unit_test_guide",
        "source_preset");
    assert(builtGuide.ok);
    assert(builtGuide.object.id == "built_guide");
    assert(builtGuide.object.kind == DraftingShapeKind::Guide);
    assert(builtGuide.object.layerId == "layout");
    assert(builtGuide.object.metadata.toolProvenance == "unit_test_guide");
    assert(builtGuide.object.metadata.source == "source_preset");
    assert(builtGuide.object.metadata.guideVisual.color == "#83aeca");

    GuideVisualMetadata guideVisual;
    guideVisual.label = "safe edge";
    guideVisual.color = "#54d2c6";
    guideVisual.dashStyle = "dot";
    guideVisual.showLabel = false;
    auto visualGuide = buildDraftingGuideObject(
        "visual_guide",
        GuideGeometry{GuideOrientation::Horizontal, 0.24},
        "layout",
        "visual_test",
        {},
        guideVisual);
    assert(visualGuide.ok);
    assert(visualGuide.object.metadata.guideVisual.label == "safe edge");
    assert(visualGuide.object.metadata.guideVisual.color == "#54d2c6");
    assert(visualGuide.object.metadata.guideVisual.dashStyle == "dot");
    assert(!visualGuide.object.metadata.guideVisual.showLabel);

    guideVisual.color = "bad";
    auto badVisualGuide = buildDraftingGuideObject(
        "bad_visual_guide",
        GuideGeometry{GuideOrientation::Horizontal, 0.24},
        "layout",
        "visual_test",
        {},
        guideVisual);
    assert(!badVisualGuide.ok);
    assert(badVisualGuide.code == DraftingResultCode::InvalidMetadata);

    Bounds2D alignBounds{0.2, 0.3, 0.2, 0.4};
    auto alignLeft = alignBoundsToNearestGuide(document, alignBounds, "left");
    assert(alignLeft.ok);
    assert(alignLeft.orientation == GuideOrientation::Vertical);
    assert(near(alignLeft.target, 0.2));
    assert(near(alignLeft.guidePosition, 0.25));
    assert(near(alignLeft.dx, 0.05));
    assert(near(alignLeft.dy, 0.0));

    auto alignRight = alignBoundsToNearestGuide(document, alignBounds, "right");
    assert(alignRight.ok);
    assert(near(alignRight.target, 0.4));
    assert(near(alignRight.dx, -0.15));

    auto alignCenterX = alignBoundsToNearestGuide(document, alignBounds, "center_x");
    assert(alignCenterX.ok);
    assert(near(alignCenterX.target, 0.3));
    assert(near(alignCenterX.dx, -0.05));

    auto alignTop = alignBoundsToNearestGuide(document, alignBounds, "top");
    assert(alignTop.ok);
    assert(alignTop.orientation == GuideOrientation::Horizontal);
    assert(near(alignTop.target, 0.3));
    assert(near(alignTop.guidePosition, 0.75));
    assert(near(alignTop.dx, 0.0));
    assert(near(alignTop.dy, 0.45));

    auto alignBottom = alignBoundsToNearestGuide(document, alignBounds, "bottom");
    assert(alignBottom.ok);
    assert(near(alignBottom.target, 0.7));
    assert(near(alignBottom.dy, 0.05));

    auto alignCenterY = alignBoundsToNearestGuide(document, alignBounds, "center_y");
    assert(alignCenterY.ok);
    assert(near(alignCenterY.target, 0.5));
    assert(near(alignCenterY.dy, 0.25));

    assert(!alignBoundsToNearestGuide(document, alignBounds, "missing").ok);
    assert(!alignBoundsToNearestGuide(document, Bounds2D{0.0, 0.0, std::numeric_limits<double>::infinity(), 1.0}, "left").ok);

    DraftingDocument noGuides = makeDraftingDocument("no_guides");
    auto noGuideAlignment = alignBoundsToNearestGuide(noGuides, alignBounds, "left");
    assert(!noGuideAlignment.ok);
    assert(noGuideAlignment.code == DraftingResultCode::ObjectNotFound);

    DraftingSnapSettings guideSnapSettings;
    guideSnapSettings.objectSnapEnabled = true;
    guideSnapSettings.guideEnabled = true;
    guideSnapSettings.objectTolerance = 0.03;

    DraftingObject lineForSnap = object(
        "line_for_snap",
        DraftingShapeKind::Line,
        LineGeometry{{0.1, 0.5}, {0.2, 0.5}});
    std::vector<DraftingGuideMoveSnapAnchor> lineAnchors = guideMoveSnapAnchorsForObject(lineForSnap);
    assert(!lineAnchors.empty());
    assert(lineAnchors[0].rank == 0);
    assert(lineAnchors[0].label == "endpoint");

    auto verticalMoveSnap = guideMoveSnapPlan(document, lineForSnap, {"line_for_snap"}, guideSnapSettings, 0.04, 0.0);
    assert(verticalMoveSnap.ok);
    assert(!verticalMoveSnap.intersection);
    assert(verticalMoveSnap.anchorRank == 0);
    assert(verticalMoveSnap.anchorLabel == "endpoint");
    assert(verticalMoveSnap.sourceObjectId == "guide_v");
    assert(near(verticalMoveSnap.intendedAnchor.x, 0.24));
    assert(near(verticalMoveSnap.snappedAnchor.x, 0.25));
    assert(near(verticalMoveSnap.dx, 0.05));
    assert(near(verticalMoveSnap.dy, 0.0));

    auto selectedGuideIgnored = guideMoveSnapPlan(document, lineForSnap, {"line_for_snap", "guide_v"}, guideSnapSettings, 0.04, 0.0);
    assert(!selectedGuideIgnored.ok);

    DraftingObject lineForIntersection = object(
        "line_for_intersection",
        DraftingShapeKind::Line,
        LineGeometry{{0.1, 0.7}, {0.2, 0.7}});
    auto intersectionSnap = guideMoveSnapPlan(document, lineForIntersection, {"line_for_intersection"}, guideSnapSettings, 0.04, 0.04);
    assert(intersectionSnap.ok);
    assert(intersectionSnap.intersection);
    assert(intersectionSnap.sourceObjectId == "guide_v");
    assert(near(intersectionSnap.snappedAnchor.x, 0.25));
    assert(near(intersectionSnap.snappedAnchor.y, 0.75));
    assert(near(intersectionSnap.dx, 0.05));
    assert(near(intersectionSnap.dy, 0.05));

    DraftingSnapSettings guideSnapDisabled = guideSnapSettings;
    guideSnapDisabled.guideEnabled = false;
    assert(!guideMoveSnapPlan(document, lineForSnap, {"line_for_snap"}, guideSnapDisabled, 0.04, 0.0).ok);

    return 0;
}
