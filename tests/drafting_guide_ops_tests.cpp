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

    return 0;
}
