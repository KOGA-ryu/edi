#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

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
    Guide
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

Point2D normalizeDraftingPoint(Point2D point);
const char *draftingSnapKindName(DraftingSnapKind kind);
const char *draftingSnapSourceKindName(DraftingSnapSourceKind kind);
const char *draftingSnapTolerancePresetId(double tolerance);
double draftingSnapToleranceForPreset(const std::string &presetId);
std::vector<DraftingSnapCandidate> snapCandidatesForObject(const DraftingObject &object, const DraftingSnapSettings &settings = {});
std::vector<DraftingSnapCandidate> snapCandidatesForDocument(const DraftingDocument &document, const DraftingSnapSettings &settings = {});
DraftingSnapResult noneSnap(Point2D point);
DraftingSnapResult gridSnap(Point2D point, const DraftingSnapSettings &settings = {});
DraftingSnapResult resolveSnap(Point2D rawPoint, const DraftingDocument &document, const DraftingSnapSettings &settings = {});

} // namespace edi::drafting
