#include "drafting/DraftingSvgOut.h"

#include "drafting/DraftingPhysicalGeometry.h"

#include <sstream>
#include <string>
#include <vector>

namespace edi::drafting {

namespace {

// Fixed 3-decimal formatting with trailing zeros trimmed, so output is stable
// and compact (e.g. 25.4, 0.5, 100).
std::string formatNumber(double value)
{
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(3);
    stream << value;
    std::string text = stream.str();
    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') {
            text.pop_back();
        }
        if (text.back() == '.') {
            text.pop_back();
        }
    }
    if (text == "-0") {
        text = "0";
    }
    return text;
}

} // namespace

std::string svgFromPlotJob(const DraftingPlotJob &job, const DraftingGridProjection &grid)
{
    const double mmPerUnit = millimetersPerUnit(grid.settings.unit);
    const double widthMm = grid.settings.width * mmPerUnit;
    const double heightMm = grid.settings.height * mmPerUnit;

    // Group stroke segments by pen + color + line style + opacity
    // (first-appearance order): per-object styling means one pen can carry
    // several looks, and each look needs its own <path>. Opacity joins the
    // key for the same reason dash style did — without it the first
    // segment's alpha would silently win for the whole group.
    std::vector<std::string> penOrder;
    std::vector<std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < job.strokeSegments.size(); ++i) {
        const DraftingPlotSegment &keyed = job.strokeSegments[i];
        const std::string pen = keyed.penId + "|" + keyed.strokeColor + "|" + keyed.lineStyle
            + "|" + formatNumber(keyed.opacity);
        std::size_t index = penOrder.size();
        for (std::size_t p = 0; p < penOrder.size(); ++p) {
            if (penOrder[p] == pen) {
                index = p;
                break;
            }
        }
        if (index == penOrder.size()) {
            penOrder.push_back(pen);
            groups.emplace_back();
        }
        groups[index].push_back(i);
    }

    std::ostringstream out;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 "
        << formatNumber(widthMm) << ' ' << formatNumber(heightMm)
        << "\" width=\"" << formatNumber(widthMm) << "mm\" height=\"" << formatNumber(heightMm) << "mm\">\n";

    for (std::size_t g = 0; g < groups.size(); ++g) {
        if (groups[g].empty()) {
            continue;
        }
        const DraftingPlotSegment &first = job.strokeSegments[groups[g].front()];
        out << "  <path fill=\"none\" stroke=\"" << first.strokeColor
            << "\" stroke-width=\"" << formatNumber(first.strokeWidth) << "\"";
        // Dash vocabulary scaled by stroke width, mirroring the canvas pens.
        if (first.lineStyle == "dash") {
            out << " stroke-dasharray=\"" << formatNumber(first.strokeWidth * 4.0)
                << ' ' << formatNumber(first.strokeWidth * 2.0) << "\"";
        } else if (first.lineStyle == "dot") {
            out << " stroke-dasharray=\"" << formatNumber(first.strokeWidth)
                << ' ' << formatNumber(first.strokeWidth * 2.0) << "\"";
        }
        // Fully opaque is SVG's default — emit the attribute only when it
        // says something, keeping existing documents byte-identical.
        if (first.opacity < 1.0) {
            out << " stroke-opacity=\"" << formatNumber(first.opacity) << "\"";
        }
        out << " d=\"";
        bool firstSegment = true;
        for (std::size_t i : groups[g]) {
            const DraftingPlotSegment &segment = job.strokeSegments[i];
            // SVG y is not flipped (top-left origin like the screen).
            const double ax = segment.a.x * widthMm;
            const double ay = segment.a.y * heightMm;
            const double bx = segment.b.x * widthMm;
            const double by = segment.b.y * heightMm;
            if (!firstSegment) {
                out << ' ';
            }
            out << 'M' << formatNumber(ax) << ',' << formatNumber(ay)
                << " L" << formatNumber(bx) << ',' << formatNumber(by);
            firstSegment = false;
        }
        out << "\"/>\n";
    }

    out << "</svg>\n";
    return out.str();
}

} // namespace edi::drafting
