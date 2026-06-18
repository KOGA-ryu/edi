#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingPlotOrderMode {
    LayerOrder,
    NearestNext,
};

enum class DraftingPlotDirectionMode {
    PreserveDirection,
    AllowReverseSegments,
};

struct DraftingPlotSettings {
    DraftingPlotOrderMode orderMode = DraftingPlotOrderMode::LayerOrder;
    DraftingPlotDirectionMode directionMode = DraftingPlotDirectionMode::PreserveDirection;
    double calibrationScale = 1.0;
};

struct DraftingPlotObject {
    DraftingObjectId objectId;
    LayerId layerId;
    std::string penId;
    std::string strokeColor;
    double strokeWidth = 0.0;
};

struct DraftingPlotSegment {
    DraftingObjectId objectId;
    LayerId layerId;
    Point2D rawA;
    Point2D rawB;
    Point2D a;
    Point2D b;
    std::string penId;
    std::string strokeColor;
    double strokeWidth = 0.0;
    std::string lineStyle = "solid";
    // Per-object stroke opacity (0..1). Carried for renderers that can show
    // it (canvas, SVG); pen plotters ignore it — a physical pen has no alpha.
    double opacity = 1.0;
};

// Solid per-object fill, carried on a SEPARATE channel from the stroke
// segments. Fill is a closed-region concept SVG can paint but pen plotters
// (HPGL/G-code) cannot — so it must NOT ride on DraftingPlotSegment, which is
// the shared pen-plotter vocabulary. `points` is the closed vertex ring in the
// same projected space as a segment's `a`/`b` (the SVG writer multiplies by the
// page's mm size); `color` is "#rrggbb"; `opacity` is the fill alpha (0..1).
struct DraftingPlotFill {
    DraftingObjectId objectId;
    std::vector<Point2D> points;
    std::string color;
    double opacity = 1.0;
};

struct DraftingPlotTravelSegment {
    DraftingObjectId fromObjectId;
    DraftingObjectId toObjectId;
    LayerId toLayerId;
    std::string toPenId;
    Point2D rawA;
    Point2D rawB;
    Point2D a;
    Point2D b;
    double distance = 0.0;
};

struct DraftingPlotLayerStats {
    LayerId layerId;
    std::string layerName;
    int objectCount = 0;
    int segmentCount = 0;
    double strokeDistance = 0.0;
    double travelDistance = 0.0;
    bool ready = false;
    std::string blockedReason = "no_plotted_objects";
};

struct DraftingPlotPenStats {
    std::string penId;
    std::string strokeColor;
    double strokeWidth = 0.0;
    int objectCount = 0;
    int segmentCount = 0;
    double strokeDistance = 0.0;
    double travelDistance = 0.0;
    bool ready = false;
    std::string blockedReason = "no_assigned_segments";
};

struct DraftingPlotWarning {
    DraftingObjectId objectId;
    std::string kind;
    std::string message;
};

struct DraftingPlotPlan {
    std::vector<DraftingPlotObject> objects;
    std::vector<DraftingPlotSegment> segments;
    // Per-object solid fills, beside (not inside) the stroke segments — see
    // DraftingPlotFill. Empty for documents whose objects have no fill.
    std::vector<DraftingPlotFill> fills;
    std::vector<DraftingPlotTravelSegment> travelSegments;
    std::vector<DraftingPlotLayerStats> layerStats;
    std::vector<DraftingPlotPenStats> penStats;
    std::vector<DraftingPlotWarning> warnings;
    DraftingPlotOrderMode orderMode = DraftingPlotOrderMode::LayerOrder;
    DraftingPlotDirectionMode directionMode = DraftingPlotDirectionMode::PreserveDirection;
    Bounds2D plotBounds;
    double travelDistance = 0.0;
    double calibrationScale = 1.0;
    bool hasPlotBounds = false;
};

DraftingPlotSettings defaultDraftingPlotSettings();
const char *draftingPlotOrderModeName(DraftingPlotOrderMode mode);
DraftingPlotOrderMode draftingPlotOrderModeFromName(const std::string &name);
const char *draftingPlotDirectionModeName(DraftingPlotDirectionMode mode);
DraftingPlotDirectionMode draftingPlotDirectionModeFromName(const std::string &name);
bool draftingShapeCanPlot(DraftingShapeKind kind);
DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid);
DraftingPlotPlan buildDraftingPlotPlan(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings);


// The stroke-color gate the plot readiness checks use ("#rrggbb" exactly).
// Public so the UI rejects at entry what the plot would block at exit —
// one vocabulary for "a color this system accepts".
bool draftingStrokeColorIsValid(const std::string &value);

// Returns true for exactly the four geometry kinds that can carry a solid fill
// that the painter/SVG writer will actually render: Rectangle, Circle, Ellipse,
// Polygon. Every other kind is an open or structurally line-only shape.
//
// This is kept in LOCKSTEP with the closedFillRing visitor above: that visitor
// returns a non-empty ring for exactly these four kinds and falls through to
// empty (or hits the static_assert) for the rest.  Enforcing the gate here at
// authoring time — rather than silently ignoring the fill at render time —
// keeps the model consistent and avoids invisible "write-only" state.
//
// The switch is EXHAUSTIVE over all 14 DraftingShapeKind members so that adding
// a new kind is a compile error here too, forcing an explicit decision.
bool draftingShapeIsFillable(DraftingShapeKind kind);

} // namespace edi::drafting
