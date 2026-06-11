#pragma once

#include "drafting/DraftingTypes.h"

#include <string>
#include <vector>

namespace edi::drafting {

// What the inspector needs in order to pick a context: the active tool and
// what (if anything) is selected. Plain data in, plain data out — the widget
// layer supplies these from the controller and only toggles visibility.
struct DraftingInspectorInput {
    std::string toolId;
    bool hasSelection = false;
    // Read only when hasSelection is true.
    DraftingShapeKind selectedKind = DraftingShapeKind::Point;
};

// The resolved context plus the ordered list of section groups to show.
// Group ids name widget containers the panel builds once; everything not in
// the list is hidden. The context id exists for tests and for debugging —
// the widget layer keys off groupIds alone.
struct DraftingInspectorPlan {
    std::string contextId;
    std::vector<std::string> groupIds;
};

// toolId -> the options groups for that tool (display order); empty when the
// tool has none. A lookup, not a switch in the widget code, so F6 (belt
// configuration) and new tools extend a table instead of editing panel logic.
// A vector because tools share option groups: the polygon tool shows its
// sides group AND the radius group the circle/arc tools use.
std::vector<std::string> draftingToolOptionsGroups(const std::string &toolId);

DraftingInspectorPlan planDraftingInspector(const DraftingInspectorInput &input);

} // namespace edi::drafting
