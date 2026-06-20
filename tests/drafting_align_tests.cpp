#include "drafting/DraftingAlign.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
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
    EDI_CHECK(built.ok);
    return built.object;
}

void add(DraftingDocument &document, DraftingObject object)
{
    auto result = addObject(document, std::move(object));
    EDI_CHECK(result.ok);
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
    EDI_CHECK(std::string(draftingAlignmentModeName(DraftingAlignmentMode::CenterX)) == "center_x");
    EDI_CHECK(draftingAlignmentModeFromId("left") == DraftingAlignmentMode::Left);
    EDI_CHECK(draftingAlignmentModeFromId("right") == DraftingAlignmentMode::Right);
    EDI_CHECK(draftingAlignmentModeFromId("top") == DraftingAlignmentMode::Top);
    EDI_CHECK(draftingAlignmentModeFromId("bottom") == DraftingAlignmentMode::Bottom);
    EDI_CHECK(draftingAlignmentModeFromId("center_x") == DraftingAlignmentMode::CenterX);
    EDI_CHECK(draftingAlignmentModeFromId("center_y") == DraftingAlignmentMode::CenterY);
    EDI_CHECK(!draftingAlignmentModeFromId("distribute_x"));
    EDI_CHECK(draftingDistributeModeFromAxisId("x") == DraftingAlignmentMode::DistributeX);
    EDI_CHECK(draftingDistributeModeFromAxisId("y") == DraftingAlignmentMode::DistributeY);
    EDI_CHECK(!draftingDistributeModeFromAxisId("z"));

    DraftingDocument document = makeDraftingDocument("align_doc");
    add(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.2, 0.3}, 0.2, 0.1}));
    add(document, object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.7, 0.5}, 0.05}));
    add(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.5, 0.9}}));

    auto left = planDraftingAlignment(document, {"rect_1", "circle_1", "point_1"}, DraftingAlignmentMode::Left);
    EDI_CHECK(left.ok);
    EDI_CHECK(left.translations.size() == 2);
    const DraftingTranslation *circleLeft = findTranslation(left, "circle_1");
    const DraftingTranslation *pointLeft = findTranslation(left, "point_1");
    EDI_CHECK(circleLeft != nullptr);
    EDI_CHECK(pointLeft != nullptr);
    EDI_CHECK(nearlyEqual(circleLeft->dx, -0.45));
    EDI_CHECK(nearlyEqual(circleLeft->dy, 0.0));
    EDI_CHECK(nearlyEqual(pointLeft->dx, -0.3));

    auto centerY = planDraftingAlignment(document, {"rect_1", "circle_1", "point_1"}, DraftingAlignmentMode::CenterY);
    EDI_CHECK(centerY.ok);
    const DraftingTranslation *rectCenterY = findTranslation(centerY, "rect_1");
    const DraftingTranslation *circleCenterY = findTranslation(centerY, "circle_1");
    const DraftingTranslation *pointCenterY = findTranslation(centerY, "point_1");
    EDI_CHECK(rectCenterY != nullptr);
    EDI_CHECK(circleCenterY != nullptr);
    EDI_CHECK(pointCenterY != nullptr);
    EDI_CHECK(nearlyEqual(rectCenterY->dy, 0.25));
    EDI_CHECK(nearlyEqual(circleCenterY->dy, 0.1));
    EDI_CHECK(nearlyEqual(pointCenterY->dy, -0.3));

    DraftingDocument distributeDocument = makeDraftingDocument("distribute_doc");
    add(distributeDocument, object("left_point", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}}));
    add(distributeDocument, object("middle_point", DraftingShapeKind::Point, PointGeometry{{0.4, 0.5}}));
    add(distributeDocument, object("right_point", DraftingShapeKind::Point, PointGeometry{{0.9, 0.9}}));
    auto distributeX = planDraftingAlignment(
        distributeDocument,
        {"right_point", "middle_point", "left_point"},
        DraftingAlignmentMode::DistributeX);
    EDI_CHECK(distributeX.ok);
    EDI_CHECK(distributeX.translations.size() == 1);
    EDI_CHECK(distributeX.translations.front().objectId == "middle_point");
    EDI_CHECK(nearlyEqual(distributeX.translations.front().dx, 0.1));
    EDI_CHECK(nearlyEqual(distributeX.translations.front().dy, 0.0));

    auto distributeY = planDraftingAlignment(
        distributeDocument,
        {"right_point", "middle_point", "left_point"},
        DraftingAlignmentMode::DistributeY);
    EDI_CHECK(distributeY.ok);
    EDI_CHECK(distributeY.translations.empty());

    auto tooFewForAlign = planDraftingAlignment(document, {"rect_1"}, DraftingAlignmentMode::Left);
    EDI_CHECK(!tooFewForAlign.ok);
    EDI_CHECK(tooFewForAlign.code == DraftingResultCode::InvalidSelectionTarget);

    auto tooFewForDistribute = planDraftingAlignment(document, {"rect_1", "circle_1"}, DraftingAlignmentMode::DistributeX);
    EDI_CHECK(!tooFewForDistribute.ok);
    EDI_CHECK(tooFewForDistribute.code == DraftingResultCode::InvalidSelectionTarget);

    auto missing = planDraftingAlignment(document, {"rect_1", "missing"}, DraftingAlignmentMode::Right);
    EDI_CHECK(!missing.ok);
    EDI_CHECK(missing.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingDocument lockedDocument = document;
    findObject(lockedDocument, "circle_1")->locked = true;
    auto locked = planDraftingAlignment(lockedDocument, {"rect_1", "circle_1"}, DraftingAlignmentMode::Left);
    EDI_CHECK(!locked.ok);
    EDI_CHECK(locked.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingDocument hiddenDocument = document;
    findObject(hiddenDocument, "circle_1")->visible = false;
    auto hidden = planDraftingAlignment(hiddenDocument, {"rect_1", "circle_1"}, DraftingAlignmentMode::Left);
    EDI_CHECK(!hidden.ok);
    EDI_CHECK(hidden.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingDocument missingLayerDocument = document;
    findObject(missingLayerDocument, "circle_1")->layerId = "missing_layer";
    auto missingLayer = planDraftingAlignment(missingLayerDocument, {"rect_1", "circle_1"}, DraftingAlignmentMode::Left);
    EDI_CHECK(!missingLayer.ok);
    EDI_CHECK(missingLayer.code == DraftingResultCode::LayerNotFound);

    return 0;
}
