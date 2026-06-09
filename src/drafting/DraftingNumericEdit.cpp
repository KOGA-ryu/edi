#include "drafting/DraftingNumericEdit.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <type_traits>
#include <utility>

namespace edi::drafting {

namespace {

constexpr double kPi = 3.14159265358979323846;

double degreesToRadians(double degrees)
{
    return degrees * kPi / 180.0;
}

double lineLength(const LineGeometry &line)
{
    return distance(line.a, line.b);
}

DraftingNumericEditResult acceptedIfValid(DraftingGeometry geometry)
{
    const GeometryValidationResult validation = validateGeometry(geometry);
    if (!validation.ok) {
        return DraftingNumericEditResult::rejected(validation.code, validation.message);
    }
    return DraftingNumericEditResult::accepted(std::move(geometry));
}

} // namespace

DraftingNumericEditResult DraftingNumericEditResult::accepted(DraftingGeometry geometry)
{
    DraftingNumericEditResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = std::move(geometry);
    return result;
}

DraftingNumericEditResult DraftingNumericEditResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingNumericEditResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

double lineAngleDegrees(const LineGeometry &line)
{
    return std::atan2(line.b.y - line.a.y, line.b.x - line.a.x) * 180.0 / kPi;
}

DraftingNumericEditResult applyNumericGeometryEdit(
    const DraftingObject &object,
    const std::string &fieldId,
    double value)
{
    if (fieldId.empty()) {
        return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field id is empty");
    }
    if (!std::isfinite(value)) {
        return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric edit value must be finite");
    }
    if (!kindMatchesGeometry(object.kind, object.geometry)) {
        return DraftingNumericEditResult::rejected(DraftingResultCode::KindGeometryMismatch, "shape kind does not match geometry");
    }

    return std::visit([&](auto geometry) -> DraftingNumericEditResult {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            if (fieldId == "x") {
                geometry.point.x = value;
            } else if (fieldId == "y") {
                geometry.point.y = value;
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to point");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            if (fieldId == "x1") {
                geometry.a.x = value;
            } else if (fieldId == "y1") {
                geometry.a.y = value;
            } else if (fieldId == "x2") {
                geometry.b.x = value;
            } else if (fieldId == "y2") {
                geometry.b.y = value;
            } else if (fieldId == "line_length") {
                if (value < 0.0) {
                    return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "line length must be non-negative");
                }
                const double angle = degreesToRadians(lineAngleDegrees(geometry));
                geometry.b = {
                    geometry.a.x + std::cos(angle) * value,
                    geometry.a.y + std::sin(angle) * value,
                };
            } else if (fieldId == "line_angle_deg") {
                const double length = lineLength(geometry);
                const double angle = degreesToRadians(value);
                geometry.b = {
                    geometry.a.x + std::cos(angle) * length,
                    geometry.a.y + std::sin(angle) * length,
                };
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to line");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            if (fieldId == "x") {
                geometry.origin.x = value;
            } else if (fieldId == "y") {
                geometry.origin.y = value;
            } else if (fieldId == "width") {
                geometry.width = value;
            } else if (fieldId == "height") {
                geometry.height = value;
            } else if (fieldId == "rotation_deg") {
                geometry.rotationDeg = value;
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to rectangle");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            if (fieldId == "cx") {
                geometry.center.x = value;
            } else if (fieldId == "cy") {
                geometry.center.y = value;
            } else if (fieldId == "radius") {
                geometry.radius = value;
            } else if (fieldId == "diameter") {
                if (value < 0.0) {
                    return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "circle diameter must be non-negative");
                }
                geometry.radius = value / 2.0;
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to circle");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (fieldId == "position") {
                geometry.position = value;
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to guide");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            if (fieldId == "x1") {
                geometry.a.x = value;
            } else if (fieldId == "y1") {
                geometry.a.y = value;
            } else if (fieldId == "x2") {
                geometry.b.x = value;
            } else if (fieldId == "y2") {
                geometry.b.y = value;
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to construction line");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            if (fieldId == "x1") {
                geometry.a.x = value;
            } else if (fieldId == "y1") {
                geometry.a.y = value;
            } else if (fieldId == "x2") {
                geometry.b.x = value;
            } else if (fieldId == "y2") {
                geometry.b.y = value;
            } else if (fieldId == "offset") {
                geometry.offset = value;
            } else {
                return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to dimension");
            }
            return acceptedIfValid(DraftingGeometry{geometry});
        } else {
            return DraftingNumericEditResult::rejected(DraftingResultCode::InvalidGeometry, "numeric field does not apply to this geometry");
        }
    }, object.geometry);
}

} // namespace edi::drafting
