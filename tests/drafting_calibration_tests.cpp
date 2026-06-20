#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
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
    DraftingCalibrationPatternRequest defaultRequest = defaultDraftingCalibrationPatternRequest(
        DraftingCalibrationPatternKind::LineSpacing,
        "calibration_default",
        "plot_layer");
    EDI_CHECK(defaultRequest.kind == DraftingCalibrationPatternKind::LineSpacing);
    EDI_CHECK(defaultRequest.idPrefix == "calibration_default");
    EDI_CHECK(defaultRequest.layerId == "plot_layer");
    EDI_CHECK(nearlyEqual(defaultRequest.origin.x, 0.15));
    EDI_CHECK(nearlyEqual(defaultRequest.origin.y, 0.15));
    EDI_CHECK(nearlyEqual(defaultRequest.size, 0.24));
    EDI_CHECK(nearlyEqual(defaultRequest.spacing, 0.04));
    EDI_CHECK(defaultRequest.lineCount == 5);
    const DraftingCalibrationPatternResult defaultPattern = buildDraftingCalibrationPattern(defaultRequest);
    EDI_CHECK(defaultPattern.ok);
    EDI_CHECK(defaultPattern.objects.size() == 5);

    DraftingCalibrationPatternRequest squareRequest;
    squareRequest.kind = DraftingCalibrationPatternKind::Square;
    squareRequest.idPrefix = "calibration_0001";
    squareRequest.layerId = "plot_layer";
    squareRequest.origin = {0.1, 0.2};
    squareRequest.size = 0.25;
    const DraftingCalibrationPatternResult square = buildDraftingCalibrationPattern(squareRequest);
    EDI_CHECK(square.ok);
    EDI_CHECK(square.objects.size() == 1);
    EDI_CHECK(square.objects.front().id == "calibration_0001_square");
    EDI_CHECK(square.objects.front().kind == DraftingShapeKind::Rectangle);
    EDI_CHECK(square.objects.front().layerId == "plot_layer");
    EDI_CHECK(square.objects.front().metadata.toolProvenance == "calibration_square");
    EDI_CHECK(nearlyEqual(square.objects.front().bounds.x, 0.1));
    EDI_CHECK(nearlyEqual(square.objects.front().bounds.y, 0.2));
    EDI_CHECK(nearlyEqual(square.objects.front().bounds.width, 0.25));
    EDI_CHECK(nearlyEqual(square.objects.front().bounds.height, 0.25));

    DraftingCalibrationPatternRequest circleRequest = squareRequest;
    circleRequest.kind = DraftingCalibrationPatternKind::Circle;
    circleRequest.idPrefix = "calibration_0002";
    const DraftingCalibrationPatternResult circle = buildDraftingCalibrationPattern(circleRequest);
    EDI_CHECK(circle.ok);
    EDI_CHECK(circle.objects.size() == 1);
    EDI_CHECK(circle.objects.front().id == "calibration_0002_circle");
    EDI_CHECK(circle.objects.front().kind == DraftingShapeKind::Circle);
    EDI_CHECK(nearlyEqual(circle.objects.front().bounds.x, 0.1));
    EDI_CHECK(nearlyEqual(circle.objects.front().bounds.y, 0.2));
    EDI_CHECK(nearlyEqual(circle.objects.front().bounds.width, 0.25));
    EDI_CHECK(nearlyEqual(circle.objects.front().bounds.height, 0.25));

    DraftingCalibrationPatternRequest spacingRequest = squareRequest;
    spacingRequest.kind = DraftingCalibrationPatternKind::LineSpacing;
    spacingRequest.idPrefix = "calibration_0003";
    spacingRequest.size = 0.4;
    spacingRequest.spacing = 0.05;
    spacingRequest.lineCount = 4;
    const DraftingCalibrationPatternResult spacing = buildDraftingCalibrationPattern(spacingRequest);
    EDI_CHECK(spacing.ok);
    EDI_CHECK(spacing.objects.size() == 4);
    for (std::size_t index = 0; index < spacing.objects.size(); ++index) {
        const DraftingObject &object = spacing.objects[index];
        EDI_CHECK(object.kind == DraftingShapeKind::Line);
        EDI_CHECK(object.layerId == "plot_layer");
        EDI_CHECK(object.metadata.toolProvenance == "calibration_line_spacing");
        EDI_CHECK(nearlyEqual(object.bounds.x, 0.1));
        EDI_CHECK(nearlyEqual(object.bounds.y, 0.2 + static_cast<double>(index) * 0.05));
        EDI_CHECK(nearlyEqual(object.bounds.width, 0.4));
        EDI_CHECK(nearlyEqual(object.bounds.height, 0.0));
    }

    const DraftingCalibrationMeasurementResult squareMeasurement = measureDraftingCalibrationPattern(
        {square.objects, 0.252, "bench_check"});
    EDI_CHECK(squareMeasurement.ok);
    EDI_CHECK(squareMeasurement.measurement.patternId == "calibration_square");
    EDI_CHECK(squareMeasurement.measurement.objectIds.size() == 1);
    EDI_CHECK(squareMeasurement.measurement.objectIds.front() == "calibration_0001_square");
    EDI_CHECK(nearlyEqual(squareMeasurement.measurement.expectedValue, 0.25));
    EDI_CHECK(nearlyEqual(squareMeasurement.measurement.measuredValue, 0.252));
    EDI_CHECK(nearlyEqual(squareMeasurement.measurement.errorValue, 0.002));
    EDI_CHECK(nearlyEqual(squareMeasurement.measurement.percentError, 0.8));
    EDI_CHECK(formatDraftingCalibrationMeasurementNote(squareMeasurement.measurement).find("calibration_square") != std::string::npos);
    const DraftingCalibrationCorrectionPlan squareCorrection = planDraftingCalibrationCorrection(squareMeasurement.measurement);
    EDI_CHECK(squareCorrection.ok);
    EDI_CHECK(squareCorrection.patternId == "calibration_square");
    EDI_CHECK(nearlyEqual(squareCorrection.expectedValue, 0.25));
    EDI_CHECK(nearlyEqual(squareCorrection.measuredValue, 0.252));
    EDI_CHECK(nearlyEqual(squareCorrection.scaleFactor, 0.25 / 0.252));
    EDI_CHECK(squareCorrection.correctionPercent < 0.0);

    const DraftingCalibrationMeasurementResult circleMeasurement = measureDraftingCalibrationPattern(
        {circle.objects, 0.2475, "bench_check"});
    EDI_CHECK(circleMeasurement.ok);
    EDI_CHECK(circleMeasurement.measurement.patternId == "calibration_circle");
    EDI_CHECK(nearlyEqual(circleMeasurement.measurement.expectedValue, 0.25));
    EDI_CHECK(nearlyEqual(circleMeasurement.measurement.errorValue, -0.0025));

    const DraftingCalibrationMeasurementResult spacingMeasurement = measureDraftingCalibrationPattern(
        {spacing.objects, 0.051, "bench_check"});
    EDI_CHECK(spacingMeasurement.ok);
    EDI_CHECK(spacingMeasurement.measurement.patternId == "calibration_line_spacing");
    EDI_CHECK(spacingMeasurement.measurement.objectIds.size() == 4);
    EDI_CHECK(nearlyEqual(spacingMeasurement.measurement.expectedValue, 0.05));
    EDI_CHECK(nearlyEqual(spacingMeasurement.measurement.errorValue, 0.001));
    EDI_CHECK(nearlyEqual(spacingMeasurement.measurement.percentError, 2.0));
    const DraftingCalibrationCorrectionPlan spacingCorrection = planDraftingCalibrationCorrection(spacingMeasurement.measurement);
    EDI_CHECK(spacingCorrection.ok);
    EDI_CHECK(nearlyEqual(spacingCorrection.scaleFactor, 0.05 / 0.051));

    DraftingDocument selectedMeasurementDocument = makeDraftingDocument("selected_calibration_doc");
    EDI_CHECK(addLayer(selectedMeasurementDocument, makeDraftingLayer("plot_layer", "Plot Layer", 1)).ok);
    for (const DraftingObject &object : spacing.objects) {
        EDI_CHECK(addObject(selectedMeasurementDocument, object).ok);
        selectedMeasurementDocument.selectedObjectIds.push_back(object.id);
    }
    const DraftingCalibrationMeasurementResult selectedSpacingMeasurement = measureSelectedDraftingCalibrationPattern(
        selectedMeasurementDocument,
        selectedMeasurementDocument.selectedObjectIds,
        0.052,
        "selection_test");
    EDI_CHECK(selectedSpacingMeasurement.ok);
    EDI_CHECK(selectedSpacingMeasurement.measurement.source == "selection_test");
    EDI_CHECK(selectedSpacingMeasurement.measurement.objectIds.size() == spacing.objects.size());
    EDI_CHECK(nearlyEqual(selectedSpacingMeasurement.measurement.expectedValue, 0.05));
    EDI_CHECK(nearlyEqual(selectedSpacingMeasurement.measurement.measuredValue, 0.052));

    EDI_CHECK(!measureSelectedDraftingCalibrationPattern(selectedMeasurementDocument, {}, 0.05, "selection_test").ok);
    auto missingSelectionMeasurement = measureSelectedDraftingCalibrationPattern(
        selectedMeasurementDocument,
        {"missing_object"},
        0.05,
        "selection_test");
    EDI_CHECK(!missingSelectionMeasurement.ok);
    EDI_CHECK(missingSelectionMeasurement.code == DraftingResultCode::InvalidSelectionTarget);

    DraftingObject nonCalibrationObject = square.objects.front();
    nonCalibrationObject.metadata.toolProvenance.clear();
    EDI_CHECK(!measureDraftingCalibrationPattern({{}, 0.25, "bench_check"}).ok);
    EDI_CHECK(!measureDraftingCalibrationPattern({square.objects, 0.0, "bench_check"}).ok);
    EDI_CHECK(!measureDraftingCalibrationPattern({std::vector<DraftingObject>{nonCalibrationObject}, 0.25, "bench_check"}).ok);

    std::vector<DraftingObject> mixedObjects = square.objects;
    mixedObjects.push_back(circle.objects.front());
    EDI_CHECK(!measureDraftingCalibrationPattern({mixedObjects, 0.25, "bench_check"}).ok);
    DraftingCalibrationMeasurement invalidMeasurement = squareMeasurement.measurement;
    invalidMeasurement.expectedValue = 0.0;
    EDI_CHECK(!planDraftingCalibrationCorrection(invalidMeasurement).ok);
    invalidMeasurement = squareMeasurement.measurement;
    invalidMeasurement.measuredValue = 0.0;
    EDI_CHECK(!planDraftingCalibrationCorrection(invalidMeasurement).ok);

    DraftingCalibrationPatternRequest invalid = squareRequest;
    invalid.idPrefix.clear();
    EDI_CHECK(!buildDraftingCalibrationPattern(invalid).ok);
    invalid = squareRequest;
    invalid.size = 0.0;
    EDI_CHECK(!buildDraftingCalibrationPattern(invalid).ok);
    invalid = squareRequest;
    invalid.kind = DraftingCalibrationPatternKind::LineSpacing;
    invalid.lineCount = 0;
    EDI_CHECK(!buildDraftingCalibrationPattern(invalid).ok);

    EDI_CHECK(draftingCalibrationPatternKindFromId("test_circle") == DraftingCalibrationPatternKind::Circle);
    EDI_CHECK(draftingCalibrationPatternKindFromId("line_spacing") == DraftingCalibrationPatternKind::LineSpacing);
    EDI_CHECK(draftingCalibrationPatternKindFromId("anything_else") == DraftingCalibrationPatternKind::Square);

    return 0;
}
