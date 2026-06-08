#include "drafting/DraftingMeasurementFormat.h"

#include <sstream>

namespace edi::drafting {

std::string formatMeasurementValue(const MeasurementValue &value)
{
    std::ostringstream stream;
    stream << value.value << ' ';
    if (value.kind == MeasurementKind::Area) {
        stream << "square ";
    }
    stream << value.label;
    return stream.str();
}

std::vector<std::string> formatObjectMeasurementSummary(const ObjectMeasurementSummary &summary)
{
    std::vector<std::string> lines;
    if (summary.hasDistance) {
        lines.push_back("distance: " + formatMeasurementValue(summary.distance));
    }
    if (summary.hasArea) {
        lines.push_back("area: " + formatMeasurementValue(summary.area));
    }
    lines.push_back("width: " + formatMeasurementValue(summary.dimensions.width));
    lines.push_back("height: " + formatMeasurementValue(summary.dimensions.height));
    return lines;
}

} // namespace edi::drafting
