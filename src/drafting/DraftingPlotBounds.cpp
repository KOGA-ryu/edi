#include "drafting/DraftingPlotBounds.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingLayerOps.h"

#include <algorithm>
#include <cmath>

namespace edi::drafting {
namespace {

Bounds2D includeBounds(Bounds2D bounds, Bounds2D next)
{
    const double left = std::min(bounds.x, next.x);
    const double top = std::min(bounds.y, next.y);
    const double right = std::max(bounds.x + bounds.width, next.x + next.width);
    const double bottom = std::max(bounds.y + bounds.height, next.y + next.height);
    return {left, top, right - left, bottom - top};
}

Bounds2D boundsForPoints(Point2D a, Point2D b)
{
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.x, b.x);
    const double bottom = std::max(a.y, b.y);
    return {left, top, right - left, bottom - top};
}

bool containsObjectId(const std::vector<DraftingObjectId> &objectIds, const DraftingObjectId &objectId)
{
    return std::find(objectIds.begin(), objectIds.end(), objectId) != objectIds.end();
}

bool objectEffectivelyPlotReady(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return object.visible
        && layer != nullptr
        && layer->visible
        && layer->plot.plotEnabled
        && draftingShapeCanPlot(object.kind);
}

DraftingPlotBoundsStatus statusForRelation(DraftingDrawableBoundsRelation relation)
{
    if (relation == DraftingDrawableBoundsRelation::Unavailable) {
        return DraftingPlotBoundsStatus::Unavailable;
    }
    return relation == DraftingDrawableBoundsRelation::Inside
        ? DraftingPlotBoundsStatus::InsideDrawable
        : DraftingPlotBoundsStatus::OutsideDrawable;
}

} // namespace

const char *draftingPlotBoundsStatusName(DraftingPlotBoundsStatus status)
{
    switch (status) {
    case DraftingPlotBoundsStatus::InsideDrawable:
        return "inside";
    case DraftingPlotBoundsStatus::OutsideDrawable:
        return "outside";
    case DraftingPlotBoundsStatus::Unavailable:
        return "unavailable";
    }
    return "unavailable";
}

const char *draftingDrawableBoundsRelationName(DraftingDrawableBoundsRelation relation)
{
    switch (relation) {
    case DraftingDrawableBoundsRelation::Inside:
        return "inside";
    case DraftingDrawableBoundsRelation::PartiallyOutside:
        return "partially_outside";
    case DraftingDrawableBoundsRelation::FullyOutside:
        return "fully_outside";
    case DraftingDrawableBoundsRelation::TooLarge:
        return "too_large";
    case DraftingDrawableBoundsRelation::Unavailable:
        return "unavailable";
    }
    return "unavailable";
}

DraftingDrawableBoundsRelation classifyBoundsAgainstDrawable(Bounds2D bounds, Bounds2D drawable)
{
    if (!isFinite(bounds) || !isFinite(drawable)) {
        return DraftingDrawableBoundsRelation::Unavailable;
    }
    if (bounds.width > drawable.width || bounds.height > drawable.height) {
        return DraftingDrawableBoundsRelation::TooLarge;
    }
    if (boundsInsideDrawable(bounds, drawable)) {
        return DraftingDrawableBoundsRelation::Inside;
    }

    const bool intersects = bounds.x <= drawable.x + drawable.width
        && bounds.x + bounds.width >= drawable.x
        && bounds.y <= drawable.y + drawable.height
        && bounds.y + bounds.height >= drawable.y;
    return intersects
        ? DraftingDrawableBoundsRelation::PartiallyOutside
        : DraftingDrawableBoundsRelation::FullyOutside;
}

bool boundsInsideDrawable(Bounds2D bounds, Bounds2D drawable)
{
    if (!isFinite(bounds) || !isFinite(drawable)) {
        return false;
    }
    return bounds.x >= drawable.x
        && bounds.y >= drawable.y
        && bounds.x + bounds.width <= drawable.x + drawable.width
        && bounds.y + bounds.height <= drawable.y + drawable.height;
}

Bounds2D translateBounds(Bounds2D bounds, double dx, double dy)
{
    bounds.x += dx;
    bounds.y += dy;
    return bounds;
}

DraftingPlotBoundsResult rawPlotOutputBounds(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings)
{
    const DraftingPlotPlan plotPlan = buildDraftingPlotPlan(document, grid, settings);
    bool hasBounds = false;
    Bounds2D bounds;
    for (const DraftingPlotSegment &segment : plotPlan.segments) {
        if (!isFinite(segment.rawA) || !isFinite(segment.rawB)) {
            continue;
        }
        const Bounds2D segmentBounds = boundsForPoints(segment.rawA, segment.rawB);
        bounds = hasBounds ? includeBounds(bounds, segmentBounds) : segmentBounds;
        hasBounds = true;
    }

    if (!hasBounds) {
        return {};
    }

    const DraftingDrawableBoundsRelation relation = classifyBoundsAgainstDrawable(bounds, grid.drawableBounds);
    return {true, bounds, statusForRelation(relation), relation};
}

DraftingPlotBoundsResult selectedRawPlotOutputBounds(
    const DraftingDocument &document,
    const std::vector<DraftingObjectId> &objectIds,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings)
{
    if (objectIds.empty()) {
        return {};
    }

    bool hasObjectBounds = false;
    Bounds2D selectedObjectBounds;
    for (const DraftingObjectId &objectId : objectIds) {
        const DraftingObject *object = findObject(document, objectId);
        if (object == nullptr) {
            return {};
        }
        if (object->locked || draftingObjectLayerLocked(document, *object) || !objectEffectivelyPlotReady(document, *object)) {
            return {};
        }
        if (!isFinite(object->bounds)) {
            return {};
        }
        selectedObjectBounds = hasObjectBounds ? includeBounds(selectedObjectBounds, object->bounds) : object->bounds;
        hasObjectBounds = true;
    }

    if (!hasObjectBounds) {
        return {};
    }

    const DraftingPlotPlan plotPlan = buildDraftingPlotPlan(document, grid, settings);
    bool hasPlotBounds = false;
    Bounds2D selectedPlotBounds;
    for (const DraftingPlotSegment &segment : plotPlan.segments) {
        if (!containsObjectId(objectIds, segment.objectId)
            || !isFinite(segment.rawA)
            || !isFinite(segment.rawB)) {
            continue;
        }
        const Bounds2D segmentBounds = boundsForPoints(segment.rawA, segment.rawB);
        selectedPlotBounds = hasPlotBounds ? includeBounds(selectedPlotBounds, segmentBounds) : segmentBounds;
        hasPlotBounds = true;
    }

    const Bounds2D bounds = hasPlotBounds ? selectedPlotBounds : selectedObjectBounds;
    const DraftingDrawableBoundsRelation relation = classifyBoundsAgainstDrawable(bounds, grid.drawableBounds);
    return {true, bounds, statusForRelation(relation), relation};
}

} // namespace edi::drafting
