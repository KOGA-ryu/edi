#include "drafting/DraftingBuildPlan.h"

#include "drafting/DraftingDocument.h"

#include <cassert>
#include <string>

using namespace edi::drafting;

int main()
{
    DraftingObject measuredRect = makeDraftingObject(
        "panel_01",
        DraftingShapeKind::Rectangle,
        RectangleGeometry{{0.0, 0.0}, 10.0, 5.0});
    measuredRect.metadata.measurement.unit = MeasurementUnit::Centimeter;
    measuredRect.metadata.measurement.canvasUnitsPerRealUnit = 2.0;
    measuredRect.metadata.measurementNote = "cut from acrylic";

    BuildPlanNote rectNote = buildPlanNoteForObject(measuredRect);
    assert(rectNote.objectId == "panel_01");
    assert(rectNote.materialNote.empty());
    assert(rectNote.constructionNote == "cut from acrylic");
    assert(rectNote.measurementLines.size() == 3);
    assert(rectNote.measurementLines[0] == std::string("area: 12.5 square centimeter"));
    assert(rectNote.measurementLines[1] == std::string("width: 5 centimeter"));
    assert(rectNote.measurementLines[2] == std::string("height: 2.5 centimeter"));

    DraftingObject measuredLine = makeDraftingObject(
        "rail_01",
        DraftingShapeKind::Line,
        LineGeometry{{0.0, 0.0}, {0.0, 12.0}});
    measuredLine.metadata.measurement.unit = MeasurementUnit::Inch;
    measuredLine.metadata.measurement.canvasUnitsPerRealUnit = 4.0;

    BuildPlanNote lineNote = buildPlanNoteForObject(measuredLine);
    assert(lineNote.objectId == "rail_01");
    assert(lineNote.constructionNote.empty());
    assert(lineNote.measurementLines.size() == 3);
    assert(lineNote.measurementLines[0] == std::string("distance: 3 inch"));
    assert(lineNote.measurementLines[1] == std::string("width: 0 inch"));
    assert(lineNote.measurementLines[2] == std::string("height: 3 inch"));

    DraftingObject invalidMeasuredRect = measuredRect;
    invalidMeasuredRect.metadata.measurement.canvasUnitsPerRealUnit = 0.0;
    BuildPlanNote invalidNote = buildPlanNoteForObject(invalidMeasuredRect);
    assert(invalidNote.objectId == "panel_01");
    assert(invalidNote.measurementLines.empty());
    assert(invalidNote.constructionNote == "cut from acrylic");

    return 0;
}
