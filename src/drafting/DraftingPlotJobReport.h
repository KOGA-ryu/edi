#pragma once

#include "drafting/DraftingPlotJob.h"

#include <string>

namespace edi::drafting {

std::string formatDraftingPlotJobReport(const DraftingPlotJob &job);

} // namespace edi::drafting
