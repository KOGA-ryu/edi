#pragma once

#include "drafting/DraftingGrid.h"
#include "drafting/DraftingPlotJob.h"

#include <string>

namespace edi::drafting {

// How the pen is raised/lowered between travel and stroke.
enum class DraftingGcodePenMode {
    ZAxis,   // G0 Z<up> lifts, G1 Z<down> lowers (a Z-driven pen holder)
    Spindle, // M5 lifts, M3 lowers (a marker/laser switched by spindle on/off)
};

struct DraftingGcodeSettings {
    double feedRate = 1500.0; // mm/min for G1 stroke moves
    double penUpZ = 5.0;      // Z height clear of the page (ZAxis mode)
    double penDownZ = 0.0;    // Z height on the page (ZAxis mode)
    bool returnHome = true;   // G0 X0 Y0 before M2
    DraftingGcodePenMode penMode = DraftingGcodePenMode::ZAxis;
};

// Emits CNC G-code for the plot job's stroke segments, the G-code sibling of
// hpglFromPlotJob. G0 = rapid travel (pen up), G1 = stroke (pen down); each
// pen becomes a tool change (T<n> M6, n = penStats order). Coordinates are
// normalized -> physical mm, absolute (G90), y flipped so the page origin is
// bottom-left, fixed 3 decimals. Pure: no Qt.
std::string gcodeFromPlotJob(const DraftingPlotJob &job,
                             const DraftingGridProjection &grid,
                             const DraftingGcodeSettings &settings = {});

} // namespace edi::drafting
