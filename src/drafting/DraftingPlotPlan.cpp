#include "drafting/DraftingPlotPlan.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace edi::drafting {
namespace {

int layerOrderForObject(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return layer == nullptr ? std::numeric_limits<int>::max() : layer->order;
}

} // namespace

bool draftingShapeCanPlot(DraftingShapeKind kind)
{
    return kind != DraftingShapeKind::Guide
        && kind != DraftingShapeKind::ConstructionLine
        && kind != DraftingShapeKind::Dimension;
}

DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid)
{
    DraftingPlotPlan plan;

    std::vector<const DraftingObject *> sortedObjects;
    sortedObjects.reserve(document.objects.size());
    for (const DraftingObject &object : document.objects) {
        sortedObjects.push_back(&object);
    }
    std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [&](const DraftingObject *a, const DraftingObject *b) {
        return layerOrderForObject(document, *a) < layerOrderForObject(document, *b);
    });

    for (const DraftingObject *objectPointer : sortedObjects) {
        const DraftingObject &object = *objectPointer;
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !layer->visible || !layer->plot.plotEnabled || !object.visible || !draftingShapeCanPlot(object.kind)) {
            continue;
        }

        plan.objects.push_back({
            object.id,
            object.layerId,
            layer->plot.penId,
            layer->plot.strokeColor,
            layer->plot.strokeWidth,
        });

        if (boundsOutsideDrawableArea(object.bounds, grid)) {
            plan.warnings.push_back({
                object.id,
                "out_of_drawable_bounds",
                "plot object is outside drawable bounds",
            });
        }
    }

    return plan;
}

} // namespace edi::drafting
