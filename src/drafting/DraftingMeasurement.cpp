#include "drafting/DraftingMeasurement.h"

#include "drafting/DraftingGeometry.h"

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

MeasurementValue measureDistance(Point2D a, Point2D b, const ScaleCalibration &calibration)
{
    return {applyScale(distance(a, b), calibration), calibration.realUnit, measurementUnitName(calibration.realUnit)};
}

MeasurementValue measureArea(const DraftingGeometry &geometry, const ScaleCalibration &calibration)
{
    const double scale = calibration.canvasUnitsPerRealUnit == 0.0 ? 1.0 : calibration.canvasUnitsPerRealUnit;
    return {area(geometry) / (scale * scale), calibration.realUnit, measurementUnitName(calibration.realUnit)};
}

Bounds2D measureDimensions(const DraftingGeometry &geometry)
{
    return computeBounds(geometry);
}

const char *measurementUnitName(MeasurementUnit unit)
{
    switch (unit) {
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
