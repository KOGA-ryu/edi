#include "drafting/DraftingBuildPlan.h"

#include "drafting/DraftingDocument.h"

#include "EdiAssert.h"
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

    auto checkedRectNote = buildPlanNoteForObjectChecked(measuredRect);
    EDI_CHECK(checkedRectNote.ok);
    EDI_CHECK(checkedRectNote.code == DraftingResultCode::None);
    BuildPlanNote rectNote = checkedRectNote.note;
    EDI_CHECK(rectNote.objectId == "panel_01");
    EDI_CHECK(rectNote.materialNote.empty());
    EDI_CHECK(rectNote.constructionNote == "cut from acrylic");
    EDI_CHECK(rectNote.measurementLines.size() == 3);
    EDI_CHECK(rectNote.measurementLines[0] == std::string("area: 12.5 square centimeter"));
    EDI_CHECK(rectNote.measurementLines[1] == std::string("width: 5 centimeter"));
    EDI_CHECK(rectNote.measurementLines[2] == std::string("height: 2.5 centimeter"));

    DraftingObject measuredLine = makeDraftingObject(
        "rail_01",
        DraftingShapeKind::Line,
        LineGeometry{{0.0, 0.0}, {0.0, 12.0}});
    measuredLine.metadata.measurement.unit = MeasurementUnit::Inch;
    measuredLine.metadata.measurement.canvasUnitsPerRealUnit = 4.0;

    auto checkedLineNote = buildPlanNoteForObjectChecked(measuredLine);
    EDI_CHECK(checkedLineNote.ok);
    BuildPlanNote lineNote = checkedLineNote.note;
    EDI_CHECK(lineNote.objectId == "rail_01");
    EDI_CHECK(lineNote.constructionNote.empty());
    EDI_CHECK(lineNote.measurementLines.size() == 3);
    EDI_CHECK(lineNote.measurementLines[0] == std::string("distance: 3 inch"));
    EDI_CHECK(lineNote.measurementLines[1] == std::string("width: 0 inch"));
    EDI_CHECK(lineNote.measurementLines[2] == std::string("height: 3 inch"));

    DraftingObject invalidMeasuredRect = measuredRect;
    invalidMeasuredRect.metadata.measurement.canvasUnitsPerRealUnit = 0.0;
    auto checkedInvalidNote = buildPlanNoteForObjectChecked(invalidMeasuredRect);
    EDI_CHECK(!checkedInvalidNote.ok);
    EDI_CHECK(checkedInvalidNote.code == DraftingResultCode::InvalidMetadata);
    EDI_CHECK(checkedInvalidNote.note.objectId == "panel_01");
    EDI_CHECK(checkedInvalidNote.note.measurementLines.empty());
    EDI_CHECK(checkedInvalidNote.note.constructionNote == "cut from acrylic");
    BuildPlanNote invalidNote = buildPlanNoteForObject(invalidMeasuredRect);
    EDI_CHECK(invalidNote.objectId == "panel_01");
    EDI_CHECK(invalidNote.measurementLines.empty());
    EDI_CHECK(invalidNote.constructionNote == "cut from acrylic");

    BuildPlanDocument planDocument = buildPlanDocumentForObjects({measuredRect, measuredLine, invalidMeasuredRect});
    EDI_CHECK(planDocument.notes.size() == 2);
    EDI_CHECK(planDocument.notes[0].objectId == "panel_01");
    EDI_CHECK(planDocument.notes[1].objectId == "rail_01");
    EDI_CHECK(planDocument.failures.size() == 1);
    EDI_CHECK(!planDocument.failures[0].ok);
    EDI_CHECK(planDocument.failures[0].code == DraftingResultCode::InvalidMetadata);
    EDI_CHECK(planDocument.failures[0].note.objectId == "panel_01");
    EDI_CHECK(planDocument.failures[0].note.constructionNote == "cut from acrylic");

    BuildPlanDocument emptyPlanDocument = buildPlanDocumentForObjects({});
    EDI_CHECK(emptyPlanDocument.notes.empty());
    EDI_CHECK(emptyPlanDocument.failures.empty());

    return 0;
}
