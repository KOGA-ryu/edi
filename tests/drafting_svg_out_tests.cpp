#include "drafting/DraftingSvgOut.h"

#include "EdiAssert.h"
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
    EDI_CHECK(svg == expected);

    // Inch units scale the viewBox by 25.4 mm/in.
    {
        DraftingGridProjection inchGrid;
        inchGrid.settings.width = 1.0;
        inchGrid.settings.height = 2.0;
        inchGrid.settings.unit = DraftingGridUnit::Inch;
        const std::string inchSvg = svgFromPlotJob(sampleJob(), inchGrid);
        EDI_CHECK(inchSvg.find("viewBox=\"0 0 25.4 50.8\"") != std::string::npos);
    }

    // An empty job still produces a valid, path-free SVG envelope.
    {
        DraftingPlotJob empty;
        const std::string emptySvg = svgFromPlotJob(empty, page());
        EDI_CHECK(emptySvg.find("<path") == std::string::npos);
        EDI_CHECK(emptySvg.find("<svg") == 0);
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
        EDI_CHECK(styledSvg.find("stroke=\"#ff6600\"") != std::string::npos);
        EDI_CHECK(styledSvg.find("stroke-dasharray=") != std::string::npos);
        EDI_CHECK(styledSvg.find("stroke=\"#d7dde8\"") != std::string::npos);
        // Two looks, two paths — even on one physical pen.
        std::size_t paths = 0;
        for (std::size_t at = styledSvg.find("<path"); at != std::string::npos; at = styledSvg.find("<path", at + 1)) {
            ++paths;
        }
        EDI_CHECK(paths == 2);
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
        EDI_CHECK(fadedSvg.find("stroke-opacity=\"0.5\"") != std::string::npos);
        // Exactly one occurrence: the opaque path carries no opacity attribute.
        EDI_CHECK(fadedSvg.find("stroke-opacity", fadedSvg.find("stroke-opacity") + 1) == std::string::npos);
        std::size_t paths = 0;
        for (std::size_t at = fadedSvg.find("<path"); at != std::string::npos; at = fadedSvg.find("<path", at + 1)) {
            ++paths;
        }
        EDI_CHECK(paths == 2); // same pen, same color — alpha alone splits the look
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
        EDI_CHECK(nearSvg.find("stroke-opacity") == std::string::npos);
        std::size_t paths = 0;
        for (std::size_t at = nearSvg.find("<path"); at != std::string::npos; at = nearSvg.find("<path", at + 1)) {
            ++paths;
        }
        EDI_CHECK(paths == 1); // same look at output resolution -> one path
    }

    // Fill side-channel: a closed <path fill="#..."> is emitted per fill
    // record, BEFORE the stroke paths (fill-under-stroke z-order). The opaque
    // fill carries no fill-opacity attribute; the half-opaque one does.
    {
        DraftingPlotJob job;
        job.strokeSegments = {
            segment({0.1, 0.1}, {0.9, 0.1}, "pen_black", "#000000", 2.0),
        };
        DraftingPlotFill solid;
        solid.objectId = "rect";
        solid.points = {{0.1, 0.1}, {0.9, 0.1}, {0.9, 0.9}, {0.1, 0.9}};
        solid.color = "#ff0000";
        solid.opacity = 1.0; // fully opaque -> no fill-opacity attribute
        DraftingPlotFill faded;
        faded.objectId = "tri";
        faded.points = {{0.2, 0.2}, {0.4, 0.2}, {0.3, 0.4}};
        faded.color = "#00ff00";
        faded.opacity = 0.5;
        job.fills = {solid, faded};

        const std::string svgFill = svgFromPlotJob(job, page());
        // Opaque fill: colour present, closed with Z, no fill-opacity.
        const std::size_t opaqueAt = svgFill.find("<path fill=\"#ff0000\" d=\"M10,10 L90,10 L90,90 L10,90 Z\"/>");
        EDI_CHECK(opaqueAt != std::string::npos);
        // Half-opaque fill carries the attribute.
        const std::size_t fadedAt = svgFill.find("<path fill=\"#00ff00\" fill-opacity=\"0.5\" d=\"M20,20 L40,20 L30,40 Z\"/>");
        EDI_CHECK(fadedAt != std::string::npos);
        // Fill paints UNDER the stroke: both fills precede the stroke path.
        const std::size_t strokeAt = svgFill.find("<path fill=\"none\"");
        EDI_CHECK(strokeAt != std::string::npos);
        EDI_CHECK(opaqueAt < strokeAt);
        EDI_CHECK(fadedAt < strokeAt);
        // Exactly one fill-opacity occurrence (the opaque fill omits it).
        EDI_CHECK(svgFill.find("fill-opacity", svgFill.find("fill-opacity") + 1) == std::string::npos);
    }

    // A job with no fills emits no fill path — the byte-identical default the
    // boundary requires (the golden assertion above already proves this for the
    // line-only sample job; here we make the absence explicit).
    {
        const std::string svgNoFill = svgFromPlotJob(sampleJob(), page());
        // Every <path> in the fill-less job is a stroke path (fill="none").
        for (std::size_t at = svgNoFill.find("<path"); at != std::string::npos; at = svgNoFill.find("<path", at + 1)) {
            EDI_CHECK(svgNoFill.compare(at, std::string("<path fill=\"none\"").size(), "<path fill=\"none\"") == 0);
        }
    }

    return 0;
}
