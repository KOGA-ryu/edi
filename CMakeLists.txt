cmake_minimum_required(VERSION 3.22)

project(EDI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
include(CTest)

set(DRAFTSMAN_SHELL_TARGET_NAME "edi")
set(DRAFTSMAN_SHELL_DATA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Project data root used for default profiles, themes, layouts, and review subjects.")

add_library(drawing_runtime_core STATIC
    src/runtime/DrawingRuntimeCore.h
    src/runtime/DrawingRuntimeCore.cpp
)

target_include_directories(drawing_runtime_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(drawing_runtime_core
    PUBLIC
        Qt6::Core
)

add_library(edi_drafting_core STATIC
    src/drafting/DraftingTypes.h
    src/drafting/DraftingDocument.h
    src/drafting/DraftingDocument.cpp
    src/drafting/DraftingMetadata.h
    src/drafting/DraftingMetadata.cpp
    src/drafting/DraftingStore.h
    src/drafting/DraftingStore.cpp
    src/drafting/DraftingObjectEdit.h
    src/drafting/DraftingObjectEdit.cpp
    src/drafting/DraftingNumericEdit.h
    src/drafting/DraftingNumericEdit.cpp
    src/drafting/DraftingPhysicalEdit.h
    src/drafting/DraftingPhysicalEdit.cpp
    src/drafting/DraftingPhysicalGeometry.h
    src/drafting/DraftingPhysicalGeometry.cpp
    src/drafting/DraftingCommands.h
    src/drafting/DraftingCommands.cpp
    src/drafting/DraftingConstructionOps.h
    src/drafting/DraftingConstructionOps.cpp
    src/drafting/DraftingDimensionOps.h
    src/drafting/DraftingDimensionOps.cpp
    src/drafting/DraftingGeometry.h
    src/drafting/DraftingGeometry.cpp
    src/drafting/DraftingHitTest.h
    src/drafting/DraftingHitTest.cpp
    src/drafting/DraftingSnap.h
    src/drafting/DraftingSnap.cpp
    src/drafting/DraftingGrid.h
    src/drafting/DraftingGrid.cpp
    src/drafting/DraftingGuideOps.h
    src/drafting/DraftingGuideOps.cpp
    src/drafting/DraftingLayerOps.h
    src/drafting/DraftingLayerOps.cpp
    src/drafting/DraftingQuickMeasure.h
    src/drafting/DraftingQuickMeasure.cpp
    src/drafting/DraftingToolCreation.h
    src/drafting/DraftingToolCreation.cpp
    src/drafting/DraftingCalibration.h
    src/drafting/DraftingCalibration.cpp
    src/drafting/DraftingOffset.h
    src/drafting/DraftingOffset.cpp
    src/drafting/DraftingMirror.h
    src/drafting/DraftingMirror.cpp
    src/drafting/DraftingNudgeOps.h
    src/drafting/DraftingNudgeOps.cpp
    src/drafting/DraftingArray.h
    src/drafting/DraftingArray.cpp
    src/drafting/DraftingAlign.h
    src/drafting/DraftingAlign.cpp
    src/drafting/DraftingSelection.h
    src/drafting/DraftingSelection.cpp
    src/drafting/DraftingMeasurement.h
    src/drafting/DraftingMeasurement.cpp
    src/drafting/DraftingMeasurementFormat.h
    src/drafting/DraftingMeasurementFormat.cpp
    src/drafting/DraftingBuildPlan.h
    src/drafting/DraftingBuildPlan.cpp
    src/drafting/DraftingPlotPlan.h
    src/drafting/DraftingPlotPlan.cpp
    src/drafting/DraftingPlotBounds.h
    src/drafting/DraftingPlotBounds.cpp
    src/drafting/DraftingPlotJob.h
    src/drafting/DraftingPlotJob.cpp
    src/drafting/DraftingPlotJobReport.h
    src/drafting/DraftingPlotJobReport.cpp
    src/drafting/DraftingSerialize.h
    src/drafting/DraftingSerialize.cpp
    src/drafting/DraftingSvgOut.h
    src/drafting/DraftingSvgOut.cpp
    src/drafting/DraftingHpglOut.h
    src/drafting/DraftingHpglOut.cpp
)

target_include_directories(edi_drafting_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(edi_text_core STATIC
    src/text/TextDocument.h
    src/text/TextDocument.cpp
    src/text/TextDocumentStore.h
    src/text/TextDocumentStore.cpp
    src/text/TextEditorCommands.h
    src/text/TextEditorCommands.cpp
    src/text/TextSelection.h
    src/text/TextSelection.cpp
)

target_include_directories(edi_text_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(edi_app_core STATIC
    src/app/AppState.h
    src/app/AppState.cpp
    src/app/ProjectWorkspace.h
    src/app/ProjectWorkspace.cpp
)

target_include_directories(edi_app_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(edi_app_core
    PUBLIC
        edi_drafting_core
        edi_text_core
)

add_library(edi_format_core STATIC
    src/formats/FormatResult.h
    src/formats/FormatResult.cpp
    src/formats/TomlReader.h
    src/formats/TomlReader.cpp
    src/formats/TomlWriter.h
    src/formats/TomlWriter.cpp
    src/formats/ToonExport.h
    src/formats/ToonExport.cpp
    src/formats/MessagePackReader.h
    src/formats/MessagePackReader.cpp
    src/formats/MessagePackWriter.h
    src/formats/MessagePackWriter.cpp
    src/formats/MessagePackInspector.h
    src/formats/MessagePackInspector.cpp
    src/formats/MessagePackValue.h
    src/formats/MessagePackValue.cpp
)

target_include_directories(edi_format_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# The drafting core's document serializer (DraftingSerialize) builds on the
# MessagePack value codec, so it links the format core.
target_link_libraries(edi_drafting_core PUBLIC edi_format_core)

add_library(edi_scripting_core STATIC
    src/scripting/LuaRecipe.h
    src/scripting/LuaRecipe.cpp
    src/scripting/ScriptCommandBridge.h
    src/scripting/ScriptCommandBridge.cpp
)

target_include_directories(edi_scripting_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

function(add_edi_contract_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE ${ARGN})
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_edi_contract_test(drafting_store_tests tests/drafting_store_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_serialize_tests tests/drafting_serialize_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_document_query_tests tests/drafting_document_query_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_commands_tests tests/drafting_commands_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_construction_ops_tests tests/drafting_construction_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_dimension_ops_tests tests/drafting_dimension_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_geometry_tests tests/drafting_geometry_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_arc_tests tests/drafting_arc_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_hit_test_tests tests/drafting_hit_test_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_snap_tests tests/drafting_snap_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_grid_tests tests/drafting_grid_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_guide_ops_tests tests/drafting_guide_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_layer_ops_tests tests/drafting_layer_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_selection_tests tests/drafting_selection_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_quick_measure_tests tests/drafting_quick_measure_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_tool_creation_tests tests/drafting_tool_creation_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_calibration_tests tests/drafting_calibration_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_offset_tests tests/drafting_offset_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_mirror_tests tests/drafting_mirror_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_nudge_ops_tests tests/drafting_nudge_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_array_tests tests/drafting_array_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_align_tests tests/drafting_align_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_numeric_edit_tests tests/drafting_numeric_edit_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_physical_edit_tests tests/drafting_physical_edit_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_physical_geometry_tests tests/drafting_physical_geometry_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_object_edit_tests tests/drafting_object_edit_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_metadata_tests tests/drafting_metadata_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_measurement_tests tests/drafting_measurement_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_measurement_format_tests tests/drafting_measurement_format_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_build_plan_tests tests/drafting_build_plan_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_plot_plan_tests tests/drafting_plot_plan_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_plot_bounds_tests tests/drafting_plot_bounds_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_plot_job_tests tests/drafting_plot_job_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_svg_out_tests tests/drafting_svg_out_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_hpgl_out_tests tests/drafting_hpgl_out_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_plot_job_report_tests tests/drafting_plot_job_report_tests.cpp edi_drafting_core)
add_edi_contract_test(drawing_canvas_gesture_state_tests tests/drawing_canvas_gesture_state_tests.cpp Qt6::Core)
target_sources(drawing_canvas_gesture_state_tests PRIVATE src/widgets/DrawingCanvasGestureState.cpp src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_gesture_state_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_viewport_tests tests/drawing_canvas_viewport_tests.cpp Qt6::Core)
target_sources(drawing_canvas_viewport_tests PRIVATE src/widgets/DrawingCanvasViewport.cpp)
target_include_directories(drawing_canvas_viewport_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_handle_hit_test_tests tests/drawing_canvas_handle_hit_test_tests.cpp Qt6::Core)
target_sources(drawing_canvas_handle_hit_test_tests PRIVATE
    src/widgets/DrawingCanvasHandleHitTest.cpp
    src/widgets/DrawingCanvasValues.cpp
    src/widgets/DrawingCanvasProjectedObject.cpp
    src/widgets/DrawingCanvasGestureState.cpp
    src/widgets/DrawingCanvasViewport.cpp
)
target_include_directories(drawing_canvas_handle_hit_test_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_projected_document_tests tests/drawing_canvas_projected_document_tests.cpp Qt6::Core)
target_sources(drawing_canvas_projected_document_tests PRIVATE
    src/widgets/DrawingCanvasProjectedDocument.cpp
    src/widgets/DrawingCanvasValues.cpp
    src/widgets/DrawingCanvasProjectedObject.cpp
    src/widgets/DrawingCanvasGestureState.cpp
)
target_include_directories(drawing_canvas_projected_document_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_projected_plot_tests tests/drawing_canvas_projected_plot_tests.cpp Qt6::Core)
target_sources(drawing_canvas_projected_plot_tests PRIVATE src/widgets/DrawingCanvasProjectedPlot.cpp src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_projected_plot_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_projected_grid_tests tests/drawing_canvas_projected_grid_tests.cpp Qt6::Core)
target_sources(drawing_canvas_projected_grid_tests PRIVATE src/widgets/DrawingCanvasProjectedGrid.cpp src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_projected_grid_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_projected_status_tests tests/drawing_canvas_projected_status_tests.cpp Qt6::Core)
target_sources(drawing_canvas_projected_status_tests PRIVATE src/widgets/DrawingCanvasProjectedStatus.cpp src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_projected_status_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_projected_pointer_tests tests/drawing_canvas_projected_pointer_tests.cpp Qt6::Core)
target_sources(drawing_canvas_projected_pointer_tests PRIVATE src/widgets/DrawingCanvasProjectedPointer.cpp src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_projected_pointer_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_projected_object_tests tests/drawing_canvas_projected_object_tests.cpp Qt6::Core)
target_sources(drawing_canvas_projected_object_tests PRIVATE src/widgets/DrawingCanvasProjectedObject.cpp src/widgets/DrawingCanvasGestureState.cpp src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_projected_object_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(app_workspace_tests tests/app_workspace_tests.cpp edi_app_core)
add_edi_contract_test(text_document_tests tests/text_document_tests.cpp edi_text_core)
add_edi_contract_test(text_editor_commands_tests tests/text_editor_commands_tests.cpp edi_text_core)
add_edi_contract_test(format_toml_tests tests/format_toml_tests.cpp edi_format_core)
add_edi_contract_test(format_messagepack_tests tests/format_messagepack_tests.cpp edi_format_core)
add_edi_contract_test(format_messagepack_value_tests tests/format_messagepack_value_tests.cpp edi_format_core)
add_edi_contract_test(shell_theme_tests tests/shell_theme_tests.cpp Qt6::Core)
target_sources(shell_theme_tests PRIVATE src/widgets/ShellTheme.cpp)
target_include_directories(shell_theme_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(drawing_canvas_palette_tests tests/drawing_canvas_palette_tests.cpp Qt6::Core Qt6::Gui)
target_sources(drawing_canvas_palette_tests PRIVATE src/widgets/DrawingCanvasPalette.cpp src/widgets/ShellTheme.cpp)
target_include_directories(drawing_canvas_palette_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(shell_host_tests tests/shell_host_tests.cpp Qt6::Core Qt6::Gui Qt6::Widgets)
target_sources(shell_host_tests PRIVATE src/widgets/ShellHost.cpp)
target_include_directories(shell_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(shell_panels_tests tests/shell_panels_tests.cpp Qt6::Core)
target_sources(shell_panels_tests PRIVATE src/widgets/ShellPanels.cpp src/widgets/ShellHost.cpp)
target_include_directories(shell_panels_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(settings_store_tests tests/settings_store_tests.cpp edi_format_core Qt6::Core)
target_sources(settings_store_tests PRIVATE src/io/SettingsStore.cpp)
target_include_directories(settings_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(shell_layout_store_tests tests/shell_layout_store_tests.cpp edi_format_core Qt6::Core)
target_sources(shell_layout_store_tests PRIVATE src/io/ShellLayoutStore.cpp src/io/SettingsStore.cpp src/widgets/ShellHost.cpp src/widgets/ShellPanels.cpp)
target_include_directories(shell_layout_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(scripting_bridge_tests tests/scripting_bridge_tests.cpp edi_scripting_core)

add_executable(edi_plot_job_report
    src/tools/EdiPlotJobReport.cpp
)
target_include_directories(edi_plot_job_report PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(edi_plot_job_report PRIVATE
    edi_drafting_core
)
add_test(
    NAME edi_plot_job_report_cli_smoke
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/plot_job_report_cli_smoke.sh $<TARGET_FILE:edi_plot_job_report>
)

add_executable(drawing_document_controller_tests
    tests/drawing_document_controller_tests.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
)
target_include_directories(drawing_document_controller_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(drawing_document_controller_tests PRIVATE
    edi_drafting_core
    Qt6::Core
)
add_test(NAME drawing_document_controller_tests COMMAND drawing_document_controller_tests)

add_executable(edi_shell_window_tests
    tests/edi_shell_window_tests.cpp
    src/io/ShellLayoutStore.h
    src/io/ShellLayoutStore.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/SettingsStore.h
    src/io/SettingsStore.cpp
    src/widgets/EdiShellWindow.h
    src/widgets/EdiShellWindow.cpp
    src/widgets/EdiShellWindowPanels.cpp
    src/widgets/EdiShellWindowInspector.cpp
    src/widgets/EdiShellWindowIo.cpp
    src/widgets/ShellWidgetHelpers.h
    src/widgets/ShellWidgetHelpers.cpp
    src/widgets/ShellTheme.h
    src/widgets/ShellTheme.cpp
    src/widgets/ShellHost.h
    src/widgets/ShellHost.cpp
    src/widgets/ShellPanels.h
    src/widgets/ShellPanels.cpp
    src/widgets/DrawingCanvasWidget.h
    src/widgets/DrawingCanvasWidget.cpp
    src/widgets/DrawingCanvasPalette.h
    src/widgets/DrawingCanvasPalette.cpp
    src/widgets/DrawingCanvasViewport.h
    src/widgets/DrawingCanvasViewport.cpp
    src/widgets/DrawingCanvasHandleHitTest.h
    src/widgets/DrawingCanvasHandleHitTest.cpp
    src/widgets/DrawingCanvasProjectedDocument.h
    src/widgets/DrawingCanvasProjectedDocument.cpp
    src/widgets/DrawingCanvasProjectedPlot.h
    src/widgets/DrawingCanvasProjectedPlot.cpp
    src/widgets/DrawingCanvasProjectedGrid.h
    src/widgets/DrawingCanvasProjectedGrid.cpp
    src/widgets/DrawingCanvasProjectedStatus.h
    src/widgets/DrawingCanvasProjectedStatus.cpp
    src/widgets/DrawingCanvasProjectedPointer.h
    src/widgets/DrawingCanvasProjectedPointer.cpp
    src/widgets/DrawingCanvasObjectPainter.h
    src/widgets/DrawingCanvasObjectPainter.cpp
    src/widgets/DrawingCanvasGestureState.h
    src/widgets/DrawingCanvasGestureState.cpp
    src/widgets/DrawingCanvasProjectedObject.h
    src/widgets/DrawingCanvasProjectedObject.cpp
    src/widgets/DrawingCanvasValues.h
    src/widgets/DrawingCanvasValues.cpp
)
target_include_directories(edi_shell_window_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(edi_shell_window_tests PRIVATE
    edi_app_core
    edi_drafting_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
add_test(NAME edi_shell_window_tests COMMAND edi_shell_window_tests)

add_executable(drawing_canvas_widget_tests
    tests/drawing_canvas_widget_tests.cpp
    src/widgets/ShellTheme.h
    src/widgets/ShellTheme.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/widgets/DrawingCanvasWidget.h
    src/widgets/DrawingCanvasWidget.cpp
    src/widgets/DrawingCanvasPalette.h
    src/widgets/DrawingCanvasPalette.cpp
    src/widgets/DrawingCanvasViewport.h
    src/widgets/DrawingCanvasViewport.cpp
    src/widgets/DrawingCanvasHandleHitTest.h
    src/widgets/DrawingCanvasHandleHitTest.cpp
    src/widgets/DrawingCanvasProjectedDocument.h
    src/widgets/DrawingCanvasProjectedDocument.cpp
    src/widgets/DrawingCanvasProjectedPlot.h
    src/widgets/DrawingCanvasProjectedPlot.cpp
    src/widgets/DrawingCanvasProjectedGrid.h
    src/widgets/DrawingCanvasProjectedGrid.cpp
    src/widgets/DrawingCanvasProjectedStatus.h
    src/widgets/DrawingCanvasProjectedStatus.cpp
    src/widgets/DrawingCanvasProjectedPointer.h
    src/widgets/DrawingCanvasProjectedPointer.cpp
    src/widgets/DrawingCanvasObjectPainter.h
    src/widgets/DrawingCanvasObjectPainter.cpp
    src/widgets/DrawingCanvasGestureState.h
    src/widgets/DrawingCanvasGestureState.cpp
    src/widgets/DrawingCanvasProjectedObject.h
    src/widgets/DrawingCanvasProjectedObject.cpp
    src/widgets/DrawingCanvasValues.h
    src/widgets/DrawingCanvasValues.cpp
)
target_include_directories(drawing_canvas_widget_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(drawing_canvas_widget_tests PRIVATE
    edi_drafting_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
add_test(NAME drawing_canvas_widget_tests COMMAND drawing_canvas_widget_tests)

add_executable(${DRAFTSMAN_SHELL_TARGET_NAME}
    app/main.cpp
    src/core/DrawingCore.h
    src/core/DrawingCoreInternal.h
    src/core/DrawingObjectModel.h
    src/core/DrawingObjectModel.cpp
    src/core/DrawingCore.cpp
    src/core/DrawingCoreCommon.cpp
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/core/DrawingGeometry.cpp
    src/core/DrawingObjectOps.cpp
    src/core/DrawingCommands.cpp
    src/core/DrawingModelBuilder.cpp
    src/core/DrawingSvgExport.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/SettingsStore.h
    src/io/SettingsStore.cpp
    src/io/DrawingRecentFilesStore.h
    src/io/DrawingRecentFilesStore.cpp
    src/io/ShellLayoutStore.h
    src/io/ShellLayoutStore.cpp
    src/io/TextEditorStore.h
    src/io/TextEditorStore.cpp
    src/widgets/EdiShellWindow.h
    src/widgets/EdiShellWindow.cpp
    src/widgets/EdiShellWindowPanels.cpp
    src/widgets/EdiShellWindowInspector.cpp
    src/widgets/EdiShellWindowIo.cpp
    src/widgets/ShellWidgetHelpers.h
    src/widgets/ShellWidgetHelpers.cpp
    src/widgets/ShellTheme.h
    src/widgets/ShellTheme.cpp
    src/widgets/ShellHost.h
    src/widgets/ShellHost.cpp
    src/widgets/ShellPanels.h
    src/widgets/ShellPanels.cpp
    src/widgets/DrawingCanvasWidget.h
    src/widgets/DrawingCanvasWidget.cpp
    src/widgets/DrawingCanvasPalette.h
    src/widgets/DrawingCanvasPalette.cpp
    src/widgets/DrawingCanvasViewport.h
    src/widgets/DrawingCanvasViewport.cpp
    src/widgets/DrawingCanvasHandleHitTest.h
    src/widgets/DrawingCanvasHandleHitTest.cpp
    src/widgets/DrawingCanvasProjectedDocument.h
    src/widgets/DrawingCanvasProjectedDocument.cpp
    src/widgets/DrawingCanvasProjectedPlot.h
    src/widgets/DrawingCanvasProjectedPlot.cpp
    src/widgets/DrawingCanvasProjectedGrid.h
    src/widgets/DrawingCanvasProjectedGrid.cpp
    src/widgets/DrawingCanvasProjectedStatus.h
    src/widgets/DrawingCanvasProjectedStatus.cpp
    src/widgets/DrawingCanvasProjectedPointer.h
    src/widgets/DrawingCanvasProjectedPointer.cpp
    src/widgets/DrawingCanvasObjectPainter.h
    src/widgets/DrawingCanvasObjectPainter.cpp
    src/widgets/DrawingCanvasGestureState.h
    src/widgets/DrawingCanvasGestureState.cpp
    src/widgets/DrawingCanvasProjectedObject.h
    src/widgets/DrawingCanvasProjectedObject.cpp
    src/widgets/DrawingCanvasValues.h
    src/widgets/DrawingCanvasValues.cpp
)

target_include_directories(${DRAFTSMAN_SHELL_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${DRAFTSMAN_SHELL_TARGET_NAME} PRIVATE
    PROJECT_SOURCE_DIR="${DRAFTSMAN_SHELL_DATA_ROOT_DIR}"
)

target_link_libraries(${DRAFTSMAN_SHELL_TARGET_NAME}
    PRIVATE
        drawing_runtime_core
        edi_app_core
        edi_drafting_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
