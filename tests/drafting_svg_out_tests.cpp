#include "drafting/DraftingSvgOut.h"

#include <cassert>
#include <string>

using namespace edi::drafting;

namespace {

DraftingPlotSegment segment(Point2D a, Point2D b, const std::string &penId,
                            const std::string &color, double width)
{
    DraftingPlotSegment s;
    s.objectId = "obj";
    s.layerId = "default";
    s.rawA = a;
    s.rawB = b;
    s.a = a;
    s.b = b;
    s.penId = penId;
    s.strokeColor = color;
    s.strokeWidth = width;
    return s;
}

// 100mm x 100mm page; a black line and a two-segment blue chain on a second pen.
DraftingPlotJob sampleJob()
{
    DraftingPlotJob job;
    job.strokeSegments = {
        segment({0.0, 0.0}, {0.5, 0.0}, "pen_black", "#000000", 2.0),
        segment({1.0, 1.0}, {1.0, 0.5}, "pen_blue", "#0000ff", 1.0),
        segment({1.0, 0.5}, {0.5, 0.5}, "pen_blue", "#0000ff", 1.0),
    };
    job.penStats = {
        {"pen_black", "#000000", 2.0},
        {"pen_blue", "#0000ff", 1.0},
    };
    return job;
}

DraftingGridProjection page()
{
    DraftingGridProjection grid;
    grid.settings.width = 100.0;
    grid.settings.height = 100.0;
    grid.settings.unit = DraftingGridUnit::Millimeter;
    return grid;
}

} // namespace

int main()
{
    const std::string svg = svgFromPlotJob(sampleJob(), page());

    const std::string expected =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\" width=\"100mm\" height=\"100mm\">\n"
        "  <path fill=\"none\" stroke=\"#000000\" stroke-width=\"2\" d=\"M0,0 L50,0\"/>\n"
        "  <path fill=\"none\" stroke=\"#0000ff\" stroke-width=\"1\" d=\"M100,100 L100,50 M100,50 L50,50\"/>\n"
        "</svg>\n";
    assert(svg == expected);

    // Inch units scale the viewBox by 25.4 mm/in.
    {
        DraftingGridProjection inchGrid;
        inchGrid.settings.width = 1.0;
        inchGrid.settings.height = 2.0;
        inchGrid.settings.unit = DraftingGridUnit::Inch;
        const std::string inchSvg = svgFromPlotJob(sampleJob(), inchGrid);
        assert(inchSvg.find("viewBox=\"0 0 25.4 50.8\"") != std::string::npos);
    }

    // An empty job still produces a valid, path-free SVG envelope.
    {
        DraftingPlotJob empty;
        const std::string emptySvg = svgFromPlotJob(empty, page());
        assert(emptySvg.find("<path") == std::string::npos);
        assert(emptySvg.find("<svg") == 0);
    }

    // Per-object styling reaches the SVG: dash becomes a dasharray, and a
    // same-pen segment with a different color/style splits into its own
    // <path> (the group key is pen + color + line style, not pen alone).
    {
        DraftingPlotJob job;
        DraftingPlotSegment dashed = segment({0.1, 0.1}, {0.9, 0.1}, "pen_black", "#ff6600", 3.0);
        dashed.lineStyle = "dash";
        job.strokeSegments = {
            dashed,
            segment({0.1, 0.5}, {0.9, 0.5}, "pen_black", "#d7dde8", 2.0),
        };
        const std::string styledSvg = svgFromPlotJob(job, page());
        assert(styledSvg.find("stroke=\"#ff6600\"") != std::string::npos);
        assert(styledSvg.find("stroke-dasharray=") != std::string::npos);
        assert(styledSvg.find("stroke=\"#d7dde8\"") != std::string::npos);
        // Two looks, two paths — even on one physical pen.
        std::size_t paths = 0;
        for (std::size_t at = styledSvg.find("<path"); at != std::string::npos; at = styledSvg.find("<path", at + 1)) {
            ++paths;
        }
        assert(paths == 2);
    }

    // Stroke opacity: emitted only when it says something (1.0 is SVG's
    // default, so fully opaque output stays byte-identical to before), and
    // a different alpha splits its own <path> like any other look change.
    {
        DraftingPlotJob job;
        DraftingPlotSegment faded = segment({0.1, 0.1}, {0.9, 0.1}, "pen_black", "#d7dde8", 2.0);
        faded.opacity = 0.5;
        job.strokeSegments = {
            faded,
            segment({0.1, 0.5}, {0.9, 0.5}, "pen_black", "#d7dde8", 2.0), // opacity 1.0
        };
        const std::string fadedSvg = svgFromPlotJob(job, page());
        assert(fadedSvg.find("stroke-opacity=\"0.5\"") != std::string::npos);
        // Exactly one occurrence: the opaque path carries no opacity attribute.
        assert(fadedSvg.find("stroke-opacity", fadedSvg.find("stroke-opacity") + 1) == std::string::npos);
        std::size_t paths = 0;
        for (std::size_t at = fadedSvg.find("<path"); at != std::string::npos; at = fadedSvg.find("<path", at + 1)) {
            ++paths;
        }
        assert(paths == 2); // same pen, same color — alpha alone splits the look
    }

    // Quantization consistency: an opacity that FORMATS to "1" (0.9996 at
    // 3 decimals) must neither emit an attribute nor split its own path —
    // the emit gate and the group key must share the same resolution.
    {
        DraftingPlotJob job;
        DraftingPlotSegment nearOpaque = segment({0.1, 0.1}, {0.9, 0.1}, "pen_black", "#d7dde8", 2.0);
        nearOpaque.opacity = 0.9996;
        job.strokeSegments = {
            nearOpaque,
            segment({0.1, 0.5}, {0.9, 0.5}, "pen_black", "#d7dde8", 2.0), // exactly 1.0
        };
        const std::string nearSvg = svgFromPlotJob(job, page());
        assert(nearSvg.find("stroke-opacity") == std::string::npos);
        std::size_t paths = 0;
        for (std::size_t at = nearSvg.find("<path"); at != std::string::npos; at = nearSvg.find("<path", at + 1)) {
            ++paths;
        }
        assert(paths == 1); // same look at output resolution -> one path
    }

    return 0;
}
