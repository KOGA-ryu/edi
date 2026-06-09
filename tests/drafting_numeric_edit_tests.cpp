#include "drafting/DraftingNumericEdit.h"

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

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {3.0, 4.0}});
    const auto *sourceLine = std::get_if<LineGeometry>(&line.geometry);
    assert(sourceLine != nullptr);
    assert(nearlyEqual(lineAngleDegrees(*sourceLine), 53.1301023542));

    auto lengthEdit = applyNumericGeometryEdit(line, "line_length", 10.0);
    assert(lengthEdit.ok);
    const auto *lengthLine = std::get_if<LineGeometry>(&lengthEdit.geometry);
    assert(lengthLine != nullptr);
    assert(nearlyEqual(lengthLine->a.x, 0.0));
    assert(nearlyEqual(lengthLine->a.y, 0.0));
    assert(nearlyEqual(lengthLine->b.x, 6.0));
    assert(nearlyEqual(lengthLine->b.y, 8.0));

    auto angleEdit = applyNumericGeometryEdit(line, "line_angle_deg", 0.0);
    assert(angleEdit.ok);
    const auto *angleLine = std::get_if<LineGeometry>(&angleEdit.geometry);
    assert(angleLine != nullptr);
    assert(nearlyEqual(angleLine->b.x, 5.0));
    assert(nearlyEqual(angleLine->b.y, 0.0));

    auto negativeLength = applyNumericGeometryEdit(line, "line_length", -1.0);
    assert(!negativeLength.ok);
    assert(negativeLength.code == DraftingResultCode::InvalidGeometry);

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    auto diameterEdit = applyNumericGeometryEdit(circle, "diameter", 0.5);
    assert(diameterEdit.ok);
    const auto *diameterCircle = std::get_if<CircleGeometry>(&diameterEdit.geometry);
    assert(diameterCircle != nullptr);
    assert(nearlyEqual(diameterCircle->center.x, 0.25));
    assert(nearlyEqual(diameterCircle->radius, 0.25));

    auto negativeDiameter = applyNumericGeometryEdit(circle, "diameter", -0.5);
    assert(!negativeDiameter.ok);
    assert(negativeDiameter.code == DraftingResultCode::InvalidGeometry);

    auto negativeRadius = applyNumericGeometryEdit(circle, "radius", -0.1);
    assert(!negativeRadius.ok);
    assert(negativeRadius.code == DraftingResultCode::InvalidGeometry);

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.1, 0.2}, 0.3, 0.4});
    auto negativeWidth = applyNumericGeometryEdit(rect, "width", -0.1);
    assert(!negativeWidth.ok);
    assert(negativeWidth.code == DraftingResultCode::InvalidGeometry);

    auto pointX = applyNumericGeometryEdit(object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}}), "x", 0.5);
    assert(pointX.ok);
    const auto *point = std::get_if<PointGeometry>(&pointX.geometry);
    assert(point != nullptr);
    assert(nearlyEqual(point->point.x, 0.5));
    assert(nearlyEqual(point->point.y, 0.2));

    auto badField = applyNumericGeometryEdit(line, "missing", 1.0);
    assert(!badField.ok);
    assert(badField.code == DraftingResultCode::InvalidGeometry);

    auto badValue = applyNumericGeometryEdit(line, "x1", std::numeric_limits<double>::infinity());
    assert(!badValue.ok);
    assert(badValue.code == DraftingResultCode::InvalidGeometry);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Circle;
    auto mismatch = applyNumericGeometryEdit(mismatched, "line_length", 2.0);
    assert(!mismatch.ok);
    assert(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    return 0;
}
