#include "drafting/DraftingGcodeOut.h"

#include <cassert>
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
    // Golden output: 100mm page, y flipped (origin bottom-left), pens become
    // tool changes, blue's two shared-endpoint segments chain into one stroke.
    // G0 = travel (pen up), G1 = stroke (pen down) — the load-bearing contrast.
    const std::string gcode = gcodeFromPlotJob(sampleJob(), page(), {});
    const std::string expected =
        "G21\n"
        "G90\n"
        "G0 Z5.000\n"
        "T1 M6\n"
        "G0 X0.000 Y100.000\n"
        "G1 Z0.000 F1500.000\n"
        "G1 X50.000 Y100.000\n"
        "G0 Z5.000\n"
        "T2 M6\n"
        "G0 X100.000 Y0.000\n"
        "G1 Z0.000 F1500.000\n"
        "G1 X100.000 Y50.000\n"
        "G1 X50.000 Y50.000\n"
        "G0 Z5.000\n"
        "G0 X0.000 Y0.000\n"
        "M2\n";
    assert(gcode == expected);

    // returnHome=false drops the G0 X0 Y0 before M2.
    {
        DraftingGcodeSettings noHome;
        noHome.returnHome = false;
        const std::string out = gcodeFromPlotJob(sampleJob(), page(), noHome);
        assert(out.find("G0 X0.000 Y0.000") == std::string::npos);
        assert(out.substr(out.size() - 3) == "M2\n");
    }

    // Pen->tool mapping follows penStats order: reversing it swaps T indices
    // and which tool draws the two-point chain.
    {
        DraftingPlotJob job = sampleJob();
        job.penStats = {
            {"pen_blue", "#0000ff", 1.0},
            {"pen_black", "#000000", 1.0},
        };
        const std::string swapped = gcodeFromPlotJob(job, page(), {});
        const std::size_t t1 = swapped.find("T1 M6");
        const std::size_t t2 = swapped.find("T2 M6");
        assert(t1 != std::string::npos && t2 != std::string::npos && t1 < t2);
        // The chained (two-draw) stroke now sits under T1 (pen_blue).
        assert(swapped.find("T1 M6\nG0 X100.000 Y0.000\nG1 Z0.000 F1500.000\n"
                            "G1 X100.000 Y50.000\nG1 X50.000 Y50.000\n") != std::string::npos);
    }

    // Spindle mode swaps Z pen control for M3 (down) / M5 (up); no Z words.
    {
        DraftingGcodeSettings spindle;
        spindle.penMode = DraftingGcodePenMode::Spindle;
        const std::string out = gcodeFromPlotJob(sampleJob(), page(), spindle);
        assert(out.find(" Z") == std::string::npos);   // no Z moves at all
        assert(out.find("M3\n") != std::string::npos);  // pen down
        assert(out.find("M5\n") != std::string::npos);  // pen up
        // Travel is still G0, stroke still G1.
        assert(out.find("G0 X0.000 Y100.000\nM3\nG1 X50.000 Y100.000\nM5\n") != std::string::npos);
    }

    // Y is genuinely flipped: a top-of-page segment (y=0) maps to max machine Y.
    {
        DraftingPlotJob job;
        job.strokeSegments = {segment({0.0, 0.0}, {1.0, 0.0}, "pen_black")};
        job.penStats = {{"pen_black", "#000000", 1.0}};
        const std::string out = gcodeFromPlotJob(job, page(), {});
        assert(out.find("G0 X0.000 Y100.000\n") != std::string::npos); // y=0 -> 100mm
    }

    return 0;
}
