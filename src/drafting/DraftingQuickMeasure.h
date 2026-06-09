#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <string>

namespace edi::drafting {

enum class DraftingQuickMeasureKind {
    None,
    Point,
    Line,
    Rectangle,
    Circle,
    Unsupported
};

struct DraftingQuickMeasureResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingQuickMeasureKind kind = DraftingQuickMeasureKind::None;
    DraftingObjectId objectId;
    DraftingShapeKind objectKind = DraftingShapeKind::Point;
    double hitDistance = 0.0;
    DraftingGridUnit unit = DraftingGridUnit::CanvasUnit;
    std::string unitName;
    std::string unitLabel;
    std::string label;

    double x = 0.0;
    double y = 0.0;
    double physicalX = 0.0;
    double physicalY = 0.0;

    double length = 0.0;
    double angleDeg = 0.0;
    double physicalLength = 0.0;
    double physicalAngleDeg = 0.0;

    double width = 0.0;
    double height = 0.0;
    double area = 0.0;
    double physicalWidth = 0.0;
    double physicalHeight = 0.0;
    double physicalArea = 0.0;

    double radius = 0.0;
    double diameter = 0.0;
    double physicalRadius = 0.0;
    double physicalDiameter = 0.0;
    double physicalRadiusY = 0.0;
    double physicalDiameterY = 0.0;

    static DraftingQuickMeasureResult rejected(
        DraftingQuickMeasureKind kind,
        DraftingResultCode code,
        std::string message,
        const DraftingGridProjection &grid);
};

const char *draftingQuickMeasureKindName(DraftingQuickMeasureKind kind);

DraftingQuickMeasureResult quickMeasureAt(
    const DraftingDocument &document,
    Point2D rawPoint,
    const DraftingGridProjection &grid);

} // namespace edi::drafting
