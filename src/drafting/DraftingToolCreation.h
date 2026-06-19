#pragma once

#include "drafting/DraftingCanvasDims.h"
#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingToolKind {
    SelectMove,
    Point,
    Line,
    Arrow,
    DoubleArrow,
    Rectangle,
    Circle,
    Arc,
    Ellipse,
    RegularPolygon,
    Polyline,
    Spline,
    Wall,
    TextAnnotation,
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
    // Angular dimension: NOT a drag tool — the user clicks two lines and the
    // controller's two-line-pick arm (DR-13 follow-up) calls planAngularDimension.
    // buildDraftingObjectForTool rejects this kind explicitly (see below).
    AngularDimension,
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
    // Rectangle variant options (N4): both zero = plain box. Carried like
    // polygonSides — controller tool-option state rides into the request.
    double rectCornerRadius = 0.0;
    double rectInset = 0.0;
    // Parametric radius (#30) for every radius-from-gesture tool — circle,
    // arc, regular polygon. 0 = gesture-sized (the click distance); positive
    // = stamp this radius regardless of where the second click landed,
    // clamped to the unit document space like the gesture itself.
    // One field, not one per tool: the tools share the same radius semantics,
    // so they share the same option.
    double fixedRadius = 0.0;
    // Wall tool: band thickness (in canvas units) for the two-click wall.
    // Carried like rectCornerRadius/fixedRadius — controller tool-option state
    // rides into the request. Default 0.1 so a fresh wall renders as a visible
    // band rather than a zero-width line.
    double wallThickness = kDefaultWallToolThickness;
    // Polyline tool: the accumulated click trail. Multi-click tools carry
    // their whole path here; start/end stay meaningful for two-click tools
    // only. Validation (>= 2 finite vertices) lives with the geometry.
    std::vector<Point2D> vertices;
};

DraftingToolKind draftingToolKindFromId(const std::string &toolId);
const char *draftingToolKindName(DraftingToolKind kind);
DraftingObjectBuildResult buildDraftingObjectForTool(const DraftingToolCreationRequest &request);

} // namespace edi::drafting
