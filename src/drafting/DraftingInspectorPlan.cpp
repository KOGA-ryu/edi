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
        // B2-CTX: relation-aware contexts. These win over the kind branch when the
        // widget layer sets the corresponding bool in DraftingInspectorInput.
        // groupIds name WIDGET containers edi-ui builds; these names are a
        // dungeon-map ↔ edi-ui coordination point (provisional until edi-ui confirms).
        {"object_connection", {"connection_summary", "connection_verbs"}},    // delete + re-route verbs
        {"object_plug",       {"plug_summary", "plug_type", "plug_verbs"}},   // door-type picker + delete
        // A drawable shape: identity, numeric geometry, transforms, and the
        // guide-creation helpers that act on the selection's bounds.
        // "block_instance" is in this list so its container is visible
        // whenever an object is selected; refreshInspector then adds a
        // second gate on has_block_instance_selection to hide the group
        // for non-instance objects. Two-gate pattern: plan owns context,
        // refreshInspector owns the projection bool sub-gate.
        {"object_shape", {"selection_summary", "style", "geometry", "transform", "object_guides", "block_instance"}},
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

    // B2-CTX: relation branches sit ABOVE the kind branch so they fire first for
    // map-authoring selections. Both bools default false, so every non-map call
    // falls through to the existing kind / tool / document / empty chain —
    // no regression to existing contexts (the key invariant).
    //
    // Why this order: a connection row-click is an EXPLICIT relation intent and
    // wins over everything; an active plug-anchor object is more specific than its
    // raw Point kind (which would fall to object_shape). Mutual exclusion is
    // maintained by the controller: selectConnection clears the object selection
    // and any object-select / begin*Pick / setSelectedToolId clears
    // m_activeConnectionId — so both bools are never true simultaneously.
    if      (input.hasConnectionSelection)  plan.contextId = "object_connection";
    else if (input.activeIsPlugAnchor)      plan.contextId = "object_plug";
    else if (input.hasSelection)            plan.contextId = contextForKind(input.selectedKind);
    else if (!toolOptions.empty())          plan.contextId = "tool_options";
    else if (input.toolId == "select_move") plan.contextId = "document";
    else                                    plan.contextId = "empty";

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
