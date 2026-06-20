#include "drafting/DraftingGuideOps.h"

#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
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
    EDI_CHECK(built.ok);
    return built.object;
}

} // namespace

int main()
{
    EDI_CHECK(sameGuide({GuideOrientation::Vertical, 0.5}, {GuideOrientation::Vertical, 0.5000005}));
    EDI_CHECK(!sameGuide({GuideOrientation::Vertical, 0.5}, {GuideOrientation::Horizontal, 0.5}));
    EDI_CHECK(isGuideObject(object("guide_kind", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.1})));
    EDI_CHECK(!isGuideObject(object("point_kind", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}})));

    DraftingDocument document = makeDraftingDocument("guide_ops_doc");
    EDI_CHECK(addLayer(document, makeDraftingLayer("hidden", "Hidden", 2)).ok);
    EDI_CHECK(addObject(document, object("guide_v", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25})).ok);
    EDI_CHECK(addObject(document, object("guide_h", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.75})).ok);
    DraftingObject hiddenGuide = object("hidden_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.9});
    hiddenGuide.layerId = "hidden";
    EDI_CHECK(addObject(document, hiddenGuide).ok);
    EDI_CHECK(updateLayerFlags(document, "hidden", false, false).ok);

    auto existing = existingGuideId(document, GuideGeometry{GuideOrientation::Vertical, 0.25});
    EDI_CHECK(existing);
    EDI_CHECK(*existing == "guide_v");
    EDI_CHECK(!existingGuideId(document, GuideGeometry{GuideOrientation::Vertical, 0.5}));

    auto nearestVertical = nearestVisibleGuidePosition(document, GuideOrientation::Vertical, 0.3);
    EDI_CHECK(nearestVertical);
    EDI_CHECK(near(*nearestVertical, 0.25));
    auto nearestHorizontal = nearestVisibleGuidePosition(document, GuideOrientation::Horizontal, 0.6);
    EDI_CHECK(nearestHorizontal);
    EDI_CHECK(near(*nearestHorizontal, 0.75));
    EDI_CHECK(!nearestVisibleGuidePosition(document, GuideOrientation::Vertical, std::numeric_limits<double>::infinity()));

    Bounds2D drawable{0.1, 0.2, 0.6, 0.4};
    auto origin = moveGuideToDrawable({GuideOrientation::Vertical, 0.5}, drawable, DraftingGuideDrawablePlacement::Origin);
    EDI_CHECK(origin.ok);
    EDI_CHECK(near(origin.geometry.position, 0.1));
    auto center = moveGuideToDrawable({GuideOrientation::Horizontal, 0.5}, drawable, DraftingGuideDrawablePlacement::Center);
    EDI_CHECK(center.ok);
    EDI_CHECK(near(center.geometry.position, 0.4));
    auto max = moveGuideToDrawable({GuideOrientation::Vertical, 0.5}, drawable, DraftingGuideDrawablePlacement::Max);
    EDI_CHECK(max.ok);
    EDI_CHECK(near(max.geometry.position, 0.7));

    auto positiveOffset = offsetGuide({GuideOrientation::Vertical, 0.5}, "positive", 0.05, 0.1, 2.0);
    EDI_CHECK(positiveOffset.ok);
    EDI_CHECK(near(positiveOffset.geometry.position, 0.6));
    auto negativeOffset = offsetGuide({GuideOrientation::Horizontal, 0.5}, "negative", 0.05, 0.1, 0.5);
    EDI_CHECK(negativeOffset.ok);
    EDI_CHECK(near(negativeOffset.geometry.position, 0.45));
    EDI_CHECK(!offsetGuide({GuideOrientation::Horizontal, 0.5}, "sideways", 0.05, 0.1, 1.0).ok);

    Bounds2D bounds{0.2, 0.3, 0.4, 0.5};
    auto left = guideFromBoundsPlacement(bounds, "left");
    EDI_CHECK(left.ok);
    EDI_CHECK(left.geometry.orientation == GuideOrientation::Vertical);
    EDI_CHECK(near(left.geometry.position, 0.2));
    auto bottom = guideFromBoundsPlacement(bounds, "bottom");
    EDI_CHECK(bottom.ok);
    EDI_CHECK(bottom.geometry.orientation == GuideOrientation::Horizontal);
    EDI_CHECK(near(bottom.geometry.position, 0.8));
    EDI_CHECK(!guideFromBoundsPlacement(bounds, "missing").ok);

    auto rightOffset = offsetGuideFromBoundsPlacement(bounds, "right", 0.05, 0.1);
    EDI_CHECK(rightOffset.ok);
    EDI_CHECK(rightOffset.geometry.orientation == GuideOrientation::Vertical);
    EDI_CHECK(near(rightOffset.geometry.position, 0.65));
    auto centerYMinus = offsetGuideFromBoundsPlacement(bounds, "center_y_minus", 0.05, 0.1);
    EDI_CHECK(centerYMinus.ok);
    EDI_CHECK(centerYMinus.geometry.orientation == GuideOrientation::Horizontal);
    EDI_CHECK(near(centerYMinus.geometry.position, 0.45));
    EDI_CHECK(!offsetGuideFromBoundsPlacement(bounds, "missing", 0.05, 0.1).ok);

    auto drawableBoundsPreset = guidePresetForDrawable("drawable_bounds", drawable);
    EDI_CHECK(drawableBoundsPreset.ok);
    EDI_CHECK(drawableBoundsPreset.guides.size() == 4);
    EDI_CHECK(drawableBoundsPreset.guides[0].geometry.orientation == GuideOrientation::Vertical);
    EDI_CHECK(near(drawableBoundsPreset.guides[0].geometry.position, 0.1));
    EDI_CHECK(drawableBoundsPreset.guides[0].label == "drawable left");
    EDI_CHECK(drawableBoundsPreset.guides[0].color == "#f6c65b");
    GuideVisualMetadata drawableLeftVisual = guidePresetVisualMetadata(drawableBoundsPreset.guides[0]);
    EDI_CHECK(drawableLeftVisual.label == "drawable left");
    EDI_CHECK(drawableLeftVisual.color == "#f6c65b");
    EDI_CHECK(drawableLeftVisual.dashStyle == "dash");
    EDI_CHECK(drawableLeftVisual.showLabel);
    EDI_CHECK(drawableBoundsPreset.guides[3].geometry.orientation == GuideOrientation::Horizontal);
    EDI_CHECK(near(drawableBoundsPreset.guides[3].geometry.position, 0.6));

    auto centerlinePreset = guidePresetForDrawable("drawable_centerlines", drawable);
    EDI_CHECK(centerlinePreset.ok);
    EDI_CHECK(centerlinePreset.guides.size() == 2);
    EDI_CHECK(near(centerlinePreset.guides[0].geometry.position, 0.4));
    EDI_CHECK(near(centerlinePreset.guides[1].geometry.position, 0.4));

    auto thirdsPreset = guidePresetForDrawable("thirds", drawable);
    EDI_CHECK(thirdsPreset.ok);
    EDI_CHECK(thirdsPreset.guides.size() == 4);
    EDI_CHECK(near(thirdsPreset.guides[0].geometry.position, 0.3));
    EDI_CHECK(near(thirdsPreset.guides[2].geometry.position, 0.3333333333333333));

    auto quartersPreset = guidePresetForDrawable("quarters", drawable);
    EDI_CHECK(quartersPreset.ok);
    EDI_CHECK(quartersPreset.guides.size() == 6);
    EDI_CHECK(near(quartersPreset.guides[2].geometry.position, 0.55));
    EDI_CHECK(near(quartersPreset.guides[5].geometry.position, 0.5));

    auto marginPreset = guidePresetForDrawable("margin_safe", drawable);
    EDI_CHECK(marginPreset.ok);
    EDI_CHECK(marginPreset.guides.size() == 6);
    EDI_CHECK(marginPreset.guides[0].label == "safe left");
    EDI_CHECK(marginPreset.guides[0].color == "#d98b8b");
    EDI_CHECK(!guidePresetForDrawable("missing", drawable).ok);
    EDI_CHECK(!guidePresetForDrawable("drawable_bounds", Bounds2D{0.0, 0.0, 0.0, 1.0}).ok);

    auto builtGuide = buildDraftingGuideObject(
        "built_guide",
        GuideGeometry{GuideOrientation::Vertical, 0.42},
        "layout",
        "unit_test_guide",
        "source_preset");
    EDI_CHECK(builtGuide.ok);
    EDI_CHECK(builtGuide.object.id == "built_guide");
    EDI_CHECK(builtGuide.object.kind == DraftingShapeKind::Guide);
    EDI_CHECK(builtGuide.object.layerId == "layout");
    EDI_CHECK(builtGuide.object.metadata.toolProvenance == "unit_test_guide");
    EDI_CHECK(builtGuide.object.metadata.source == "source_preset");
    EDI_CHECK(builtGuide.object.metadata.guideVisual.color == "#83aeca");

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
    EDI_CHECK(visualGuide.ok);
    EDI_CHECK(visualGuide.object.metadata.guideVisual.label == "safe edge");
    EDI_CHECK(visualGuide.object.metadata.guideVisual.color == "#54d2c6");
    EDI_CHECK(visualGuide.object.metadata.guideVisual.dashStyle == "dot");
    EDI_CHECK(!visualGuide.object.metadata.guideVisual.showLabel);

    guideVisual.color = "bad";
    auto badVisualGuide = buildDraftingGuideObject(
        "bad_visual_guide",
        GuideGeometry{GuideOrientation::Horizontal, 0.24},
        "layout",
        "visual_test",
        {},
        guideVisual);
    EDI_CHECK(!badVisualGuide.ok);
    EDI_CHECK(badVisualGuide.code == DraftingResultCode::InvalidMetadata);

    Bounds2D alignBounds{0.2, 0.3, 0.2, 0.4};
    auto alignLeft = alignBoundsToNearestGuide(document, alignBounds, "left");
    EDI_CHECK(alignLeft.ok);
    EDI_CHECK(alignLeft.orientation == GuideOrientation::Vertical);
    EDI_CHECK(near(alignLeft.target, 0.2));
    EDI_CHECK(near(alignLeft.guidePosition, 0.25));
    EDI_CHECK(near(alignLeft.dx, 0.05));
    EDI_CHECK(near(alignLeft.dy, 0.0));

    auto alignRight = alignBoundsToNearestGuide(document, alignBounds, "right");
    EDI_CHECK(alignRight.ok);
    EDI_CHECK(near(alignRight.target, 0.4));
    EDI_CHECK(near(alignRight.dx, -0.15));

    auto alignCenterX = alignBoundsToNearestGuide(document, alignBounds, "center_x");
    EDI_CHECK(alignCenterX.ok);
    EDI_CHECK(near(alignCenterX.target, 0.3));
    EDI_CHECK(near(alignCenterX.dx, -0.05));

    auto alignTop = alignBoundsToNearestGuide(document, alignBounds, "top");
    EDI_CHECK(alignTop.ok);
    EDI_CHECK(alignTop.orientation == GuideOrientation::Horizontal);
    EDI_CHECK(near(alignTop.target, 0.3));
    EDI_CHECK(near(alignTop.guidePosition, 0.75));
    EDI_CHECK(near(alignTop.dx, 0.0));
    EDI_CHECK(near(alignTop.dy, 0.45));

    auto alignBottom = alignBoundsToNearestGuide(document, alignBounds, "bottom");
    EDI_CHECK(alignBottom.ok);
    EDI_CHECK(near(alignBottom.target, 0.7));
    EDI_CHECK(near(alignBottom.dy, 0.05));

    auto alignCenterY = alignBoundsToNearestGuide(document, alignBounds, "center_y");
    EDI_CHECK(alignCenterY.ok);
    EDI_CHECK(near(alignCenterY.target, 0.5));
    EDI_CHECK(near(alignCenterY.dy, 0.25));

    EDI_CHECK(!alignBoundsToNearestGuide(document, alignBounds, "missing").ok);
    EDI_CHECK(!alignBoundsToNearestGuide(document, Bounds2D{0.0, 0.0, std::numeric_limits<double>::infinity(), 1.0}, "left").ok);

    DraftingDocument noGuides = makeDraftingDocument("no_guides");
    auto noGuideAlignment = alignBoundsToNearestGuide(noGuides, alignBounds, "left");
    EDI_CHECK(!noGuideAlignment.ok);
    EDI_CHECK(noGuideAlignment.code == DraftingResultCode::ObjectNotFound);

    DraftingSnapSettings guideSnapSettings;
    guideSnapSettings.objectSnapEnabled = true;
    guideSnapSettings.guideEnabled = true;
    guideSnapSettings.objectTolerance = 0.03;

    DraftingObject lineForSnap = object(
        "line_for_snap",
        DraftingShapeKind::Line,
        LineGeometry{{0.1, 0.5}, {0.2, 0.5}});
    std::vector<DraftingGuideMoveSnapAnchor> lineAnchors = guideMoveSnapAnchorsForObject(lineForSnap);
    EDI_CHECK(!lineAnchors.empty());
    EDI_CHECK(lineAnchors[0].rank == 0);
    EDI_CHECK(lineAnchors[0].label == "endpoint");

    auto verticalMoveSnap = guideMoveSnapPlan(document, lineForSnap, {"line_for_snap"}, guideSnapSettings, 0.04, 0.0);
    EDI_CHECK(verticalMoveSnap.ok);
    EDI_CHECK(!verticalMoveSnap.intersection);
    EDI_CHECK(verticalMoveSnap.anchorRank == 0);
    EDI_CHECK(verticalMoveSnap.anchorLabel == "endpoint");
    EDI_CHECK(verticalMoveSnap.sourceObjectId == "guide_v");
    EDI_CHECK(near(verticalMoveSnap.intendedAnchor.x, 0.24));
    EDI_CHECK(near(verticalMoveSnap.snappedAnchor.x, 0.25));
    EDI_CHECK(near(verticalMoveSnap.dx, 0.05));
    EDI_CHECK(near(verticalMoveSnap.dy, 0.0));

    auto selectedGuideIgnored = guideMoveSnapPlan(document, lineForSnap, {"line_for_snap", "guide_v"}, guideSnapSettings, 0.04, 0.0);
    EDI_CHECK(!selectedGuideIgnored.ok);

    DraftingObject lineForIntersection = object(
        "line_for_intersection",
        DraftingShapeKind::Line,
        LineGeometry{{0.1, 0.7}, {0.2, 0.7}});
    auto intersectionSnap = guideMoveSnapPlan(document, lineForIntersection, {"line_for_intersection"}, guideSnapSettings, 0.04, 0.04);
    EDI_CHECK(intersectionSnap.ok);
    EDI_CHECK(intersectionSnap.intersection);
    EDI_CHECK(intersectionSnap.sourceObjectId == "guide_v");
    EDI_CHECK(near(intersectionSnap.snappedAnchor.x, 0.25));
    EDI_CHECK(near(intersectionSnap.snappedAnchor.y, 0.75));
    EDI_CHECK(near(intersectionSnap.dx, 0.05));
    EDI_CHECK(near(intersectionSnap.dy, 0.05));

    DraftingSnapSettings guideSnapDisabled = guideSnapSettings;
    guideSnapDisabled.guideEnabled = false;
    EDI_CHECK(!guideMoveSnapPlan(document, lineForSnap, {"line_for_snap"}, guideSnapDisabled, 0.04, 0.0).ok);

    return 0;
}
