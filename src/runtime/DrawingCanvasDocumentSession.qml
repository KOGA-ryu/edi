import QtQuick

QtObject {
    id: canvasDocumentSession

    function drawingCanvasObjects(session, unusedRevision) {
        var objects = [
            { id: "artboard_bounds", label: "Artboard bounds", kind: "rect", layer_id: "layer_00_canvas", detail: "normalized square artboard" },
            { id: "major_grid", label: "Major grid", kind: "grid", layer_id: "layer_01_grid", detail: session.drawingGridMode + " / " + String(session.drawingGridDivisions) + " divisions" }
        ]
        var generated = session.asArray(session.drawingGeneratedObjects)
        for (var index = 0; index < generated.length; ++index) {
            objects.push(generated[index])
        }
        return objects
    }

    function drawingCanvasDocument(session, unusedRevision) {
        return {
            canvas_id: "pattern_lab_2d_native_canvas_v0",
            coordinate_space: "normalized_artboard",
            selected_tool_id: session.selectedDrawingToolId,
            selected_variant_id: session.selectedDrawingVariantId,
            selected_layer_id: session.selectedDrawingLayerId,
            selected_object_id: session.selectedDrawingObjectId,
            selected_object_ids: session.selectedDrawingObjectIds,
            pending_point: session.drawingPendingPoint,
            layers: [
                { id: "layer_00_canvas", visible: true, objects: [
                    { id: "artboard_bounds", kind: "rect", border_visible: session.drawingArtboardBorderVisible }
                ] },
                { id: "layer_01_grid", visible: session.drawingGridVisible, objects: [
                    {
                        id: "major_grid",
                        kind: "grid",
                        mode: session.drawingGridMode,
                        divisions: session.drawingGridDivisions,
                        major_every: session.drawingGridMajorEvery,
                        ascii_cell_grid_visible: session.drawingAsciiCellGridVisible,
                        ascii_columns: session.drawingAsciiColumns,
                        ascii_rows: session.drawingAsciiRows,
                        ascii_major_every: session.drawingAsciiMajorEvery,
                        center_axes_visible: session.drawingCenterAxesVisible,
                        diagonal_guides_visible: session.drawingDiagonalGuidesVisible,
                        radial_guides_visible: session.drawingRadialGuidesVisible,
                        radial_guide_count: session.drawingRadialGuideCount
                    }
                ] },
                { id: "layer_09_script_geometry", visible: true, objects: session.asArray(session.drawingGeneratedObjects) },
                { id: "layer_08_metadata", visible: false, objects: [
                    { id: "last_script_status", kind: "metadata", value: session.drawingLastScriptStatus }
                ] }
            ]
        }
    }

    function drawingCanvasExportDocument(session, unusedRevision) {
        if (session.drawingNativeController) {
            return session.drawingNativeController.modelDocument()
        }
        var toolParameters = {
            circle_arc_mode: session.drawingCircleArcMode,
            circle_arc_start_angle_deg: session.drawingCircleArcStartAngleDeg,
            circle_arc_end_angle_deg: session.drawingCircleArcEndAngleDeg,
            regular_polygon_sides: session.drawingRegularPolygonSides,
            regular_polygon_rotation_deg: session.drawingRegularPolygonRotationDeg,
            line_variant: session.drawingLineVariant,
            line_thickness: session.drawingLineThickness,
            line_style: session.drawingLineStyle,
            stroke_opacity: session.drawingStrokeOpacity,
            stroke_color: session.drawingStrokeColor,
            fill_color: session.drawingFillColor
        }
        return {
            export_kind: "pattern_lab_2d_native_model_v0",
            script_id: session.drawingLastScriptId,
            script_status: session.drawingLastScriptStatus,
            script_errors: session.drawingLastScriptErrors,
            canvas_px: [session.drawingCanvasSizePx, session.drawingCanvasSizePx],
            snap: {
                grid_enabled: session.drawingSnapGridEnabled,
                grid_step_px: session.drawingSnapGridStepPx
            },
            selected_tool_id: session.selectedDrawingToolId,
            selected_variant_id: session.selectedDrawingVariantId,
            selected_layer_id: session.selectedDrawingLayerId,
            selected_object_id: session.selectedDrawingObjectId,
            selected_object_ids: session.selectedDrawingObjectIds,
            tool_parameters: toolParameters,
            drawing_style: {
                selected_variant_id: session.selectedDrawingVariantId,
                line_variant: session.drawingLineVariant,
                line_thickness: session.drawingLineThickness,
                line_style: session.drawingLineStyle,
                stroke_opacity: session.drawingStrokeOpacity,
                stroke_color: session.drawingStrokeColor,
                fill_color: session.drawingFillColor,
                circle_arc_mode: session.drawingCircleArcMode
            },
            generated_objects: session.asArray(session.drawingGeneratedObjects),
            object_counts: drawingObjectCounts(session, session.revision),
            validation: session.drawingModelValidationRows(session.revision)
        }
    }

    function drawingCanvasExportJson(session, unusedRevision) {
        if (session.drawingNativeController) {
            return session.drawingNativeController.exportJson()
        }
        return JSON.stringify(drawingCanvasExportDocument(session, session.revision), null, 2) + "\n"
    }

    function drawingObjectCounts(session, unusedRevision) {
        var counts = ({})
        var objects = drawingCanvasObjects(session, session.revision)
        for (var index = 0; index < objects.length; ++index) {
            var kind = String(objects[index].kind || "unknown")
            counts[kind] = Number(counts[kind] || 0) + 1
        }
        return counts
    }
}
