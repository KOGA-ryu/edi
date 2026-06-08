#include "drafting/DraftingBuildPlan.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingMeasurement.h"
#include "drafting/DraftingMeasurementFormat.h"

namespace edi::drafting {

BuildPlanNote buildPlanNoteForObject(const DraftingObject &object)
{
    BuildPlanNote note;
    note.objectId = object.id;
    note.constructionNote = object.metadata.measurementNote;

    const auto summary = summarizeObjectMeasurement(object);
    if (summary.ok) {
        note.measurementLines = formatObjectMeasurementSummary(summary.value);
    }

    return note;
}

} // namespace edi::drafting
