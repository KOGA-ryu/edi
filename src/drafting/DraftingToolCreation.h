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
    RegularPolygon,
    HorizontalGuide,
    VerticalGuide,
    HorizontalConstructionLine,
    VerticalConstructionLine,
    AngledConstructionLine,
    DistanceDimension,
    WidthDimension,
    HeightDimension,
    RadiusDimension,
    DiameterDimension,
    Unknown
};

struct DraftingToolCreationRequest {
    DraftingToolKind tool = DraftingToolKind::Unknown;
    DraftingObjectId objectId;
    LayerId layerId = "default";
    Point2D start;
    Point2D end;
    std::string toolProvenance;
    // Regular-polygon tool options (legacy defaults: 6 sides, 30deg rotation).
    int polygonSides = 6;
    double polygonRotationDeg = 30.0;
};

DraftingToolKind draftingToolKindFromId(const std::string &toolId);
const char *draftingToolKindName(DraftingToolKind kind);
DraftingObjectBuildResult buildDraftingObjectForTool(const DraftingToolCreationRequest &request);

} // namespace edi::drafting
