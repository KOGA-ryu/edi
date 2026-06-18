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
    // "block_instance" is always in the object_shape group list; refreshInspector
    // adds a second projection-bool gate (has_block_instance_selection) to hide it
    // for non-instance objects. Plan owns context; refresh owns the sub-gate.
    for (const DraftingShapeKind kind : {DraftingShapeKind::Point, DraftingShapeKind::Line,
                                         DraftingShapeKind::Rectangle, DraftingShapeKind::Circle,
                                         DraftingShapeKind::Arc, DraftingShapeKind::Polygon,
                                         DraftingShapeKind::Polyline}) {
        const DraftingInspectorPlan p = plan("select_move", true, kind);
        assert(p.contextId == "object_shape");
        assert(sameGroups(p, {"selection_summary", "style", "geometry", "transform", "object_guides", "block_instance"}));
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
        assert(sameGroups(p, {"tool_polygon", "tool_radius", "selection_summary", "style", "geometry", "transform", "object_guides", "block_instance"}));
    }
    {
        const DraftingInspectorPlan p = plan("circle_tool", true, DraftingShapeKind::Circle);
        assert(p.contextId == "object_shape");
        assert(sameGroups(p, {"tool_radius", "selection_summary", "style", "geometry", "transform", "object_guides", "block_instance"}));
    }

    // Selection wins over the document context even with the neutral tool.
    {
        const DraftingInspectorPlan p = plan("select_move", true, DraftingShapeKind::Line);
        assert(p.contextId == "object_shape");
    }

    return 0;
}
