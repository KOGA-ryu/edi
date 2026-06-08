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

BuildPlanNote buildPlanNoteForObject(const DraftingObject &object);

} // namespace edi::drafting
