#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

struct DraftingGridSettings;

enum class DraftingSnapKind {
    None,
    Grid,
    Object
};

enum class DraftingSnapSourceKind {
    None,
    Endpoint,
    Vertex,
    Midpoint,
    Center,
    Guide,
    Intersection, // where two objects cross — a document-level (pairwise) candidate
    Quadrant,     // the 0/90/180/270° cardinal points on a circle/arc perimeter
    OnCurve,      // the nearest projection of the cursor onto a curve (lowest priority)
    Tangent,      // RELATIVE: a contact point where a line from the anchor touches a circle/arc
    Perpendicular, // RELATIVE: the foot of the perpendicular from the anchor onto a line/edge
    Node           // a standalone Point object (incl. materialized intersection nodes)
};

struct DraftingSnapSettings {
    bool gridEnabled = false;
    bool objectSnapEnabled = false;
    bool objectPriorityBeforeGrid = true;
    bool endpointEnabled = true;
    bool vertexEnabled = true;
    bool midpointEnabled = true;
    bool centerEnabled = true;
    bool guideEnabled = true;
    bool intersectionEnabled = true;
    bool quadrantEnabled = true;
    bool onCurveEnabled = true;
    bool tangentEnabled = true;
    bool perpendicularEnabled = true;
    bool nodeEnabled = true; // standalone Point objects and materialized intersection nodes
    double gridStep = 1.0 / 16.0;
    double gridStepX = 0.0;
    double gridStepY = 0.0;
    double objectTolerance = 0.03;
};

struct DraftingSnapCandidate {
    Point2D point;
    DraftingSnapSourceKind sourceKind = DraftingSnapSourceKind::None;
    std::string label;
    DraftingObjectId sourceObjectId;
};

struct DraftingSnapResult {
    Point2D point;
    DraftingSnapKind kind = DraftingSnapKind::None;
    DraftingSnapSourceKind sourceKind = DraftingSnapSourceKind::None;
    std::string label = "none";
    DraftingObjectId sourceObjectId;
};

// The result of pointAlongEntity — mirrors the DraftingOffsetResult plan idiom
// (ok + code + message + payload), so a caller branches on `ok` then reads `point`.
struct DraftingPointAlongResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    Point2D point;

    static DraftingPointAlongResult accepted(Point2D point);
    static DraftingPointAlongResult rejected(DraftingResultCode code, std::string message);
};

// The point a typed DISTANCE along a line / arc / polyline / polygon, measured
// from the start endpoint (fromEnd == false) or the far endpoint (fromEnd ==
// true). Rejects an overshoot (distance > path length), a negative/non-finite
// distance, or an unsupported geometry kind. Pure; no controller/UI wiring.
DraftingPointAlongResult pointAlongEntity(const DraftingGeometry &geometry, double distance, bool fromEnd);

Point2D normalizeDraftingPoint(Point2D point);
const char *draftingSnapKindName(DraftingSnapKind kind);
const char *draftingSnapSourceKindName(DraftingSnapSourceKind kind);
const char *draftingSnapTolerancePresetId(double tolerance);
double draftingSnapToleranceForPreset(const std::string &presetId);
void applyDraftingGridToSnapSettings(DraftingSnapSettings &snapSettings, const DraftingGridSettings &gridSettings);
std::vector<DraftingSnapCandidate> snapCandidatesForObject(const DraftingObject &object, const DraftingSnapSettings &settings = {});
std::vector<DraftingSnapCandidate> snapCandidatesForDocument(const DraftingDocument &document, const DraftingSnapSettings &settings = {});
// RELATIVE snap candidates anchored from `fromPoint` (the in-progress segment's
// first click): tangent-to-circle/arc contact points and perpendicular feet onto
// lines/edges. A SEPARATE overload from the anchor-free snapCandidatesForDocument
// — these depend on the anchor, so folding them in would force every snap query to
// carry a reference point. Returns only the Tangent/Perpendicular candidates.
std::vector<DraftingSnapCandidate> relativeSnapCandidatesForDocument(const DraftingDocument &document, Point2D fromPoint, const DraftingSnapSettings &settings = {});
DraftingSnapResult noneSnap(Point2D point);
DraftingSnapResult gridSnap(Point2D point, const DraftingSnapSettings &settings = {});
DraftingSnapResult resolveSnap(Point2D rawPoint, const DraftingDocument &document, const DraftingSnapSettings &settings = {});

} // namespace edi::drafting
