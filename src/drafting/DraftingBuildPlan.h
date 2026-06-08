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

BuildPlanNoteResult buildPlanNoteForObjectChecked(const DraftingObject &object);
BuildPlanNote buildPlanNoteForObject(const DraftingObject &object);

} // namespace edi::drafting
