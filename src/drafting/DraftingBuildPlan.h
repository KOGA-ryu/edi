#pragma once

#include "drafting/DraftingTypes.h"

#include <string>
#include <vector>

namespace edi::drafting {

struct DraftingObject;

struct BuildPlanNote {
    DraftingObjectId objectId;
    std::vector<std::string> measurementLines;
    std::string materialNote;
    std::string constructionNote;
};

struct BuildPlanNoteResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    BuildPlanNote note;

    static BuildPlanNoteResult accepted(BuildPlanNote note);
    static BuildPlanNoteResult rejected(DraftingResultCode code, std::string message, BuildPlanNote note);
};

struct BuildPlanDocument {
    std::vector<BuildPlanNote> notes;
    std::vector<BuildPlanNoteResult> failures;
};

BuildPlanNoteResult buildPlanNoteForObjectChecked(const DraftingObject &object);
BuildPlanNote buildPlanNoteForObject(const DraftingObject &object);
BuildPlanDocument buildPlanDocumentForObjects(const std::vector<DraftingObject> &objects);

} // namespace edi::drafting
