#include "drafting/DraftingArray.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.2}, {0.4, 0.2}});
    line.stroke.color = "#55ccaa";
    auto repeatedLine = repeatDraftingObject(line, {"repeat_1", "repeat_2", "repeat_3"}, 0.1, 0.0);
    assert(repeatedLine.ok);
    assert(repeatedLine.objects.size() == 3);
    assert(repeatedLine.objects[0].id == "repeat_1");
    assert(repeatedLine.objects[0].kind == DraftingShapeKind::Line);
    assert(repeatedLine.objects[0].metadata.toolProvenance == "repeat");
    assert(repeatedLine.objects[0].metadata.source == "line_1");
    assert(repeatedLine.objects[0].stroke.color == "#55ccaa");
    const auto *firstLine = std::get_if<LineGeometry>(&repeatedLine.objects[0].geometry);
    const auto *thirdLine = std::get_if<LineGeometry>(&repeatedLine.objects[2].geometry);
    assert(firstLine != nullptr);
    assert(thirdLine != nullptr);
    assert(nearlyEqual(firstLine->a.x, 0.2));
    assert(nearlyEqual(firstLine->a.y, 0.2));
    assert(nearlyEqual(thirdLine->a.x, 0.4));
    assert(nearlyEqual(thirdLine->b.x, 0.7));

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    auto repeatedCircle = repeatDraftingObject(circle, {"circle_repeat_1", "circle_repeat_2"}, 0.0, 0.2);
    assert(repeatedCircle.ok);
    const auto *secondCircle = std::get_if<CircleGeometry>(&repeatedCircle.objects[1].geometry);
    assert(secondCircle != nullptr);
    assert(nearlyEqual(secondCircle->center.x, 0.25));
    assert(nearlyEqual(secondCircle->center.y, 0.65));
    assert(nearlyEqual(secondCircle->radius, 0.1));

    DraftingObject guide = object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.8});
    auto invalidGuideRepeat = repeatDraftingObject(guide, {"guide_repeat_1", "guide_repeat_2", "guide_repeat_3"}, 0.0, 0.1);
    assert(!invalidGuideRepeat.ok);
    assert(invalidGuideRepeat.code == DraftingResultCode::InvalidGeometry);

    auto empty = repeatDraftingObject(line, {}, 0.1, 0.0);
    assert(!empty.ok);
    assert(empty.code == DraftingResultCode::InvalidGeometry);

    auto zeroSpacing = repeatDraftingObject(line, {"zero_repeat"}, 0.0, 0.0);
    assert(!zeroSpacing.ok);
    assert(zeroSpacing.code == DraftingResultCode::InvalidGeometry);

    auto badSpacing = repeatDraftingObject(line, {"bad_repeat"}, std::numeric_limits<double>::infinity(), 0.0);
    assert(!badSpacing.ok);
    assert(badSpacing.code == DraftingResultCode::InvalidGeometry);

    auto duplicateIds = repeatDraftingObject(line, {"duplicate_repeat", "duplicate_repeat"}, 0.1, 0.0);
    assert(!duplicateIds.ok);
    assert(duplicateIds.code == DraftingResultCode::DuplicateObjectId);

    auto sourceIdReuse = repeatDraftingObject(line, {"line_1"}, 0.1, 0.0);
    assert(!sourceIdReuse.ok);
    assert(sourceIdReuse.code == DraftingResultCode::DuplicateObjectId);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Point;
    auto mismatch = repeatDraftingObject(mismatched, {"mismatch_repeat"}, 0.1, 0.0);
    assert(!mismatch.ok);
    assert(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    return 0;
}
