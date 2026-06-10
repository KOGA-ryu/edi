#pragma once

#include "drafting/DraftingGrid.h"
#include "drafting/DraftingPlotJob.h"

#include <string>

namespace edi::drafting {

// Renders the plot job's stroke segments as an SVG document, one <path> per pen
// group (travel moves excluded). The viewBox is the grid's physical size in
// millimetres; SVG shares the screen's top-left origin so y is not flipped.
// Pure: no Qt.
std::string svgFromPlotJob(const DraftingPlotJob &job, const DraftingGridProjection &grid);

} // namespace edi::drafting
