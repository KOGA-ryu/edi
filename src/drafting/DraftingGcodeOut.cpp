#include "drafting/DraftingGcodeOut.h"

#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace edi::drafting {

namespace {

// Fixed 3-decimal mm. Deterministic across platforms (no %g locale/precision
// surprises), which is what lets the output be golden-tested line by line.
std::string fmt(double value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3f", value);
    return buffer;
}

struct MachinePoint {
    double x = 0.0;
    double y = 0.0;
    bool operator==(const MachinePoint &other) const
    {
        return std::abs(x - other.x) < 1e-9 && std::abs(y - other.y) < 1e-9;
    }
};

} // namespace

std::string gcodeFromPlotJob(const DraftingPlotJob &job,
                             const DraftingGridProjection &grid,
                             const DraftingGcodeSettings &settings)
{
    const double mmPerUnit = millimetersPerUnit(grid.settings.unit);
    const double widthMm = grid.settings.width * mmPerUnit;
    const double heightMm = grid.settings.height * mmPerUnit;

    // Normalized -> physical mm, y flipped (machine origin bottom-left).
    auto toMachine = [&](Point2D p) -> MachinePoint {
        return {p.x * widthMm, heightMm - p.y * heightMm};
    };

    auto penIndex = [&](const std::string &penId) -> int {
        for (std::size_t i = 0; i < job.penStats.size(); ++i) {
            if (job.penStats[i].penId == penId) {
                return static_cast<int>(i) + 1;
            }
        }
        return 1;
    };

    // Pens that actually carry stroke segments, in penStats order.
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
    out << "G21\n"; // millimeters
    out << "G90\n"; // absolute coordinates

    // The pen-up / pen-down commands depend on the mode, but the travel-vs-
    // stroke STRUCTURE (G0 to start, lower, G1 draws, lift) is identical —
    // so the mode is a pair of strings, not a second emit path.
    const bool spindle = settings.penMode == DraftingGcodePenMode::Spindle;
    const std::string penUp = spindle ? "M5\n" : ("G0 Z" + fmt(settings.penUpZ) + "\n");
    const std::string penDown = spindle ? "M3\n"
        : ("G1 Z" + fmt(settings.penDownZ) + " F" + fmt(settings.feedRate) + "\n");

    out << penUp; // start clear of the page

    for (const std::string &pen : pensWithSegments) {
        out << 'T' << penIndex(pen) << " M6\n"; // tool change per pen

        std::vector<MachinePoint> chain;
        auto flush = [&]() {
            if (chain.size() < 2) {
                chain.clear();
                return;
            }
            // Rapid to the chain start with the pen up, lower, draw the rest,
            // lift. G1 feed is modal, so it is set once on the pen-down move.
            out << "G0 X" << fmt(chain.front().x) << " Y" << fmt(chain.front().y) << '\n';
            out << penDown;
            for (std::size_t i = 1; i < chain.size(); ++i) {
                out << "G1 X" << fmt(chain[i].x) << " Y" << fmt(chain[i].y) << '\n';
            }
            out << penUp;
            chain.clear();
        };

        for (const DraftingPlotSegment &segment : job.strokeSegments) {
            if (segment.penId != pen) {
                continue;
            }
            const MachinePoint a = toMachine(segment.a);
            const MachinePoint b = toMachine(segment.b);
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

    if (settings.returnHome) {
        out << "G0 X0.000 Y0.000\n";
    }
    out << "M2\n"; // program end
    return out.str();
}

} // namespace edi::drafting
