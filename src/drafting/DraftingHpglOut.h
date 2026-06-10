#pragma once

#include "drafting/DraftingGrid.h"
#include "drafting/DraftingPlotJob.h"

#include <string>

namespace edi::drafting {

struct DraftingHpglSettings {
    double plotterUnitsPerMm = 40.0;
    bool returnHome = true;
};

// Emits HPGL for the plot job's stroke segments, grouped by pen (SP<n> per pen,
// indexed by first appearance in penStats). Coordinates are normalized ->
// physical mm -> plotter units, rounded to integers, with the y axis flipped
// (HPGL origin is bottom-left). Pure: no Qt.
std::string hpglFromPlotJob(const DraftingPlotJob &job,
                            const DraftingGridProjection &grid,
                            const DraftingHpglSettings &settings = {});

} // namespace edi::drafting
