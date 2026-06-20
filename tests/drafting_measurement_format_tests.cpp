#include "drafting/DraftingMeasurementFormat.h"

#include "EdiAssert.h"
#include <string>

using namespace edi::drafting;

namespace {

MeasurementValue measurementValue(MeasurementKind kind, double value, MeasurementUnit unit)
{
    return {kind, value, unit, measurementUnitName(unit)};
}

} // namespace

int main()
{
    MeasurementValue distance = measurementValue(MeasurementKind::Distance, 5.0, MeasurementUnit::Centimeter);
    EDI_CHECK(formatMeasurementValue(distance) == std::string("5 centimeter"));

    MeasurementValue area = measurementValue(MeasurementKind::Area, 12.5, MeasurementUnit::Centimeter);
    EDI_CHECK(formatMeasurementValue(area) == std::string("12.5 square centimeter"));

    MeasurementValue dimension = measurementValue(MeasurementKind::Dimension, 5.0, MeasurementUnit::Centimeter);
    EDI_CHECK(formatMeasurementValue(dimension) == std::string("5 centimeter"));

    MeasurementValue canvas = measurementValue(MeasurementKind::Dimension, 10.0, MeasurementUnit::CanvasUnit);
    EDI_CHECK(formatMeasurementValue(canvas) == std::string("10 canvas_unit"));

    MeasurementValue none = measurementValue(MeasurementKind::Distance, 0.0, MeasurementUnit::None);
    EDI_CHECK(formatMeasurementValue(none) == std::string("0 none"));

    ObjectMeasurementSummary lineSummary;
    lineSummary.hasDistance = true;
    lineSummary.distance = measurementValue(MeasurementKind::Distance, 3.0, MeasurementUnit::Inch);
    lineSummary.dimensions.width = measurementValue(MeasurementKind::Dimension, 0.0, MeasurementUnit::Inch);
    lineSummary.dimensions.height = measurementValue(MeasurementKind::Dimension, 3.0, MeasurementUnit::Inch);
    auto lineSummaryLines = formatObjectMeasurementSummary(lineSummary);
    EDI_CHECK(lineSummaryLines.size() == 3);
    EDI_CHECK(lineSummaryLines[0] == std::string("distance: 3 inch"));
    EDI_CHECK(lineSummaryLines[1] == std::string("width: 0 inch"));
    EDI_CHECK(lineSummaryLines[2] == std::string("height: 3 inch"));

    ObjectMeasurementSummary rectSummary;
    rectSummary.hasArea = true;
    rectSummary.area = measurementValue(MeasurementKind::Area, 12.5, MeasurementUnit::Centimeter);
    rectSummary.dimensions.width = measurementValue(MeasurementKind::Dimension, 5.0, MeasurementUnit::Centimeter);
    rectSummary.dimensions.height = measurementValue(MeasurementKind::Dimension, 2.5, MeasurementUnit::Centimeter);
    auto rectSummaryLines = formatObjectMeasurementSummary(rectSummary);
    EDI_CHECK(rectSummaryLines.size() == 3);
    EDI_CHECK(rectSummaryLines[0] == std::string("area: 12.5 square centimeter"));
    EDI_CHECK(rectSummaryLines[1] == std::string("width: 5 centimeter"));
    EDI_CHECK(rectSummaryLines[2] == std::string("height: 2.5 centimeter"));

    ObjectMeasurementSummary pointSummary;
    pointSummary.dimensions.width = measurementValue(MeasurementKind::Dimension, 0.0, MeasurementUnit::CanvasUnit);
    pointSummary.dimensions.height = measurementValue(MeasurementKind::Dimension, 0.0, MeasurementUnit::CanvasUnit);
    auto pointSummaryLines = formatObjectMeasurementSummary(pointSummary);
    EDI_CHECK(pointSummaryLines.size() == 2);
    EDI_CHECK(pointSummaryLines[0] == std::string("width: 0 canvas_unit"));
    EDI_CHECK(pointSummaryLines[1] == std::string("height: 0 canvas_unit"));

    return 0;
}
