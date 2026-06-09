#include "drafting/DraftingAlign.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <string>
#include <utility>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

void add(DraftingDocument &document, DraftingObject object)
{
    auto result = addObject(document, std::move(object));
    assert(result.ok);
}

const DraftingTranslation *findTranslation(const DraftingAlignmentResult &result, const DraftingObjectId &id)
{
    for (const DraftingTranslation &translation : result.translations) {
        if (translation.objectId == id) {
            return &translation;
        }
    }
    return nullptr;
}

} // namespace

int main()
{
    assert(std::string(draftingAlignmentModeName(DraftingAlignmentMode::CenterX)) == "center_x");
    assert(draftingAlignmentModeFromId("left") == DraftingAlignmentMode::Left);
    assert(draftingAlignmentModeFromId("right") == DraftingAlignmentMode::Right);
    assert(draftingAlignmentModeFromId("top") == DraftingAlignmentMode::Top);
    assert(draftingAlignmentModeFromId("bottom") == DraftingAlignmentMode::Bottom);
    assert(draftingAlignmentModeFromId("center_x") == DraftingAlignmentMode::CenterX);
    assert(draftingAlignmentModeFromId("center_y") == DraftingAlignmentMode::CenterY);
    assert(!draftingAlignmentModeFromId("distribute_x"));
    assert(draftingDistributeModeFromAxisId("x") == DraftingAlignmentMode::DistributeX);
    assert(draftingDistributeModeFromAxisId("y") == DraftingAlignmentMode::DistributeY);
    assert(!draftingDistributeModeFromAxisId("z"));

    DraftingDocument document = makeDraftingDocument("align_doc");
    add(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.2, 0.3}, 0.2, 0.1}));
    add(document, object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.7, 0.5}, 0.05}));
    add(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.9}}));

    auto left = planDraftingAlignment(document, {"rect_1", "circle_1", "point_1"}, DraftingAlignmentMode::Left);
    assert(left.ok);
    assert(left.translations.size() == 2);
    const DraftingTranslation *circleLeft = findTranslation(left, "circle_1");
    const DraftingTranslation *pointLeft = findTranslation(left, "point_1");
    assert(circleLeft != nullptr);
    assert(pointLeft != nullptr);
    assert(nearlyEqual(circleLeft->dx, -0.45));
    assert(nearlyEqual(circleLeft->dy, 0.0));
    assert(nearlyEqual(pointLeft->dx, -0.3));

    auto centerY = planDraftingAlignment(document, {"rect_1", "circle_1", "point_1"}, DraftingAlignmentMode::CenterY);
    assert(centerY.ok);
    const DraftingTranslation *rectCenterY = findTranslation(centerY, "rect_1");
    const DraftingTranslation *circleCenterY = findTranslation(centerY, "circle_1");
    const DraftingTranslation *pointCenterY = findTranslation(centerY, "point_1");
    assert(rectCenterY != nullptr);
    assert(circleCenterY != nullptr);
    assert(pointCenterY != nullptr);
    assert(nearlyEqual(rectCenterY->dy, 0.25));
    assert(nearlyEqual(circleCenterY->dy, 0.1));
    assert(nearlyEqual(pointCenterY->dy, -0.3));

    DraftingDocument distributeDocument = makeDraftingDocument("distribute_doc");
    add(distributeDocument, object("left_point", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}}));
    add(distributeDocument, object("middle_point", DraftingShapeKind::Point, PointGeometry{{0.4, 0.5}}));
    add(distributeDocument, object("right_point", DraftingShapeKind::Point, PointGeometry{{0.9, 0.9}}));
    auto distributeX = planDraftingAlignment(
        distributeDocument,
        {"right_point", "middle_point", "left_point"},
        DraftingAlignmentMode::DistributeX);
    assert(distributeX.ok);
    assert(distributeX.translations.size() == 1);
    assert(distributeX.translations.front().objectId == "middle_point");
    assert(nearlyEqual(distributeX.translations.front().dx, 0.1));
    assert(nearlyEqual(distributeX.translations.front().dy, 0.0));

    auto distributeY = planDraftingAlignment(
        distributeDocument,
        {"right_point", "middle_point", "left_point"},
        DraftingAlignmentMode::DistributeY);
    assert(distributeY.ok);
    assert(distributeY.translations.empty());

    auto tooFewForAlign = planDraftingAlignment(document, {"rect_1"}, DraftingAlignmentMode::Left);
    assert(!tooFewForAlign.ok);
    assert(tooFewForAlign.code == DraftingResultCode::InvalidSelectionTarget);

    auto tooFewForDistribute = planDraftingAlignment(document, {"rect_1", "circle_1"}, DraftingAlignmentMode::DistributeX);
    assert(!tooFewForDistribute.ok);
    assert(tooFewForDistribute.code == DraftingResultCode::InvalidSelectionTarget);

    auto missing = planDraftingAlignment(document, {"rect_1", "missing"}, DraftingAlignmentMode::Right);
    assert(!missing.ok);
    assert(missing.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingDocument lockedDocument = document;
    findObject(lockedDocument, "circle_1")->locked = true;
    auto locked = planDraftingAlignment(lockedDocument, {"rect_1", "circle_1"}, DraftingAlignmentMode::Left);
    assert(!locked.ok);
    assert(locked.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingDocument hiddenDocument = document;
    findObject(hiddenDocument, "circle_1")->visible = false;
    auto hidden = planDraftingAlignment(hiddenDocument, {"rect_1", "circle_1"}, DraftingAlignmentMode::Left);
    assert(!hidden.ok);
    assert(hidden.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingDocument missingLayerDocument = document;
    findObject(missingLayerDocument, "circle_1")->layerId = "missing_layer";
    auto missingLayer = planDraftingAlignment(missingLayerDocument, {"rect_1", "circle_1"}, DraftingAlignmentMode::Left);
    assert(!missingLayer.ok);
    assert(missingLayer.code == DraftingResultCode::LayerNotFound);

    return 0;
}
