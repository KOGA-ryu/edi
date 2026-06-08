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
    assert(formatMeasurementValue(measured) == std::string("5 centimeter"));

    DraftingGeometry rect = RectangleGeometry{{2.0, 3.0}, 10.0, 5.0};
    MeasurementValue measuredArea = measureArea(rect, calibration);
    assert(measuredArea.kind == MeasurementKind::Area);
    assert(measuredArea.value == 12.5);
    assert(measuredArea.unit == MeasurementUnit::Centimeter);
    assert(formatMeasurementValue(measuredArea) == std::string("12.5 square centimeter"));

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
    assert(formatMeasurementValue(typedDimensions.width) == std::string("5 centimeter"));

    DraftingGeometry line = LineGeometry{{0.0, 0.0}, {3.0, 4.0}};
    DimensionMeasurement lineDimensions = measureDimensionsTyped(line, calibration);
    assert(lineDimensions.width.value == 1.5);
    assert(lineDimensions.height.value == 2.0);

    DimensionMeasurement defaultDimensions = measureDimensionsTyped(rect);
    assert(defaultDimensions.width.kind == MeasurementKind::Dimension);
    assert(defaultDimensions.width.value == 10.0);
    assert(defaultDimensions.height.value == 5.0);
    assert(defaultDimensions.width.unit == MeasurementUnit::CanvasUnit);
    assert(formatMeasurementValue(defaultDimensions.width) == std::string("10 canvas_unit"));

    MeasurementValue noMeasurement;
    noMeasurement.kind = MeasurementKind::Distance;
    noMeasurement.unit = MeasurementUnit::None;
    noMeasurement.label = measurementUnitName(MeasurementUnit::None);
    assert(formatMeasurementValue(noMeasurement) == std::string("0 none"));

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

    return 0;
}
