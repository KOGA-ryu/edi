#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

enum class DraftingToolKind {
    SelectMove,
    Point,
    Line,
    Rectangle,
    Circle,
    HorizontalGuide,
    VerticalGuide,
    HorizontalConstructionLine,
    VerticalConstructionLine,
    AngledConstructionLine,
    DistanceDimension,
    Unknown
};

struct DraftingToolCreationRequest {
    DraftingToolKind tool = DraftingToolKind::Unknown;
    DraftingObjectId objectId;
    Point2D start;
    Point2D end;
    std::string toolProvenance;
};

DraftingToolKind draftingToolKindFromId(const std::string &toolId);
const char *draftingToolKindName(DraftingToolKind kind);
DraftingObjectBuildResult buildDraftingObjectForTool(const DraftingToolCreationRequest &request);

} // namespace edi::drafting
