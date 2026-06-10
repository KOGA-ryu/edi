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

    return 0;
}
