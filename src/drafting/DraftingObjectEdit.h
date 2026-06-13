#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingObjectEditKind {
    MovePoint,
    MoveLineStart,
    MoveLineEnd,
    MoveRectangleCorner,
    RotateRectangle,
    MoveCircleCenter,
    SetCircleRadius,
    MoveEllipseCenter,
    SetEllipseRx,
    SetEllipseRy,
    MoveArcCenter,
    SetArcRadius,
    SetArcStartAngle,
    SetArcEndAngle,
    MoveDimensionStart,
    MoveDimensionEnd,
    SetDimensionOffset
};

struct DraftingHandleDescriptor {
    std::string id;
    std::string role;
    Point2D point;
    bool readOnly = false;
    bool hasAnchor = false;
    Point2D anchor;
};

struct DraftingObjectEdit {
    DraftingObjectEditKind kind = DraftingObjectEditKind::MovePoint;
    std::string handleId;
    Point2D point;
    double value = 0.0;
    // N4 aspect-lock: when set on a MoveRectangleCorner edit, the drag is
    // constrained to preserve the rectangle's original width:height ratio.
    // Default false keeps every other edit path byte-identical.
    bool preserveAspect = false;
};

struct DraftingObjectEditResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingGeometry geometry = PointGeometry{};
    Bounds2D bounds;

    static DraftingObjectEditResult accepted(DraftingGeometry geometry, Bounds2D bounds);
    static DraftingObjectEditResult rejected(DraftingResultCode code, std::string message);
};

struct DraftingHandleEditPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingObjectEdit edit;

    static DraftingHandleEditPlan accepted(DraftingObjectEdit edit);
    static DraftingHandleEditPlan rejected(DraftingResultCode code, std::string message);
};

std::vector<DraftingHandleDescriptor> draftingHandlesForObject(const DraftingObject &object);
DraftingHandleDescriptor draftingHandleById(const DraftingObject &object, const std::string &handleId);
// preserveAspect rides onto a rectangle corner edit (ignored by every other
// handle); the controller passes its aspect-lock mode (or a Shift drag).
DraftingHandleEditPlan handleEditPlan(const DraftingObject &object, const std::string &handleId, Point2D point,
                                      bool preserveAspect = false);
DraftingObjectEditResult applyObjectEdit(const DraftingObject &object, const DraftingObjectEdit &edit);

} // namespace edi::drafting
