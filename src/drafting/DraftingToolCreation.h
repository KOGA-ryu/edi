#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingToolKind {
    SelectMove,
    Point,
    Line,
    Rectangle,
    Circle,
    Arc,
    RegularPolygon,
    Polyline,
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
    // Arc tool option: sweep from the start angle (legacy 15->120 = 105deg).
    double arcSweepDeg = 105.0;
    // Polyline tool: the accumulated click trail. Multi-click tools carry
    // their whole path here; start/end stay meaningful for two-click tools
    // only. Validation (>= 2 finite vertices) lives with the geometry.
    std::vector<Point2D> vertices;
};

DraftingToolKind draftingToolKindFromId(const std::string &toolId);
const char *draftingToolKindName(DraftingToolKind kind);
DraftingObjectBuildResult buildDraftingObjectForTool(const DraftingToolCreationRequest &request);

} // namespace edi::drafting
