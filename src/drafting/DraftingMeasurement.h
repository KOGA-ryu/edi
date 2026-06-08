#pragma once

#include "drafting/DraftingTypes.h"

#include <string>

namespace edi::drafting {

enum class MeasurementUnit {
    CanvasUnit,
    Millimeter,
    Centimeter,
    Meter,
    Inch,
    Foot
};

struct ScaleCalibration {
    double canvasUnitsPerRealUnit = 1.0;
    MeasurementUnit realUnit = MeasurementUnit::CanvasUnit;
};

struct MeasurementValue {
    double value = 0.0;
    MeasurementUnit unit = MeasurementUnit::CanvasUnit;
    std::string label;
};

MeasurementValue measureDistance(Point2D a, Point2D b, const ScaleCalibration &calibration = {});
MeasurementValue measureArea(const DraftingGeometry &geometry, const ScaleCalibration &calibration = {});
Bounds2D measureDimensions(const DraftingGeometry &geometry);
const char *measurementUnitName(MeasurementUnit unit);

} // namespace edi::drafting
