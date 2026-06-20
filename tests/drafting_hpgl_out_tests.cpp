#include "drafting/DraftingHpglOut.h"

#include "EdiAssert.h"
#include <string>

using namespace edi::drafting;

namespace {

DraftingPlotSegment segment(Point2D a, Point2D b, const std::string &penId)
{
    DraftingPlotSegment s;
    s.objectId = "obj";
    s.layerId = "default";
    s.rawA = a;
    s.rawB = b;
    s.a = a;
    s.b = b;
    s.penId = penId;
    s.strokeColor = "#000000";
    s.strokeWidth = 1.0;
    return s;
}

DraftingPlotJob sampleJob()
{
    DraftingPlotJob job;
    job.strokeSegments = {
        segment({0.0, 0.0}, {0.5, 0.0}, "pen_black"),
        segment({1.0, 1.0}, {1.0, 0.5}, "pen_blue"),
        segment({1.0, 0.5}, {0.5, 0.5}, "pen_blue"),
    };
    job.penStats = {
        {"pen_black", "#000000", 1.0},
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
    // 100mm page, 40 plotter units/mm, y flipped (origin bottom-left). The blue
    // pen's two segments share an endpoint and chain into one PD.
    const std::string hpgl = hpglFromPlotJob(sampleJob(), page(), {});
    const std::string expected =
        "IN;\n"
        "SP1;\n"
        "PU0,4000;\n"
        "PD2000,4000;\n"
        "SP2;\n"
        "PU4000,0;\n"
        "PD4000,2000,2000,2000;\n"
        "PU;\n"
        "SP0;\n"
        "IN;\n";
    EDI_CHECK(hpgl == expected);

    // returnHome=false omits the trailing IN;.
    {
        DraftingHpglSettings noHome;
        noHome.returnHome = false;
        const std::string noHomeHpgl = hpglFromPlotJob(sampleJob(), page(), noHome);
        EDI_CHECK(noHomeHpgl.find("SP0;\n") != std::string::npos);
        // The string ends right after SP0; (no trailing IN;).
        EDI_CHECK(noHomeHpgl.substr(noHomeHpgl.size() - 5) == "SP0;\n");
    }

    // Pen mapping follows penStats order: reversing it swaps SP indices.
    {
        DraftingPlotJob job = sampleJob();
        job.penStats = {
            {"pen_blue", "#0000ff", 1.0},
            {"pen_black", "#000000", 1.0},
        };
        const std::string swapped = hpglFromPlotJob(job, page(), {});
        // pen_blue is now SP1 and appears first; pen_black is SP2.
        const std::size_t blue = swapped.find("SP1;");
        const std::size_t black = swapped.find("SP2;");
        EDI_CHECK(blue != std::string::npos && black != std::string::npos);
        EDI_CHECK(blue < black);
        // pen_blue's chain (PD with two points) now sits under SP1.
        EDI_CHECK(swapped.find("SP1;\nPU4000,0;\nPD4000,2000,2000,2000;") != std::string::npos);
    }

    // Y is genuinely flipped: a segment at the top of the page (y=0) maps to the
    // max plotter y, not 0.
    {
        DraftingPlotJob job;
        job.strokeSegments = {segment({0.0, 0.0}, {1.0, 0.0}, "pen_black")};
        job.penStats = {{"pen_black", "#000000", 1.0}};
        const std::string topEdge = hpglFromPlotJob(job, page(), {});
        EDI_CHECK(topEdge.find("PU0,4000;") != std::string::npos); // y=0 -> 4000
    }

    return 0;
}
