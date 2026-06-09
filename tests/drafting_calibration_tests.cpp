#include "drafting/DraftingCalibration.h"

#include <cassert>
#include <cstddef>
#include <cmath>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    DraftingCalibrationPatternRequest squareRequest;
    squareRequest.kind = DraftingCalibrationPatternKind::Square;
    squareRequest.idPrefix = "calibration_0001";
    squareRequest.layerId = "plot_layer";
    squareRequest.origin = {0.1, 0.2};
    squareRequest.size = 0.25;
    const DraftingCalibrationPatternResult square = buildDraftingCalibrationPattern(squareRequest);
    assert(square.ok);
    assert(square.objects.size() == 1);
    assert(square.objects.front().id == "calibration_0001_square");
    assert(square.objects.front().kind == DraftingShapeKind::Rectangle);
    assert(square.objects.front().layerId == "plot_layer");
    assert(square.objects.front().metadata.toolProvenance == "calibration_square");
    assert(nearlyEqual(square.objects.front().bounds.x, 0.1));
    assert(nearlyEqual(square.objects.front().bounds.y, 0.2));
    assert(nearlyEqual(square.objects.front().bounds.width, 0.25));
    assert(nearlyEqual(square.objects.front().bounds.height, 0.25));

    DraftingCalibrationPatternRequest circleRequest = squareRequest;
    circleRequest.kind = DraftingCalibrationPatternKind::Circle;
    circleRequest.idPrefix = "calibration_0002";
    const DraftingCalibrationPatternResult circle = buildDraftingCalibrationPattern(circleRequest);
    assert(circle.ok);
    assert(circle.objects.size() == 1);
    assert(circle.objects.front().id == "calibration_0002_circle");
    assert(circle.objects.front().kind == DraftingShapeKind::Circle);
    assert(nearlyEqual(circle.objects.front().bounds.x, 0.1));
    assert(nearlyEqual(circle.objects.front().bounds.y, 0.2));
    assert(nearlyEqual(circle.objects.front().bounds.width, 0.25));
    assert(nearlyEqual(circle.objects.front().bounds.height, 0.25));

    DraftingCalibrationPatternRequest spacingRequest = squareRequest;
    spacingRequest.kind = DraftingCalibrationPatternKind::LineSpacing;
    spacingRequest.idPrefix = "calibration_0003";
    spacingRequest.size = 0.4;
    spacingRequest.spacing = 0.05;
    spacingRequest.lineCount = 4;
    const DraftingCalibrationPatternResult spacing = buildDraftingCalibrationPattern(spacingRequest);
    assert(spacing.ok);
    assert(spacing.objects.size() == 4);
    for (std::size_t index = 0; index < spacing.objects.size(); ++index) {
        const DraftingObject &object = spacing.objects[index];
        assert(object.kind == DraftingShapeKind::Line);
        assert(object.layerId == "plot_layer");
        assert(object.metadata.toolProvenance == "calibration_line_spacing");
        assert(nearlyEqual(object.bounds.x, 0.1));
        assert(nearlyEqual(object.bounds.y, 0.2 + static_cast<double>(index) * 0.05));
        assert(nearlyEqual(object.bounds.width, 0.4));
        assert(nearlyEqual(object.bounds.height, 0.0));
    }

    DraftingCalibrationPatternRequest invalid = squareRequest;
    invalid.idPrefix.clear();
    assert(!buildDraftingCalibrationPattern(invalid).ok);
    invalid = squareRequest;
    invalid.size = 0.0;
    assert(!buildDraftingCalibrationPattern(invalid).ok);
    invalid = squareRequest;
    invalid.kind = DraftingCalibrationPatternKind::LineSpacing;
    invalid.lineCount = 0;
    assert(!buildDraftingCalibrationPattern(invalid).ok);

    assert(draftingCalibrationPatternKindFromId("test_circle") == DraftingCalibrationPatternKind::Circle);
    assert(draftingCalibrationPatternKindFromId("line_spacing") == DraftingCalibrationPatternKind::LineSpacing);
    assert(draftingCalibrationPatternKindFromId("anything_else") == DraftingCalibrationPatternKind::Square);

    return 0;
}
