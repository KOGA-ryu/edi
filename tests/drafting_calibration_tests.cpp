#include "drafting/DraftingCalibration.h"

#include <cassert>
#include <cstddef>
#include <cmath>
#include <string>
#include <vector>

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

    const DraftingCalibrationMeasurementResult squareMeasurement = measureDraftingCalibrationPattern(
        {square.objects, 0.252, "bench_check"});
    assert(squareMeasurement.ok);
    assert(squareMeasurement.measurement.patternId == "calibration_square");
    assert(squareMeasurement.measurement.objectIds.size() == 1);
    assert(squareMeasurement.measurement.objectIds.front() == "calibration_0001_square");
    assert(nearlyEqual(squareMeasurement.measurement.expectedValue, 0.25));
    assert(nearlyEqual(squareMeasurement.measurement.measuredValue, 0.252));
    assert(nearlyEqual(squareMeasurement.measurement.errorValue, 0.002));
    assert(nearlyEqual(squareMeasurement.measurement.percentError, 0.8));
    assert(formatDraftingCalibrationMeasurementNote(squareMeasurement.measurement).find("calibration_square") != std::string::npos);

    const DraftingCalibrationMeasurementResult circleMeasurement = measureDraftingCalibrationPattern(
        {circle.objects, 0.2475, "bench_check"});
    assert(circleMeasurement.ok);
    assert(circleMeasurement.measurement.patternId == "calibration_circle");
    assert(nearlyEqual(circleMeasurement.measurement.expectedValue, 0.25));
    assert(nearlyEqual(circleMeasurement.measurement.errorValue, -0.0025));

    const DraftingCalibrationMeasurementResult spacingMeasurement = measureDraftingCalibrationPattern(
        {spacing.objects, 0.051, "bench_check"});
    assert(spacingMeasurement.ok);
    assert(spacingMeasurement.measurement.patternId == "calibration_line_spacing");
    assert(spacingMeasurement.measurement.objectIds.size() == 4);
    assert(nearlyEqual(spacingMeasurement.measurement.expectedValue, 0.05));
    assert(nearlyEqual(spacingMeasurement.measurement.errorValue, 0.001));
    assert(nearlyEqual(spacingMeasurement.measurement.percentError, 2.0));

    DraftingObject nonCalibrationObject = square.objects.front();
    nonCalibrationObject.metadata.toolProvenance.clear();
    assert(!measureDraftingCalibrationPattern({{}, 0.25, "bench_check"}).ok);
    assert(!measureDraftingCalibrationPattern({square.objects, 0.0, "bench_check"}).ok);
    assert(!measureDraftingCalibrationPattern({std::vector<DraftingObject>{nonCalibrationObject}, 0.25, "bench_check"}).ok);

    std::vector<DraftingObject> mixedObjects = square.objects;
    mixedObjects.push_back(circle.objects.front());
    assert(!measureDraftingCalibrationPattern({mixedObjects, 0.25, "bench_check"}).ok);

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
