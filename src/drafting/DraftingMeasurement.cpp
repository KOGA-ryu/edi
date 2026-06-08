#include "drafting/DraftingMeasurement.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMetadata.h"

#include <utility>
#include <variant>

namespace edi::drafting {

namespace {

double applyScale(double canvasValue, const ScaleCalibration &calibration)
{
    if (calibration.canvasUnitsPerRealUnit == 0.0) {
        return canvasValue;
    }
    return canvasValue / calibration.canvasUnitsPerRealUnit;
}

template <typename T>
ObjectMeasurementResult<T> acceptedObjectMeasurement(T value)
{
    ObjectMeasurementResult<T> result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.value = std::move(value);
    return result;
}

template <typename T>
ObjectMeasurementResult<T> rejectedObjectMeasurement(DraftingResultCode code, std::string message)
{
    ObjectMeasurementResult<T> result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

bool geometrySupportsSummaryArea(const DraftingGeometry &geometry)
{
    return std::holds_alternative<RectangleGeometry>(geometry)
        || std::holds_alternative<CircleGeometry>(geometry)
        || std::holds_alternative<PolygonGeometry>(geometry);
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

DimensionMeasurement measureDimensionsTyped(const DraftingGeometry &geometry, const ScaleCalibration &calibration)
{
    const Bounds2D bounds = measureDimensions(geometry);
    const char *label = measurementUnitName(calibration.realUnit);
    return {
        {
            MeasurementKind::Dimension,
            applyScale(bounds.width, calibration),
            calibration.realUnit,
            label,
        },
        {
            MeasurementKind::Dimension,
            applyScale(bounds.height, calibration),
            calibration.realUnit,
            label,
        },
    };
}

ObjectMeasurementResult<MeasurementValue> measureObjectDistance(const DraftingObject &object)
{
    const auto calibration = scaleCalibrationFromMetadataChecked(object.metadata.measurement);
    if (!calibration.ok) {
        return rejectedObjectMeasurement<MeasurementValue>(calibration.code, calibration.message);
    }

    const auto *line = std::get_if<LineGeometry>(&object.geometry);
    if (line == nullptr) {
        return rejectedObjectMeasurement<MeasurementValue>(
            DraftingResultCode::InvalidGeometry,
            "object geometry does not define a distance");
    }

    return acceptedObjectMeasurement(measureDistance(line->a, line->b, calibration.calibration));
}

ObjectMeasurementResult<MeasurementValue> measureObjectArea(const DraftingObject &object)
{
    const auto calibration = scaleCalibrationFromMetadataChecked(object.metadata.measurement);
    if (!calibration.ok) {
        return rejectedObjectMeasurement<MeasurementValue>(calibration.code, calibration.message);
    }
    return acceptedObjectMeasurement(measureArea(object.geometry, calibration.calibration));
}

ObjectMeasurementResult<DimensionMeasurement> measureObjectDimensions(const DraftingObject &object)
{
    const auto calibration = scaleCalibrationFromMetadataChecked(object.metadata.measurement);
    if (!calibration.ok) {
        return rejectedObjectMeasurement<DimensionMeasurement>(calibration.code, calibration.message);
    }
    return acceptedObjectMeasurement(measureDimensionsTyped(object.geometry, calibration.calibration));
}

ObjectMeasurementResult<ObjectMeasurementSummary> summarizeObjectMeasurement(const DraftingObject &object)
{
    const auto calibration = scaleCalibrationFromMetadataChecked(object.metadata.measurement);
    if (!calibration.ok) {
        return rejectedObjectMeasurement<ObjectMeasurementSummary>(calibration.code, calibration.message);
    }

    ObjectMeasurementSummary summary;
    summary.dimensions = measureDimensionsTyped(object.geometry, calibration.calibration);

    if (const auto *line = std::get_if<LineGeometry>(&object.geometry)) {
        summary.hasDistance = true;
        summary.distance = measureDistance(line->a, line->b, calibration.calibration);
    }

    if (geometrySupportsSummaryArea(object.geometry)) {
        summary.hasArea = true;
        summary.area = measureArea(object.geometry, calibration.calibration);
    }

    return acceptedObjectMeasurement(summary);
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
