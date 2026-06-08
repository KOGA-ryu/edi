#include "drafting/DraftingBuildPlan.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingMeasurement.h"
#include "drafting/DraftingMeasurementFormat.h"

#include <utility>

namespace edi::drafting {

BuildPlanNoteResult BuildPlanNoteResult::accepted(BuildPlanNote note)
{
    BuildPlanNoteResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.note = std::move(note);
    return result;
}

BuildPlanNoteResult BuildPlanNoteResult::rejected(DraftingResultCode code, std::string message, BuildPlanNote note)
{
    BuildPlanNoteResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    result.note = std::move(note);
    return result;
}

BuildPlanNoteResult buildPlanNoteForObjectChecked(const DraftingObject &object)
{
    BuildPlanNote note;
    note.objectId = object.id;
    note.constructionNote = object.metadata.measurementNote;

    const auto summary = summarizeObjectMeasurement(object);
    if (!summary.ok) {
        return BuildPlanNoteResult::rejected(summary.code, summary.message, std::move(note));
    }
    note.measurementLines = formatObjectMeasurementSummary(summary.value);

    return BuildPlanNoteResult::accepted(std::move(note));
}

BuildPlanNote buildPlanNoteForObject(const DraftingObject &object)
{
    return buildPlanNoteForObjectChecked(object).note;
}

BuildPlanDocument buildPlanDocumentForObjects(const std::vector<DraftingObject> &objects)
{
    BuildPlanDocument document;
    for (const DraftingObject &object : objects) {
        BuildPlanNoteResult note = buildPlanNoteForObjectChecked(object);
        if (note.ok) {
            document.notes.push_back(std::move(note.note));
        } else {
            document.failures.push_back(std::move(note));
        }
    }
    return document;
}

} // namespace edi::drafting
