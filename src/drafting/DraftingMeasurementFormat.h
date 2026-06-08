#pragma once

#include "drafting/DraftingMeasurement.h"

#include <string>
#include <vector>

namespace edi::drafting {

std::string formatMeasurementValue(const MeasurementValue &value);
std::vector<std::string> formatObjectMeasurementSummary(const ObjectMeasurementSummary &summary);

} // namespace edi::drafting
