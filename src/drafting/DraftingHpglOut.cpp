#include "drafting/DraftingHpglOut.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace edi::drafting {

namespace {

struct PlotterPoint {
    long x = 0;
    long y = 0;
    bool operator==(const PlotterPoint &other) const { return x == other.x && y == other.y; }
};

} // namespace

std::string hpglFromPlotJob(const DraftingPlotJob &job,
                            const DraftingGridProjection &grid,
                            const DraftingHpglSettings &settings)
{
    const double mmPerUnit = millimetersPerUnit(grid.settings.unit);
    const double widthMm = grid.settings.width * mmPerUnit;
    const double heightMm = grid.settings.height * mmPerUnit;
    const double scale = settings.plotterUnitsPerMm;

    // Normalized -> physical mm -> plotter units, y flipped (origin bottom-left).
    auto toPlotter = [&](Point2D p) -> PlotterPoint {
        const double xMm = p.x * widthMm;
        const double yMm = p.y * heightMm;
        return {
            std::lround(xMm * scale),
            std::lround((heightMm - yMm) * scale),
        };
    };

    // Pen index by first appearance in penStats (1-based, SP0 lifts the pen).
    auto penIndex = [&](const std::string &penId) -> int {
        for (std::size_t i = 0; i < job.penStats.size(); ++i) {
            if (job.penStats[i].penId == penId) {
                return static_cast<int>(i) + 1;
            }
        }
        return 1;
    };

    // Distinct pens that actually carry stroke segments, in penStats order.
    std::vector<std::string> pensWithSegments;
    for (const DraftingPlotPenStats &pen : job.penStats) {
        for (const DraftingPlotSegment &segment : job.strokeSegments) {
            if (segment.penId == pen.penId) {
                pensWithSegments.push_back(pen.penId);
                break;
            }
        }
    }

    std::ostringstream out;
    out << "IN;\n";

    for (const std::string &pen : pensWithSegments) {
        out << "SP" << penIndex(pen) << ";\n";

        // Chain consecutive segments of this pen that share endpoints.
        std::vector<PlotterPoint> chain;
        auto flush = [&]() {
            if (chain.size() < 2) {
                chain.clear();
                return;
            }
            out << "PU" << chain.front().x << ',' << chain.front().y << ";\n";
            out << "PD";
            for (std::size_t i = 1; i < chain.size(); ++i) {
                out << (i == 1 ? "" : ",") << chain[i].x << ',' << chain[i].y;
            }
            out << ";\n";
            chain.clear();
        };

        for (const DraftingPlotSegment &segment : job.strokeSegments) {
            if (segment.penId != pen) {
                continue;
            }
            const PlotterPoint a = toPlotter(segment.a);
            const PlotterPoint b = toPlotter(segment.b);
            if (!chain.empty() && chain.back() == a) {
                chain.push_back(b);
            } else {
                flush();
                chain.push_back(a);
                chain.push_back(b);
            }
        }
        flush();
    }

    out << "PU;\n";
    out << "SP0;\n";
    if (settings.returnHome) {
        out << "IN;\n";
    }
    return out.str();
}

} // namespace edi::drafting
