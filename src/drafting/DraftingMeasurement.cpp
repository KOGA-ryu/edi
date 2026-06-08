#include "drafting/DraftingMeasurement.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMetadata.h"

#include <utility>

namespace edi::drafting {

namespace {

double applyScale(double canvasValue, const ScaleCalibration &calibration)
{
    if (calibration.canvasUnitsPerRealUnit == 0.0) {
        return canvasValue;
    }
    return canvasValue / calibration.canvasUnitsPerRealUnit;
}

} // namespace

MeasurementCalibrationResult MeasurementCalibrationResult::accepted(ScaleCalibration calibration)
{
    MeasurementCalibrationResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.calibration = calibration;
    return result;
}

MeasurementCalibrationResult MeasurementCalibrationResult::rejected(DraftingResultCode code, std::string message)
{
    MeasurementCalibrationResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

MeasurementCalibrationResult scaleCalibrationFromMetadataChecked(const MeasurementMetadata &metadata)
{
    if (!isValidMeasurementMetadata(metadata)) {
        return MeasurementCalibrationResult::rejected(DraftingResultCode::InvalidMetadata, "measurement metadata is invalid");
    }
    if (metadata.unit == MeasurementUnit::None) {
        return MeasurementCalibrationResult::accepted({});
    }
    return MeasurementCalibrationResult::accepted({metadata.canvasUnitsPerRealUnit, metadata.unit});
}

ScaleCalibration scaleCalibrationFromMetadata(const MeasurementMetadata &metadata)
{
    const auto result = scaleCalibrationFromMetadataChecked(metadata);
    return result.ok ? result.calibration : ScaleCalibration{};
}

MeasurementValue measureDistance(Point2D a, Point2D b, const ScaleCalibration &calibration)
{
    return {
        MeasurementKind::Distance,
        applyScale(distance(a, b), calibration),
        calibration.realUnit,
        measurementUnitName(calibration.realUnit),
    };
}

MeasurementValue measureArea(const DraftingGeometry &geometry, const ScaleCalibration &calibration)
{
    const double scale = calibration.canvasUnitsPerRealUnit == 0.0 ? 1.0 : calibration.canvasUnitsPerRealUnit;
    return {
        MeasurementKind::Area,
        area(geometry) / (scale * scale),
        calibration.realUnit,
        measurementUnitName(calibration.realUnit),
    };
}

Bounds2D measureDimensions(const DraftingGeometry &geometry)
{
    return computeBounds(geometry);
}

const char *measurementUnitName(MeasurementUnit unit)
{
    switch (unit) {
    case MeasurementUnit::None:
        return "none";
    case MeasurementUnit::CanvasUnit:
        return "canvas_unit";
    case MeasurementUnit::Millimeter:
        return "millimeter";
    case MeasurementUnit::Centimeter:
        return "centimeter";
    case MeasurementUnit::Meter:
        return "meter";
    case MeasurementUnit::Inch:
        return "inch";
    case MeasurementUnit::Foot:
        return "foot";
    }
    return "unknown";
}

} // namespace edi::drafting
