#include "drafting/DraftingAlign.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace edi::drafting {

namespace {

struct AlignmentTarget {
    DraftingObjectId objectId;
    Bounds2D bounds;
};

constexpr double kEpsilon = 0.000001;

bool nearlyZero(double value)
{
    return std::abs(value) <= kEpsilon;
}

double left(Bounds2D bounds)
{
    return bounds.x;
}

double right(Bounds2D bounds)
{
    return bounds.x + bounds.width;
}

double top(Bounds2D bounds)
{
    return bounds.y;
}

double bottom(Bounds2D bounds)
{
    return bounds.y + bounds.height;
}

double centerX(Bounds2D bounds)
{
    return bounds.x + bounds.width / 2.0;
}

double centerY(Bounds2D bounds)
{
    return bounds.y + bounds.height / 2.0;
}

Bounds2D selectionBounds(const std::vector<AlignmentTarget> &targets)
{
    Bounds2D bounds = targets.front().bounds;
    double minX = left(bounds);
    double minY = top(bounds);
    double maxX = right(bounds);
    double maxY = bottom(bounds);

    for (const AlignmentTarget &target : targets) {
        minX = std::min(minX, left(target.bounds));
        minY = std::min(minY, top(target.bounds));
        maxX = std::max(maxX, right(target.bounds));
        maxY = std::max(maxY, bottom(target.bounds));
    }
    return {minX, minY, maxX - minX, maxY - minY};
}

bool isDistributeMode(DraftingAlignmentMode mode)
{
    return mode == DraftingAlignmentMode::DistributeX || mode == DraftingAlignmentMode::DistributeY;
}

DraftingAlignmentResult collectTargets(
    const DraftingDocument &document,
    const std::vector<DraftingObjectId> &objectIds,
    std::vector<AlignmentTarget> &targets)
{
    if (objectIds.size() < 2) {
        return DraftingAlignmentResult::rejected(DraftingResultCode::InvalidSelectionTarget, "arrange requires at least two objects");
    }

    targets.reserve(objectIds.size());
    for (const DraftingObjectId &objectId : objectIds) {
        const DraftingObject *object = findObject(document, objectId);
        if (object == nullptr) {
            return DraftingAlignmentResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target does not exist");
        }
        const DraftingLayer *layer = findLayer(document, object->layerId);
        if (layer == nullptr) {
            return DraftingAlignmentResult::rejected(DraftingResultCode::LayerNotFound, "selection target layer does not exist");
        }
        if (object->locked || layer->locked) {
            return DraftingAlignmentResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target is locked");
        }
        if (!object->visible || !kindMatchesGeometry(object->kind, object->geometry) || !isFinite(object->bounds)) {
            return DraftingAlignmentResult::rejected(DraftingResultCode::InvalidSelectionTarget, "selection target is not arrangeable");
        }
        targets.push_back({object->id, object->bounds});
    }
    return DraftingAlignmentResult::accepted({});
}

void pushTranslation(std::vector<DraftingTranslation> &translations, const DraftingObjectId &objectId, double dx, double dy)
{
    if (nearlyZero(dx) && nearlyZero(dy)) {
        return;
    }
    translations.push_back({objectId, dx, dy});
}

DraftingAlignmentResult planDistribute(std::vector<AlignmentTarget> targets, DraftingAlignmentMode mode)
{
    if (targets.size() < 3) {
        return DraftingAlignmentResult::rejected(DraftingResultCode::InvalidSelectionTarget, "distribute requires at least three objects");
    }

    const bool horizontal = mode == DraftingAlignmentMode::DistributeX;
    std::sort(targets.begin(), targets.end(), [horizontal](const AlignmentTarget &a, const AlignmentTarget &b) {
        const double primaryA = horizontal ? centerX(a.bounds) : centerY(a.bounds);
        const double primaryB = horizontal ? centerX(b.bounds) : centerY(b.bounds);
        if (primaryA == primaryB) {
            return a.objectId < b.objectId;
        }
        return primaryA < primaryB;
    });

    const double first = horizontal ? centerX(targets.front().bounds) : centerY(targets.front().bounds);
    const double last = horizontal ? centerX(targets.back().bounds) : centerY(targets.back().bounds);
    const double spacing = (last - first) / static_cast<double>(targets.size() - 1);

    std::vector<DraftingTranslation> translations;
    for (std::size_t index = 1; index + 1 < targets.size(); ++index) {
        const double desired = first + spacing * static_cast<double>(index);
        const double current = horizontal ? centerX(targets[index].bounds) : centerY(targets[index].bounds);
        if (horizontal) {
            pushTranslation(translations, targets[index].objectId, desired - current, 0.0);
        } else {
            pushTranslation(translations, targets[index].objectId, 0.0, desired - current);
        }
    }
    return DraftingAlignmentResult::accepted(std::move(translations));
}

} // namespace

