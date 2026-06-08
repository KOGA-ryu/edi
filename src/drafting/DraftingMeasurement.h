#pragma once

#include "drafting/DraftingTypes.h"

#include <string>

namespace edi::drafting {

struct DraftingObject;

struct ScaleCalibration {
    double canvasUnitsPerRealUnit = 1.0;
    MeasurementUnit realUnit = MeasurementUnit::CanvasUnit;
};

struct MeasurementCalibrationResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    ScaleCalibration calibration;

    static MeasurementCalibrationResult accepted(ScaleCalibration calibration);
    static MeasurementCalibrationResult rejected(DraftingResultCode code, std::string message);
};

enum class MeasurementKind {
    Distance,
    Area,
    Dimension
};

struct MeasurementValue {
    MeasurementKind kind = MeasurementKind::Distance;
    double value = 0.0;
    MeasurementUnit unit = MeasurementUnit::CanvasUnit;
    std::string label;
};

struct DimensionMeasurement {
    MeasurementValue width;
    MeasurementValue height;
};

struct ObjectMeasurementSummary {
    bool hasDistance = false;
    MeasurementValue distance;
    bool hasArea = false;
    MeasurementValue area;
    DimensionMeasurement dimensions;
};

template <typename T>
struct ObjectMeasurementResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    T value;
};

MeasurementCalibrationResult scaleCalibrationFromMetadataChecked(const MeasurementMetadata &metadata);
ScaleCalibration scaleCalibrationFromMetadata(const MeasurementMetadata &metadata);
MeasurementValue measureDistance(Point2D a, Point2D b, const ScaleCalibration &calibration = {});
MeasurementValue measureArea(const DraftingGeometry &geometry, const ScaleCalibration &calibration = {});
Bounds2D measureDimensions(const DraftingGeometry &geometry);
DimensionMeasurement measureDimensionsTyped(const DraftingGeometry &geometry, const ScaleCalibration &calibration = {});
ObjectMeasurementResult<MeasurementValue> measureObjectDistance(const DraftingObject &object);
ObjectMeasurementResult<MeasurementValue> measureObjectArea(const DraftingObject &object);
ObjectMeasurementResult<DimensionMeasurement> measureObjectDimensions(const DraftingObject &object);
ObjectMeasurementResult<ObjectMeasurementSummary> summarizeObjectMeasurement(const DraftingObject &object);
const char *measurementUnitName(MeasurementUnit unit);

} // namespace edi::drafting
