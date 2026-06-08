#pragma once

#include "drafting/DraftingDocument.h"

namespace edi::drafting {

struct DraftingHitTestSettings {
    double tolerance = 0.03;
};

struct DraftingHitTestResult {
    bool ok = false;
    DraftingObjectId objectId;
    DraftingShapeKind kind = DraftingShapeKind::Point;
    double distance = 0.0;
};

double hitDistance(const DraftingGeometry &geometry, Point2D point);
DraftingHitTestResult hitTestObject(const DraftingObject &object, Point2D point);
DraftingHitTestResult hitTestDocument(const DraftingDocument &document, Point2D point, DraftingHitTestSettings settings = {});

} // namespace edi::drafting
