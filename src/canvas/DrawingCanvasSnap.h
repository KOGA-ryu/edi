#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

struct SnapCandidate {
    double x = 0.0;
    double y = 0.0;
    QString sourceKind;
    QString label;
    QString sourceObjectId;
};

double effectiveGridStepPx(const QVariantMap &settings);
std::vector<SnapCandidate> snapCandidatesForObject(const CanvasObjectView &object, const QVariantMap &settings);
std::vector<SnapCandidate> snapCandidates(const std::vector<CanvasObjectView> &objects, const QVariantMap &settings);
SnapResult noneSnap(const CanvasPoint &point, const QVariantMap &settings);
SnapResult gridSnap(const CanvasPoint &point, const QVariantMap &settings);
SnapResult resolveSnap(const CanvasPoint &rawPoint, const std::vector<CanvasObjectView> &objects, const QVariantMap &settings);

} // namespace drawing_canvas
