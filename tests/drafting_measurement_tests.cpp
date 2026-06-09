#include "drafting/DraftingDocument.h"
#include "drafting/DraftingMeasurement.h"

#include <cassert>
#include <string>

using namespace edi::drafting;

int main()
{
    ScaleCalibration calibration{2.0, MeasurementUnit::Centimeter};
    MeasurementValue measured = measureDistance({0.0, 0.0}, {0.0, 10.0}, calibration);
    assert(measured.kind == MeasurementKind::Distance);
    assert(measured.value == 5.0);
    assert(measured.unit == MeasurementUnit::Centimeter);
    assert(measured.label == std::string("centimeter"));

    DraftingGeometry rect = RectangleGeometry{{2.0, 3.0}, 10.0, 5.0};
    MeasurementValue measuredArea = measureArea(rect, calibration);
    assert(measuredArea.kind == MeasurementKind::Area);
    assert(measuredArea.value == 12.5);
    assert(measuredArea.unit == MeasurementUnit::Centimeter);

    Bounds2D dimensions = measureDimensions(rect);
    assert(dimensions.x == 2.0);
    assert(dimensions.y == 3.0);
    assert(dimensions.width == 10.0);
    assert(dimensions.height == 5.0);
    DimensionMeasurement typedDimensions = measureDimensionsTyped(rect, calibration);
    assert(typedDimensions.width.kind == MeasurementKind::Dimension);
    assert(typedDimensions.height.kind == MeasurementKind::Dimension);
    assert(typedDimensions.width.value == 5.0);
    assert(typedDimensions.height.value == 2.5);
    assert(typedDimensions.width.unit == MeasurementUnit::Centimeter);
    assert(typedDimensions.height.unit == MeasurementUnit::Centimeter);

    DraftingGeometry line = LineGeometry{{0.0, 0.0}, {3.0, 4.0}};
    DimensionMeasurement lineDimensions = measureDimensionsTyped(line, calibration);
    assert(lineDimensions.width.value == 1.5);
    assert(lineDimensions.height.value == 2.0);

    DimensionMeasurement defaultDimensions = measureDimensionsTyped(rect);
    assert(defaultDimensions.width.kind == MeasurementKind::Dimension);
    assert(defaultDimensions.width.value == 10.0);
    assert(defaultDimensions.height.value == 5.0);
    assert(defaultDimensions.width.unit == MeasurementUnit::CanvasUnit);

    assert(measurementUnitName(MeasurementUnit::None) == std::string("none"));
    assert(measurementUnitName(MeasurementUnit::CanvasUnit) == std::string("canvas_unit"));
    assert(measurementUnitName(MeasurementUnit::Millimeter) == std::string("millimeter"));
    assert(measurementUnitName(MeasurementUnit::Centimeter) == std::string("centimeter"));
    assert(measurementUnitName(MeasurementUnit::Meter) == std::string("meter"));
    assert(measurementUnitName(MeasurementUnit::Inch) == std::string("inch"));
    assert(measurementUnitName(MeasurementUnit::Foot) == std::string("foot"));

    MeasurementMetadata metadataCalibration;
    metadataCalibration.unit = MeasurementUnit::Inch;
    metadataCalibration.canvasUnitsPerRealUnit = 4.0;
    auto checkedFromMetadata = scaleCalibrationFromMetadataChecked(metadataCalibration);
    assert(checkedFromMetadata.ok);
    assert(checkedFromMetadata.code == DraftingResultCode::None);
    assert(checkedFromMetadata.calibration.realUnit == MeasurementUnit::Inch);
    assert(checkedFromMetadata.calibration.canvasUnitsPerRealUnit == 4.0);
    ScaleCalibration fromMetadata = scaleCalibrationFromMetadata(metadataCalibration);
    assert(fromMetadata.realUnit == MeasurementUnit::Inch);
    assert(fromMetadata.canvasUnitsPerRealUnit == 4.0);
    MeasurementValue metadataMeasured = measureDistance({0.0, 0.0}, {0.0, 12.0}, fromMetadata);
    assert(metadataMeasured.kind == MeasurementKind::Distance);
    assert(metadataMeasured.value == 3.0);
    assert(metadataMeasured.unit == MeasurementUnit::Inch);

    ScaleCalibration defaultFromMetadata = scaleCalibrationFromMetadata(MeasurementMetadata{});
    assert(defaultFromMetadata.realUnit == MeasurementUnit::CanvasUnit);
    assert(defaultFromMetadata.canvasUnitsPerRealUnit == 1.0);
    auto checkedDefaultFromMetadata = scaleCalibrationFromMetadataChecked(MeasurementMetadata{});
    assert(checkedDefaultFromMetadata.ok);
    assert(checkedDefaultFromMetadata.calibration.realUnit == MeasurementUnit::CanvasUnit);
    assert(checkedDefaultFromMetadata.calibration.canvasUnitsPerRealUnit == 1.0);

    MeasurementMetadata invalidCalibration;
    invalidCalibration.unit = MeasurementUnit::Foot;
    invalidCalibration.canvasUnitsPerRealUnit = 0.0;
    auto checkedInvalidCalibration = scaleCalibrationFromMetadataChecked(invalidCalibration);
    assert(!checkedInvalidCalibration.ok);
    assert(checkedInvalidCalibration.code == DraftingResultCode::InvalidMetadata);
    ScaleCalibration fallbackFromInvalid = scaleCalibrationFromMetadata(invalidCalibration);
    assert(fallbackFromInvalid.realUnit == MeasurementUnit::CanvasUnit);
    assert(fallbackFromInvalid.canvasUnitsPerRealUnit == 1.0);

    MeasurementMetadata invalidLabelCalibration;
    invalidLabelCalibration.unit = MeasurementUnit::Foot;
    invalidLabelCalibration.canvasUnitsPerRealUnit = 2.0;
    invalidLabelCalibration.label = "bad\nlabel";
    auto checkedInvalidLabelCalibration = scaleCalibrationFromMetadataChecked(invalidLabelCalibration);
    assert(!checkedInvalidLabelCalibration.ok);
    assert(checkedInvalidLabelCalibration.code == DraftingResultCode::InvalidMetadata);

    DraftingObject measuredLine = makeDraftingObject("measured_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.0, 12.0}});
    measuredLine.metadata.measurement.unit = MeasurementUnit::Inch;
    measuredLine.metadata.measurement.canvasUnitsPerRealUnit = 4.0;
    auto objectDistance = measureObjectDistance(measuredLine);
    assert(objectDistance.ok);
    assert(objectDistance.value.kind == MeasurementKind::Distance);
    assert(objectDistance.value.value == 3.0);
    assert(objectDistance.value.unit == MeasurementUnit::Inch);
    auto lineSummary = summarizeObjectMeasurement(measuredLine);
    assert(lineSummary.ok);
    assert(lineSummary.value.hasDistance);
    assert(lineSummary.value.distance.value == 3.0);
    assert(!lineSummary.value.hasArea);
    assert(lineSummary.value.dimensions.width.value == 0.0);
    assert(lineSummary.value.dimensions.height.value == 3.0);

    DraftingObject measuredDimension = makeDraftingObject("measured_dimension", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {3.0, 4.0}, 0.25});
    auto dimensionDistance = measureObjectDistance(measuredDimension);
    assert(dimensionDistance.ok);
    assert(dimensionDistance.value.kind == MeasurementKind::Distance);
    assert(dimensionDistance.value.value == 5.0);
    auto dimensionSummary = summarizeObjectMeasurement(measuredDimension);
    assert(dimensionSummary.ok);
    assert(dimensionSummary.value.hasDistance);
    assert(dimensionSummary.value.distance.value == 5.0);
    assert(!dimensionSummary.value.hasArea);

    DraftingObject measuredRect = makeDraftingObject("measured_rect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.0, 0.0}, 10.0, 5.0});
    measuredRect.metadata.measurement.unit = MeasurementUnit::Centimeter;
    measuredRect.metadata.measurement.canvasUnitsPerRealUnit = 2.0;
    auto objectArea = measureObjectArea(measuredRect);
    assert(objectArea.ok);
    assert(objectArea.value.kind == MeasurementKind::Area);
    assert(objectArea.value.value == 12.5);
    assert(objectArea.value.unit == MeasurementUnit::Centimeter);
    auto objectDimensions = measureObjectDimensions(measuredRect);
    assert(objectDimensions.ok);
    assert(objectDimensions.value.width.value == 5.0);
    assert(objectDimensions.value.height.value == 2.5);
    auto rectSummary = summarizeObjectMeasurement(measuredRect);
    assert(rectSummary.ok);
    assert(!rectSummary.value.hasDistance);
    assert(rectSummary.value.hasArea);
    assert(rectSummary.value.area.value == 12.5);
    assert(rectSummary.value.dimensions.width.value == 5.0);
    assert(rectSummary.value.dimensions.height.value == 2.5);

    DraftingObject defaultMeasuredRect = makeDraftingObject("default_measured_rect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.0, 0.0}, 10.0, 5.0});
    auto defaultObjectDimensions = measureObjectDimensions(defaultMeasuredRect);
    assert(defaultObjectDimensions.ok);
    assert(defaultObjectDimensions.value.width.unit == MeasurementUnit::CanvasUnit);
    assert(defaultObjectDimensions.value.width.value == 10.0);

    DraftingObject invalidMeasuredRect = measuredRect;
    invalidMeasuredRect.metadata.measurement.canvasUnitsPerRealUnit = 0.0;
    auto invalidObjectArea = measureObjectArea(invalidMeasuredRect);
    assert(!invalidObjectArea.ok);
    assert(invalidObjectArea.code == DraftingResultCode::InvalidMetadata);
    auto invalidSummary = summarizeObjectMeasurement(invalidMeasuredRect);
    assert(!invalidSummary.ok);
    assert(invalidSummary.code == DraftingResultCode::InvalidMetadata);

    auto rectDistance = measureObjectDistance(measuredRect);
    assert(!rectDistance.ok);
    assert(rectDistance.code == DraftingResultCode::InvalidGeometry);

    DraftingObject measuredPoint = makeDraftingObject("measured_point", DraftingShapeKind::Point, PointGeometry{{2.0, 3.0}});
    auto pointSummary = summarizeObjectMeasurement(measuredPoint);
    assert(pointSummary.ok);
    assert(!pointSummary.value.hasDistance);
    assert(!pointSummary.value.hasArea);
    assert(pointSummary.value.dimensions.width.value == 0.0);
    assert(pointSummary.value.dimensions.height.value == 0.0);

    return 0;
}
