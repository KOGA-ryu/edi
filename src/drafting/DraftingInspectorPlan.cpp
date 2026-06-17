#include "drafting/DraftingInspectorPlan.h"

namespace edi::drafting {

namespace {

// Variation point as data (per the project rules): which groups a context
// shows is a table edit, never a new branch in the panel code. Order in the
// vector is display order.
struct ContextGroupsRow {
    const char *contextId;
    std::vector<std::string> groupIds;
};

const std::vector<ContextGroupsRow> &contextTable()
{
    static const std::vector<ContextGroupsRow> table = {
        // A drawable shape: identity, numeric geometry, transforms, and the
        // guide-creation helpers that act on the selection's bounds.
        {"object_shape", {"selection_summary", "style", "geometry", "transform", "object_guides"}},
        // Style shows only for kinds whose painter honors it: construction
        // lines and dimensions paint semantic palette colors by design, so
        // offering the controls there would store values nothing reads.
        // A guide edits through its own positioning controls; transforms and
        // bounds-guides do not apply to it.
        {"object_guide", {"selection_summary", "geometry", "guide_position", "guide_visuals"}},
        {"object_construction", {"selection_summary", "geometry", "construction", "transform"}},
        {"object_dimension", {"selection_summary", "geometry", "dimension"}},
        // Interim home for document-wide controls (layers, guide presets,
        // calibration, plot/canvas state): the neutral select tool with
        // nothing selected means "configure the document". These groups move
        // to floating palettes / settings in F4-F5; until then this row keeps
        // them reachable.
        {"document", {"region_fill_document", "layers_document", "guides_document", "calibration_document", "document_info", "canvas_state"}},
        {"empty", {"empty_state"}},
    };
    return table;
}

struct ToolOptionsRow {
    const char *toolId;
    std::vector<std::string> groupIds;
};

const std::vector<ToolOptionsRow> &toolOptionsTable()
{
    static const std::vector<ToolOptionsRow> table = {
        // The polygon tool draws on a circumscribed circle, so it shows the
        // shared radius option alongside its own sides group. One tool, two
        // groups — which is why this is a vector, not a single id.
        {"regular_polygon_tool", {"tool_polygon", "tool_radius"}},
        {"rectangle_tool", {"tool_rectangle"}},
        {"circle_tool", {"tool_radius"}},
        {"arc_tool", {"tool_radius"}},
    };
    return table;
}

std::string contextForKind(DraftingShapeKind kind)
{
    switch (kind) {
    case DraftingShapeKind::Guide: return "object_guide";
    case DraftingShapeKind::ConstructionLine: return "object_construction";
    case DraftingShapeKind::Dimension: return "object_dimension";
    default: return "object_shape";
    }
}

const std::vector<std::string> &groupsForContext(const std::string &contextId)
{
    static const std::vector<std::string> empty;
    for (const ContextGroupsRow &row : contextTable()) {
        if (contextId == row.contextId) {
            return row.groupIds;
        }
    }
    return empty;
}

} // namespace

std::vector<std::string> draftingToolOptionsGroups(const std::string &toolId)
{
    for (const ToolOptionsRow &row : toolOptionsTable()) {
        if (toolId == row.toolId) {
            return row.groupIds;
        }
    }
    return {};
}

DraftingInspectorPlan planDraftingInspector(const DraftingInspectorInput &input)
{
    DraftingInspectorPlan plan;
    const std::vector<std::string> toolOptions = draftingToolOptionsGroups(input.toolId);

    if (input.hasSelection) {
        plan.contextId = contextForKind(input.selectedKind);
    } else if (!toolOptions.empty()) {
        plan.contextId = "tool_options";
    } else if (input.toolId == "select_move") {
        plan.contextId = "document";
    } else {
        plan.contextId = "empty";
    }

    // Creation auto-selects the new object, so a draw loop (set sides, draw,
    // draw again) always has a selection by its second iteration. The active
    // tool's options therefore ride along with any selection context —
    // otherwise the polygon-sides control vanishes after the first polygon.
    plan.groupIds.insert(plan.groupIds.end(), toolOptions.begin(), toolOptions.end());
    const std::vector<std::string> &groups = groupsForContext(plan.contextId);
    plan.groupIds.insert(plan.groupIds.end(), groups.begin(), groups.end());
    return plan;
}

} // namespace edi::drafting
