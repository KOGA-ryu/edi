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
    SetCircleRadius
};

struct DraftingHandleDescriptor {
    std::string id;
    std::string role;
    Point2D point;
    bool readOnly = false;
};

struct DraftingObjectEdit {
    DraftingObjectEditKind kind = DraftingObjectEditKind::MovePoint;
    std::string handleId;
    Point2D point;
    double value = 0.0;
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
DraftingHandleEditPlan handleEditPlan(const DraftingObject &object, const std::string &handleId, Point2D point);
DraftingObjectEditResult applyObjectEdit(const DraftingObject &object, const DraftingObjectEdit &edit);

} // namespace edi::drafting
