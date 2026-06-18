#include "drafting/DraftingInspectorPlan.h"

#include <cassert>
#include <string>
#include <vector>

using namespace edi::drafting;

namespace {

DraftingInspectorPlan plan(std::string toolId, bool hasSelection,
                           DraftingShapeKind kind = DraftingShapeKind::Point)
{
    return planDraftingInspector({std::move(toolId), hasSelection, kind});
}

// B2-CTX: helpers for the relation-aware branches (supply the new bools only).
DraftingInspectorPlan planConnectionSelected()
{
    DraftingInspectorInput input;
    input.hasConnectionSelection = true;
    return planDraftingInspector(input);
}

DraftingInspectorPlan planPlugAnchor(bool hasSelection = false,
                                     DraftingShapeKind kind = DraftingShapeKind::Point)
{
    DraftingInspectorInput input;
    input.activeIsPlugAnchor = true;
    input.hasSelection = hasSelection;
    input.selectedKind = kind;
    return planDraftingInspector(input);
}

bool sameGroups(const DraftingInspectorPlan &actual, const std::vector<std::string> &expected)
{
    return actual.groupIds == expected;
}

} // namespace

int main()
{
    // Tool options lookup is a table: known tool resolves, unknown is empty.
    // The polygon tool carries the shared radius group besides its own:
    // option groups are per-concern, not per-tool, so tools can share them.
    assert(draftingToolOptionsGroups("regular_polygon_tool")
           == (std::vector<std::string>{"tool_polygon", "tool_radius"}));
    assert(draftingToolOptionsGroups("rectangle_tool") == (std::vector<std::string>{"tool_rectangle"}));
    assert(draftingToolOptionsGroups("circle_tool") == (std::vector<std::string>{"tool_radius"}));
    assert(draftingToolOptionsGroups("arc_tool") == (std::vector<std::string>{"tool_radius"}));
    assert(draftingToolOptionsGroups("line_tool").empty());
    assert(draftingToolOptionsGroups("").empty());

    // Neutral tool, nothing selected: the document context keeps the
    // document-wide controls reachable until F4/F5 give them homes.
    {
        const DraftingInspectorPlan p = plan("select_move", false);
        assert(p.contextId == "document");
        // DM-10: region_fill_document leads the document context (the Fill Region
        // arming verb needs no selection, so it lives in the document-wide controls).
        assert(sameGroups(p, {"region_fill_document", "layers_document", "guides_document",
                              "calibration_document", "document_info", "canvas_state"}));
    }

    // A drawing tool with no options and nothing selected: quiet empty state.
    {
        const DraftingInspectorPlan p = plan("line_tool", false);
        assert(p.contextId == "empty");
        assert(sameGroups(p, {"empty_state"}));
    }

    // A tool with options and nothing selected: only those options.
    {
        const DraftingInspectorPlan p = plan("regular_polygon_tool", false);
        assert(p.contextId == "tool_options");
        assert(sameGroups(p, {"tool_polygon", "tool_radius"}));
    }
    {
        const DraftingInspectorPlan p = plan("circle_tool", false);
        assert(p.contextId == "tool_options");
        assert(sameGroups(p, {"tool_radius"}));
    }

    // Every plain shape kind lands in the shared shape context.
    for (const DraftingShapeKind kind : {DraftingShapeKind::Point, DraftingShapeKind::Line,
                                         DraftingShapeKind::Rectangle, DraftingShapeKind::Circle,
                                         DraftingShapeKind::Arc, DraftingShapeKind::Polygon,
                                         DraftingShapeKind::Polyline}) {
        const DraftingInspectorPlan p = plan("select_move", true, kind);
        assert(p.contextId == "object_shape");
        assert(sameGroups(p, {"selection_summary", "style", "geometry", "transform", "object_guides"}));
    }

    // Kind families with their own controls get their own contexts.
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::Guide);
        assert(p.contextId == "object_guide");
        assert(sameGroups(p, {"selection_summary", "geometry", "guide_position", "guide_visuals"}));
    }
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::ConstructionLine);
        assert(p.contextId == "object_construction");
        assert(sameGroups(p, {"selection_summary", "geometry", "construction", "transform"}));
    }
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::Dimension);
        assert(p.contextId == "object_dimension");
        assert(sameGroups(p, {"selection_summary", "geometry", "dimension"}));
    }

    // Creation auto-selects, so the active tool's options ride along with a
    // selection — the draw loop must keep its options visible.
    {
        const DraftingInspectorPlan p = plan("regular_polygon_tool", true, DraftingShapeKind::Polygon);
        assert(p.contextId == "object_shape");
        assert(sameGroups(p, {"tool_polygon", "tool_radius", "selection_summary", "style", "geometry", "transform", "object_guides"}));
    }
    {
        const DraftingInspectorPlan p = plan("circle_tool", true, DraftingShapeKind::Circle);
        assert(p.contextId == "object_shape");
        assert(sameGroups(p, {"tool_radius", "selection_summary", "style", "geometry", "transform", "object_guides"}));
    }

    // Selection wins over the document context even with the neutral tool.
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::Line);
        assert(p.contextId == "object_shape");
    }

    // B2-CTX: relation-aware branches.
    //
    // (1) hasConnectionSelection=true → object_connection regardless of other fields.
    {
        const DraftingInspectorPlan p = planConnectionSelected();
        assert(p.contextId == "object_connection");
        assert(p.groupIds == (std::vector<std::string>{"connection_summary", "connection_verbs"}));
    }
    //   connection-select wins even when an object is also selected
    {
        DraftingInspectorInput input;
        input.hasConnectionSelection = true;
        input.hasSelection = true;
        input.selectedKind = DraftingShapeKind::Circle;
        const DraftingInspectorPlan p = planDraftingInspector(input);
        assert(p.contextId == "object_connection"); // connection wins
    }

    // (2) activeIsPlugAnchor=true → object_plug (overrides the Point-kind fallback
    //     to object_shape — a plug anchor is a Point, but its relation context wins).
    {
        const DraftingInspectorPlan p = planPlugAnchor(/*hasSelection=*/true, DraftingShapeKind::Point);
        assert(p.contextId == "object_plug");
        assert(p.groupIds == (std::vector<std::string>{"plug_summary", "plug_type", "plug_verbs"}));
    }

    // (3) Regression guard: a plain Point WITH selection and NO relation flags
    //     still lands in object_shape (the kind-branch, not the plug branch).
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::Point);
        assert(p.contextId == "object_shape");
    }
    // And Circle with selection + no relation flags → object_shape (unchanged).
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::Circle);
        assert(p.contextId == "object_shape");
    }

    // (4) Empty input (both bools default false) → empty context (unchanged).
    {
        const DraftingInspectorPlan p = planDraftingInspector({});
        assert(p.contextId == "empty");
    }

    return 0;
}