DraftingAlignmentResult DraftingAlignmentResult::accepted(std::vector<DraftingTranslation> translations)
{
    DraftingAlignmentResult result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.translations = std::move(translations);
    return result;
}

DraftingAlignmentResult DraftingAlignmentResult::rejected(DraftingResultCode code, std::string message)
{
    DraftingAlignmentResult result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

const char *draftingAlignmentModeName(DraftingAlignmentMode mode)
{
    switch (mode) {
    case DraftingAlignmentMode::Left:
        return "left";
    case DraftingAlignmentMode::Right:
        return "right";
    case DraftingAlignmentMode::Top:
        return "top";
    case DraftingAlignmentMode::Bottom:
        return "bottom";
    case DraftingAlignmentMode::CenterX:
        return "center_x";
    case DraftingAlignmentMode::CenterY:
        return "center_y";
    case DraftingAlignmentMode::DistributeX:
        return "distribute_x";
    case DraftingAlignmentMode::DistributeY:
        return "distribute_y";
    }
    return "unknown";
}

DraftingAlignmentResult planDraftingAlignment(
    const DraftingDocument &document,
    const std::vector<DraftingObjectId> &objectIds,
    DraftingAlignmentMode mode)
{
    std::vector<AlignmentTarget> targets;
    DraftingAlignmentResult targetResult = collectTargets(document, objectIds, targets);
    if (!targetResult.ok) {
        return targetResult;
    }
    if (isDistributeMode(mode)) {
        return planDistribute(std::move(targets), mode);
    }

    const Bounds2D anchor = selectionBounds(targets);
    std::vector<DraftingTranslation> translations;
    translations.reserve(targets.size());
    for (const AlignmentTarget &target : targets) {
        switch (mode) {
        case DraftingAlignmentMode::Left:
            pushTranslation(translations, target.objectId, left(anchor) - left(target.bounds), 0.0);
            break;
        case DraftingAlignmentMode::Right:
            pushTranslation(translations, target.objectId, right(anchor) - right(target.bounds), 0.0);
            break;
        case DraftingAlignmentMode::Top:
            pushTranslation(translations, target.objectId, 0.0, top(anchor) - top(target.bounds));
            break;
        case DraftingAlignmentMode::Bottom:
            pushTranslation(translations, target.objectId, 0.0, bottom(anchor) - bottom(target.bounds));
            break;
        case DraftingAlignmentMode::CenterX:
            pushTranslation(translations, target.objectId, centerX(anchor) - centerX(target.bounds), 0.0);
            break;
        case DraftingAlignmentMode::CenterY:
            pushTranslation(translations, target.objectId, 0.0, centerY(anchor) - centerY(target.bounds));
            break;
        case DraftingAlignmentMode::DistributeX:
        case DraftingAlignmentMode::DistributeY:
            break;
        }
    }
    return DraftingAlignmentResult::accepted(std::move(translations));
}

} // namespace edi::drafting
