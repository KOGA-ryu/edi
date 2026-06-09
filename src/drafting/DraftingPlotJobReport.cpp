#include "drafting/DraftingPlotJobReport.h"

#include <iomanip>
#include <sstream>

namespace edi::drafting {
namespace {

const char *readyText(bool ready)
{
    return ready ? "true" : "false";
}

std::string numberText(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
}

std::string boundsText(Bounds2D bounds)
{
    return "x=" + numberText(bounds.x)
        + " y=" + numberText(bounds.y)
        + " w=" + numberText(bounds.width)
        + " h=" + numberText(bounds.height);
}

} // namespace

std::string formatDraftingPlotJobReport(const DraftingPlotJob &job)
{
    std::ostringstream out;
    out << "edi_plot_job_report\n";
    out << "status: " << (job.ready ? "ready" : "blocked") << '\n';
    out << "calibration_scale: " << numberText(job.calibrationScale) << '\n';
    out << "order_mode: " << draftingPlotOrderModeName(job.orderMode) << '\n';
    out << "direction_mode: " << draftingPlotDirectionModeName(job.directionMode) << '\n';
    out << "plot_bounds: " << (job.hasPlotBounds ? boundsText(job.plotBounds) : "none") << '\n';
    out << "stroke_segments: " << job.strokeSegments.size() << '\n';
    out << "travel_segments: " << job.travelSegments.size() << '\n';
    out << "travel_distance: " << numberText(job.travelDistance) << '\n';
    out << "warnings: " << job.warnings.size() << '\n';
    for (const DraftingPlotWarning &warning : job.warnings) {
        out << "- warning object=" << warning.objectId
            << " kind=" << warning.kind
            << " message=\"" << warning.message << "\"\n";
    }
    out << "blocked_reasons: " << job.blockedReasons.size() << '\n';
    for (const std::string &reason : job.blockedReasons) {
        out << "- reason=" << reason << '\n';
    }
    out << "layers: " << job.layerStats.size() << '\n';
    for (const DraftingPlotLayerStats &stats : job.layerStats) {
        out << "- layer id=" << stats.layerId
            << " ready=" << readyText(stats.ready)
            << " objects=" << stats.objectCount
            << " segments=" << stats.segmentCount
            << " stroke=" << numberText(stats.strokeDistance)
            << " travel=" << numberText(stats.travelDistance)
            << " reason=" << stats.blockedReason << '\n';
    }
    out << "pens: " << job.penStats.size() << '\n';
    for (const DraftingPlotPenStats &stats : job.penStats) {
        out << "- pen id=" << stats.penId
            << " ready=" << readyText(stats.ready)
            << " objects=" << stats.objectCount
            << " segments=" << stats.segmentCount
            << " stroke=" << numberText(stats.strokeDistance)
            << " travel=" << numberText(stats.travelDistance)
            << " reason=" << stats.blockedReason << '\n';
    }
    return out.str();
}

} // namespace edi::drafting
