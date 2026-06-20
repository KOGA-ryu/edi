#include "drafting/DraftingDocument.h"
#include "drafting/DraftingMeasurement.h"

#include "EdiAssert.h"
#include <string>

using namespace edi::drafting;

int main()
{
    ScaleCalibration calibration{2.0, MeasurementUnit::Centimeter};
    MeasurementValue measured = measureDistance({0.0, 0.0}, {0.0, 10.0}, calibration);
    EDI_CHECK(measured.kind == MeasurementKind::Distance);
    EDI_CHECK(measured.value == 5.0);
    EDI_CHECK(measured.unit == MeasurementUnit::Centimeter);
    EDI_CHECK(measured.label == std::string("centimeter"));

    DraftingGeometry rect = RectangleGeometry{{2.0, 3.0}, 10.0, 5.0};
    MeasurementValue measuredArea = measureArea(rect, calibration);
    EDI_CHECK(measuredArea.kind == MeasurementKind::Area);
    EDI_CHECK(measuredArea.value == 12.5);
    EDI_CHECK(measuredArea.unit == MeasurementUnit::Centimeter);

    Bounds2D dimensions = measureDimensions(rect);
    EDI_CHECK(dimensions.x == 2.0);
    EDI_CHECK(dimensions.y == 3.0);
    EDI_CHECK(dimensions.width == 10.0);
    EDI_CHECK(dimensions.height == 5.0);
    DimensionMeasurement typedDimensions = measureDimensionsTyped(rect, calibration);
    EDI_CHECK(typedDimensions.width.kind == MeasurementKind::Dimension);
    EDI_CHECK(typedDimensions.height.kind == MeasurementKind::Dimension);
    EDI_CHECK(typedDimensions.width.value == 5.0);
    EDI_CHECK(typedDimensions.height.value == 2.5);
    EDI_CHECK(typedDimensions.width.unit == MeasurementUnit::Centimeter);
    EDI_CHECK(typedDimensions.height.unit == MeasurementUnit::Centimeter);

    DraftingGeometry line = LineGeometry{{0.0, 0.0}, {3.0, 4.0}};
    DimensionMeasurement lineDimensions = measureDimensionsTyped(line, calibration);
    EDI_CHECK(lineDimensions.width.value == 1.5);
    EDI_CHECK(lineDimensions.height.value == 2.0);

    DimensionMeasurement defaultDimensions = measureDimensionsTyped(rect);
    EDI_CHECK(defaultDimensions.width.kind == MeasurementKind::Dimension);
    EDI_CHECK(defaultDimensions.width.value == 10.0);
    EDI_CHECK(defaultDimensions.height.value == 5.0);
    EDI_CHECK(defaultDimensions.width.unit == MeasurementUnit::CanvasUnit);

    EDI_CHECK(measurementUnitName(MeasurementUnit::None) == std::string("none"));
    EDI_CHECK(measurementUnitName(MeasurementUnit::CanvasUnit) == std::string("canvas_unit"));
    EDI_CHECK(measurementUnitName(MeasurementUnit::Millimeter) == std::string("millimeter"));
    EDI_CHECK(measurementUnitName(MeasurementUnit::Centimeter) == std::string("centimeter"));
    EDI_CHECK(measurementUnitName(MeasurementUnit::Meter) == std::string("meter"));
    EDI_CHECK(measurementUnitName(MeasurementUnit::Inch) == std::string("inch"));
    EDI_CHECK(measurementUnitName(MeasurementUnit::Foot) == std::string("foot"));

    MeasurementMetadata metadataCalibration;
    metadataCalibration.unit = MeasurementUnit::Inch;
    metadataCalibration.canvasUnitsPerRealUnit = 4.0;
    auto checkedFromMetadata = scaleCalibrationFromMetadataChecked(metadataCalibration);
    EDI_CHECK(checkedFromMetadata.ok);
    EDI_CHECK(checkedFromMetadata.code == DraftingResultCode::None);
    EDI_CHECK(checkedFromMetadata.calibration.realUnit == MeasurementUnit::Inch);
    EDI_CHECK(checkedFromMetadata.calibration.canvasUnitsPerRealUnit == 4.0);
    ScaleCalibration fromMetadata = scaleCalibrationFromMetadata(metadataCalibration);
    EDI_CHECK(fromMetadata.realUnit == MeasurementUnit::Inch);
    EDI_CHECK(fromMetadata.canvasUnitsPerRealUnit == 4.0);
    MeasurementValue metadataMeasured = measureDistance({0.0, 0.0}, {0.0, 12.0}, fromMetadata);
    EDI_CHECK(metadataMeasured.kind == MeasurementKind::Distance);
    EDI_CHECK(metadataMeasured.value == 3.0);
    EDI_CHECK(metadataMeasured.unit == MeasurementUnit::Inch);

    ScaleCalibration defaultFromMetadata = scaleCalibrationFromMetadata(MeasurementMetadata{});
    EDI_CHECK(defaultFromMetadata.realUnit == MeasurementUnit::CanvasUnit);
    EDI_CHECK(defaultFromMetadata.canvasUnitsPerRealUnit == 1.0);
    auto checkedDefaultFromMetadata = scaleCalibrationFromMetadataChecked(MeasurementMetadata{});
    EDI_CHECK(checkedDefaultFromMetadata.ok);
    EDI_CHECK(checkedDefaultFromMetadata.calibration.realUnit == MeasurementUnit::CanvasUnit);
    EDI_CHECK(checkedDefaultFromMetadata.calibration.canvasUnitsPerRealUnit == 1.0);

    MeasurementMetadata invalidCalibration;
    invalidCalibration.unit = MeasurementUnit::Foot;
    invalidCalibration.canvasUnitsPerRealUnit = 0.0;
    auto checkedInvalidCalibration = scaleCalibrationFromMetadataChecked(invalidCalibration);
    EDI_CHECK(!checkedInvalidCalibration.ok);
    EDI_CHECK(checkedInvalidCalibration.code == DraftingResultCode::InvalidMetadata);
    ScaleCalibration fallbackFromInvalid = scaleCalibrationFromMetadata(invalidCalibration);
    EDI_CHECK(fallbackFromInvalid.realUnit == MeasurementUnit::CanvasUnit);
    EDI_CHECK(fallbackFromInvalid.canvasUnitsPerRealUnit == 1.0);

    MeasurementMetadata invalidLabelCalibration;
    invalidLabelCalibration.unit = MeasurementUnit::Foot;
    invalidLabelCalibration.canvasUnitsPerRealUnit = 2.0;
    invalidLabelCalibration.label = "bad\nlabel";
    auto checkedInvalidLabelCalibration = scaleCalibrationFromMetadataChecked(invalidLabelCalibration);
    EDI_CHECK(!checkedInvalidLabelCalibration.ok);
    EDI_CHECK(checkedInvalidLabelCalibration.code == DraftingResultCode::InvalidMetadata);

    DraftingObject measuredLine = makeDraftingObject("measured_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.0, 12.0}});
    measuredLine.metadata.measurement.unit = MeasurementUnit::Inch;
    measuredLine.metadata.measurement.canvasUnitsPerRealUnit = 4.0;
    auto objectDistance = measureObjectDistance(measuredLine);
    EDI_CHECK(objectDistance.ok);
    EDI_CHECK(objectDistance.value.kind == MeasurementKind::Distance);
    EDI_CHECK(objectDistance.value.value == 3.0);
    EDI_CHECK(objectDistance.value.unit == MeasurementUnit::Inch);
    auto lineSummary = summarizeObjectMeasurement(measuredLine);
    EDI_CHECK(lineSummary.ok);
    EDI_CHECK(lineSummary.value.hasDistance);
    EDI_CHECK(lineSummary.value.distance.value == 3.0);
    EDI_CHECK(!lineSummary.value.hasArea);
    EDI_CHECK(lineSummary.value.dimensions.width.value == 0.0);
    EDI_CHECK(lineSummary.value.dimensions.height.value == 3.0);

    DraftingObject measuredDimension = makeDraftingObject("measured_dimension", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {3.0, 4.0}, 0.25});
    auto dimensionDistance = measureObjectDistance(measuredDimension);
    EDI_CHECK(dimensionDistance.ok);
    EDI_CHECK(dimensionDistance.value.kind == MeasurementKind::Distance);
    EDI_CHECK(dimensionDistance.value.value == 5.0);
    auto dimensionSummary = summarizeObjectMeasurement(measuredDimension);
    EDI_CHECK(dimensionSummary.ok);
    EDI_CHECK(dimensionSummary.value.hasDistance);
    EDI_CHECK(dimensionSummary.value.distance.value == 5.0);
    EDI_CHECK(!dimensionSummary.value.hasArea);

    DraftingObject measuredRect = makeDraftingObject("measured_rect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.0, 0.0}, 10.0, 5.0});
    measuredRect.metadata.measurement.unit = MeasurementUnit::Centimeter;
    measuredRect.metadata.measurement.canvasUnitsPerRealUnit = 2.0;
    auto objectArea = measureObjectArea(measuredRect);
    EDI_CHECK(objectArea.ok);
    EDI_CHECK(objectArea.value.kind == MeasurementKind::Area);
    EDI_CHECK(objectArea.value.value == 12.5);
    EDI_CHECK(objectArea.value.unit == MeasurementUnit::Centimeter);
    auto objectDimensions = measureObjectDimensions(measuredRect);
    EDI_CHECK(objectDimensions.ok);
    EDI_CHECK(objectDimensions.value.width.value == 5.0);
    EDI_CHECK(objectDimensions.value.height.value == 2.5);
    auto rectSummary = summarizeObjectMeasurement(measuredRect);
    EDI_CHECK(rectSummary.ok);
    EDI_CHECK(!rectSummary.value.hasDistance);
    EDI_CHECK(rectSummary.value.hasArea);
    EDI_CHECK(rectSummary.value.area.value == 12.5);
    EDI_CHECK(rectSummary.value.dimensions.width.value == 5.0);
    EDI_CHECK(rectSummary.value.dimensions.height.value == 2.5);

    DraftingObject defaultMeasuredRect = makeDraftingObject("default_measured_rect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.0, 0.0}, 10.0, 5.0});
    auto defaultObjectDimensions = measureObjectDimensions(defaultMeasuredRect);
    EDI_CHECK(defaultObjectDimensions.ok);
    EDI_CHECK(defaultObjectDimensions.value.width.unit == MeasurementUnit::CanvasUnit);
    EDI_CHECK(defaultObjectDimensions.value.width.value == 10.0);

    DraftingObject invalidMeasuredRect = measuredRect;
    invalidMeasuredRect.metadata.measurement.canvasUnitsPerRealUnit = 0.0;
    auto invalidObjectArea = measureObjectArea(invalidMeasuredRect);
    EDI_CHECK(!invalidObjectArea.ok);
    EDI_CHECK(invalidObjectArea.code == DraftingResultCode::InvalidMetadata);
    auto invalidSummary = summarizeObjectMeasurement(invalidMeasuredRect);
    EDI_CHECK(!invalidSummary.ok);
    EDI_CHECK(invalidSummary.code == DraftingResultCode::InvalidMetadata);

    auto rectDistance = measureObjectDistance(measuredRect);
    EDI_CHECK(!rectDistance.ok);
    EDI_CHECK(rectDistance.code == DraftingResultCode::InvalidGeometry);

    DraftingObject measuredPoint = makeDraftingObject("measured_point", DraftingShapeKind::Point, PointGeometry{{2.0, 3.0}});
    auto pointSummary = summarizeObjectMeasurement(measuredPoint);
    EDI_CHECK(pointSummary.ok);
    EDI_CHECK(!pointSummary.value.hasDistance);
    EDI_CHECK(!pointSummary.value.hasArea);
    EDI_CHECK(pointSummary.value.dimensions.width.value == 0.0);
    EDI_CHECK(pointSummary.value.dimensions.height.value == 0.0);

    return 0;
}
