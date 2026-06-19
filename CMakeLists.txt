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
    src/drafting/DraftingClipboard.h
    src/drafting/DraftingClipboard.cpp
    src/drafting/DraftingInspectorPlan.h
    src/drafting/DraftingInspectorPlan.cpp
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
    src/drafting/DraftingDerived.h
    src/drafting/DraftingDerived.cpp
    src/drafting/DraftingMirror.h
    src/drafting/DraftingMirror.cpp
    src/drafting/DraftingModify.h
    src/drafting/DraftingModify.cpp
    src/drafting/DraftingNudgeOps.h
    src/drafting/DraftingNudgeOps.cpp
    src/drafting/DraftingArray.h
    src/drafting/DraftingArray.cpp
    src/drafting/DraftingRoom.h
    src/drafting/DraftingRoom.cpp
    src/drafting/DraftingAsciiMap.h
    src/drafting/DraftingAsciiMap.cpp
    src/drafting/DraftingGraphOps.h
    src/drafting/DraftingGraphOps.cpp
    src/drafting/DraftingRegionFill.h
    src/drafting/DraftingRegionFill.cpp
    src/drafting/DraftingMapQuery.h
    src/drafting/DraftingMapQuery.cpp
    src/drafting/DraftingOverlap.h
    src/drafting/DraftingOverlap.cpp
    src/drafting/DraftingSpan.h
    src/drafting/DraftingSpan.cpp
    src/drafting/DraftingBlockOps.h
    src/drafting/DraftingBlockOps.cpp
    src/drafting/DraftingMotifOps.h
    src/drafting/DraftingMotifOps.cpp
    src/drafting/DraftingIntersectionOps.h
    src/drafting/DraftingIntersectionOps.cpp
    src/drafting/DraftingCorridor.h
    src/drafting/DraftingCorridor.cpp
    src/drafting/DraftingPathfind.h
    src/drafting/DraftingPathfind.cpp
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
    src/drafting/DraftingGcodeOut.h
    src/drafting/DraftingGcodeOut.cpp
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
    src/text/TextEncoding.h
    src/text/TextEncoding.cpp
    src/text/TextSearch.h
    src/text/TextSearch.cpp
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

# Exhaustiveness guard: a switch over DraftingShapeKind (or any enum) that omits a
# case is a build error here, not a silent fall-through. Pairs with the
# always_false_v static_asserts on the std::visit sites. See
# docs/adding-a-drafting-tool.md.
target_compile_options(edi_drafting_core PRIVATE -Werror=switch)

# Pillar A — the asset zoo: a curated, cross-document library of reusable assets.
# It persists with the shared MessagePack value codec + EDIM envelope, so it links
# the format core. It deliberately does NOT depend on the drafting core: the zoo is
# a catalog that outlives any one drawing, and an AssetRecord references geometry
# only by opaque string id — keeping the backbone independent of the document.
add_library(edi_zoo_core STATIC
    src/zoo/AssetZoo.h
    src/zoo/AssetZooOps.h
    src/zoo/AssetZooOps.cpp
    src/zoo/AssetZooSerialize.h
    src/zoo/AssetZooSerialize.cpp
)

target_include_directories(edi_zoo_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(edi_zoo_core PUBLIC edi_format_core)

add_library(edi_asset_link STATIC
    src/assetlink/AssetLinkResolve.h
    src/assetlink/AssetLinkResolve.cpp
)
target_include_directories(edi_asset_link PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# The asset-link bridge is the single, named meeting-point of two deliberately
# isolated cores: it resolves a drawing's neutral assetRefs against the zoo
# catalog. It links BOTH; neither core links the other.
target_link_libraries(edi_asset_link PUBLIC edi_drafting_core edi_zoo_core)

add_library(edi_recipe_core STATIC
    src/recipe/RecipeMeasure.h
    src/recipe/RecipeMeasure.cpp
    src/recipe/RecipeMouldings.h
    src/recipe/RecipeMouldings.cpp
    src/recipe/RecipeTextNumbers.h
    src/recipe/RecipeOps.h
    src/recipe/RecipeOps.cpp
    src/recipe/RecipeOpsBind.h
    src/recipe/RecipeOpsBind.cpp
    src/recipe/RecipeOpSchema.h
    src/recipe/RecipeOpSchema.cpp
    src/recipe/RecipeOpsResolve.h
    src/recipe/RecipeOpsResolve.cpp
    src/recipe/RecipeOpsStore.h
    src/recipe/RecipeOpsStore.cpp
    src/recipe/RecipeOpsValidate.h
    src/recipe/RecipeOpsValidate.cpp
    src/recipe/RecipeOpsAscii.h
    src/recipe/RecipeOpsAscii.cpp
    src/recipe/RecipeCraftsmen.h
    src/recipe/RecipeCraftsmen.cpp
)

target_include_directories(edi_recipe_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Recipes resolve their measurement bindings against the drafting document.
target_link_libraries(edi_recipe_core PUBLIC edi_drafting_core)

add_library(edi_scripting_core STATIC
    src/scripting/LuaRecipe.h
    src/scripting/LuaRecipe.cpp
    src/scripting/ScriptCommandBridge.h
    src/scripting/ScriptCommandBridge.cpp
    src/scripting/BlenderRunPlan.h
    src/scripting/BlenderRunPlan.cpp
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
add_edi_contract_test(drafting_transform_geometry_tests tests/drafting_transform_geometry_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_shape_kind_exhaustive_tests tests/drafting_shape_kind_exhaustive_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_arc_tests tests/drafting_arc_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_ellipse_tests tests/drafting_ellipse_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_text_tests tests/drafting_text_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_spline_tests tests/drafting_spline_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_wall_tests tests/drafting_wall_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_room_tests tests/drafting_room_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_ascii_map_tests tests/drafting_ascii_map_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_graph_ops_tests tests/drafting_graph_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_document_bounds_tests tests/drafting_document_bounds_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_map_query_tests tests/drafting_map_query_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_overlap_tests tests/drafting_overlap_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_span_tests tests/drafting_span_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_region_fill_tests tests/drafting_region_fill_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_block_ops_tests tests/drafting_block_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_motif_ops_tests tests/drafting_motif_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_intersection_points_tests tests/drafting_intersection_points_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_corridor_tests tests/drafting_corridor_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_pathfind_tests tests/drafting_pathfind_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_modify_tests tests/drafting_modify_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_hit_test_tests tests/drafting_hit_test_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_snap_tests tests/drafting_snap_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_point_along_tests tests/drafting_point_along_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_derived_tests tests/drafting_derived_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_grid_tests tests/drafting_grid_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_guide_ops_tests tests/drafting_guide_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_layer_ops_tests tests/drafting_layer_ops_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_selection_tests tests/drafting_selection_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_clipboard_tests tests/drafting_clipboard_tests.cpp edi_drafting_core)
add_edi_contract_test(drafting_inspector_plan_tests tests/drafting_inspector_plan_tests.cpp edi_drafting_core)
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
add_edi_contract_test(drafting_gcode_out_tests tests/drafting_gcode_out_tests.cpp edi_drafting_core)
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
add_edi_contract_test(text_encoding_tests tests/text_encoding_tests.cpp edi_text_core)
add_edi_contract_test(text_search_tests tests/text_search_tests.cpp edi_text_core)
add_edi_contract_test(text_editor_commands_tests tests/text_editor_commands_tests.cpp edi_text_core)
add_edi_contract_test(format_toml_tests tests/format_toml_tests.cpp edi_format_core)
add_edi_contract_test(format_messagepack_tests tests/format_messagepack_tests.cpp edi_format_core)
add_edi_contract_test(format_messagepack_value_tests tests/format_messagepack_value_tests.cpp edi_format_core)
add_edi_contract_test(asset_zoo_tests tests/asset_zoo_tests.cpp edi_zoo_core)
add_edi_contract_test(asset_link_resolve_tests tests/asset_link_resolve_tests.cpp edi_asset_link)
add_edi_contract_test(belt_state_tests tests/belt_state_tests.cpp)
target_sources(belt_state_tests PRIVATE src/widgets/BeltState.cpp)
target_include_directories(belt_state_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_executable(belt_cross_widget_tests
    tests/belt_cross_widget_tests.cpp
    src/widgets/BeltCrossWidget.h
    src/widgets/BeltCrossWidget.cpp
    src/widgets/BeltState.cpp
)
target_include_directories(belt_cross_widget_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(belt_cross_widget_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
add_test(NAME belt_cross_widget_tests COMMAND belt_cross_widget_tests)
add_edi_contract_test(drawing_canvas_ruler_ticks_tests tests/drawing_canvas_ruler_ticks_tests.cpp Qt6::Core Qt6::Gui)
target_sources(drawing_canvas_ruler_ticks_tests PRIVATE
    src/widgets/DrawingCanvasRulerTicks.cpp
    src/widgets/DrawingCanvasViewport.cpp
    src/widgets/DrawingCanvasProjectedGrid.cpp
    src/widgets/DrawingCanvasValues.cpp)
target_include_directories(drawing_canvas_ruler_ticks_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
add_edi_contract_test(asset_zoo_store_tests tests/asset_zoo_store_tests.cpp edi_zoo_core Qt6::Core)
target_sources(asset_zoo_store_tests PRIVATE src/io/AssetZooStore.cpp src/io/SettingsStore.cpp)
target_include_directories(asset_zoo_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(room_spec_store_tests tests/room_spec_store_tests.cpp edi_drafting_core edi_format_core)
target_sources(room_spec_store_tests PRIVATE src/io/RoomSpecStore.cpp)
target_include_directories(room_spec_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(map_spec_store_tests tests/map_spec_store_tests.cpp edi_drafting_core edi_format_core)
target_sources(map_spec_store_tests PRIVATE src/io/RoomSpecStore.cpp)
target_include_directories(map_spec_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(map_toon_export_tests tests/map_toon_export_tests.cpp edi_drafting_core)
target_sources(map_toon_export_tests PRIVATE src/io/MapToonExport.cpp)
target_include_directories(map_toon_export_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# G1 crypt generator — full end-to-end chain test (buildCryptMapSpec ->
# createMapFromSpec -> exportMapToToon).  Needs the controller sources
# (same set as drawing_document_controller_tests) plus MapToonExport and
# the new CryptGenerator module.  Uses QCoreApplication for the controller.
add_executable(crypt_generator_tests
    tests/crypt_generator_tests.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/MapToonExport.h
    src/io/MapToonExport.cpp
    src/io/CryptGenerator.h
    src/io/CryptGenerator.cpp
)
target_include_directories(crypt_generator_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(crypt_generator_tests PRIVATE
    edi_drafting_core
    Qt6::Core
)
add_test(NAME crypt_generator_tests COMMAND crypt_generator_tests)

# Phase-1 CANARY: pins the reference-dungeon TOON (sha256 6c632293…, 1685 bytes)
# and the DraftingObject count produced by createMapFromSpec byte-identically.
# This test MUST remain green through every additive Phase-1 slice; a drift means
# a new field's default is not neutral → halt and report (do NOT re-bless).
# Same source set as crypt_generator_tests + RoomSpecStore (for parseMapSpecToml).
# EDI_TESTS_DATA_DIR points to tests/data/ so the test locates dungeon.map.toml.
add_executable(map_regression_lock_tests
    tests/map_regression_lock_tests.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/MapToonExport.h
    src/io/MapToonExport.cpp
    src/io/RoomSpecStore.h
    src/io/RoomSpecStore.cpp
)
target_include_directories(map_regression_lock_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(map_regression_lock_tests PRIVATE
    EDI_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
)
target_link_libraries(map_regression_lock_tests PRIVATE
    edi_drafting_core
    edi_format_core
    Qt6::Core
)
add_test(NAME map_regression_lock_tests COMMAND map_regression_lock_tests)

# Phase-1 DETERMINISM GATE (brief 069): "same input → same dungeon".
# Runs createMapFromSpec TWICE with the reference dungeon spec, exports both
# documents via exportMapToToon (Seam C / document overload), then asserts
# the two TOON strings are byte-identical and the two DraftingObject counts
# are equal.  Owns the ordering-determinism invariant that the single-run
# canary cannot catch.  Uses the same source set as map_regression_lock_tests.
add_executable(map_determinism_tests
    tests/map_determinism_tests.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/MapToonExport.h
    src/io/MapToonExport.cpp
    src/io/RoomSpecStore.h
    src/io/RoomSpecStore.cpp
)
target_include_directories(map_determinism_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(map_determinism_tests PRIVATE
    EDI_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
)
target_link_libraries(map_determinism_tests PRIVATE
    edi_drafting_core
    edi_format_core
    Qt6::Core
)
add_test(NAME map_determinism_tests COMMAND map_determinism_tests)

# Controller behavioral test for syncGraphForMovedObject (brief 052):
# verifies that applyCommandAndEmit calls the anchor-resync after a geometry-
# mutation command, closing the pre-052 drift bug where plug.anchor went stale
# on interactive moves of a plug-anchored marker object.
add_executable(drafting_graph_sync_tests
    tests/drafting_graph_sync_tests.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/MapToonExport.h
    src/io/MapToonExport.cpp
    src/io/CryptGenerator.h
    src/io/CryptGenerator.cpp
)
target_include_directories(drafting_graph_sync_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(drafting_graph_sync_tests PRIVATE
    edi_drafting_core
    Qt6::Core
)
add_test(NAME drafting_graph_sync_tests COMMAND drafting_graph_sync_tests)

add_edi_contract_test(shell_layout_store_tests tests/shell_layout_store_tests.cpp edi_format_core Qt6::Core)
target_sources(shell_layout_store_tests PRIVATE src/io/ShellLayoutStore.cpp src/io/SettingsStore.cpp src/widgets/ShellHost.cpp src/widgets/ShellPanels.cpp)
target_include_directories(shell_layout_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(text_session_store_tests tests/text_session_store_tests.cpp edi_text_core edi_format_core Qt6::Core)
target_sources(text_session_store_tests PRIVATE src/io/TextSessionStore.cpp src/io/SettingsStore.cpp)
target_include_directories(text_session_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_edi_contract_test(scripting_bridge_tests tests/scripting_bridge_tests.cpp edi_scripting_core)
add_edi_contract_test(blender_run_plan_tests tests/blender_run_plan_tests.cpp edi_scripting_core)
add_edi_contract_test(process_run_store_tests tests/process_run_store_tests.cpp Qt6::Core)
target_include_directories(process_run_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(process_run_store_tests PRIVATE src/io/ProcessRunStore.h src/io/ProcessRunStore.cpp)
add_edi_contract_test(recipe_mouldings_tests tests/recipe_mouldings_tests.cpp edi_recipe_core)
add_edi_contract_test(recipe_ops_tests tests/recipe_ops_tests.cpp edi_recipe_core)
target_compile_definitions(recipe_ops_tests PRIVATE EDI_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/samples")
add_edi_contract_test(recipe_ops_ascii_tests tests/recipe_ops_ascii_tests.cpp edi_recipe_core)
target_compile_definitions(recipe_ops_ascii_tests PRIVATE EDI_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/samples")
add_edi_contract_test(recipe_ops_resolve_tests tests/recipe_ops_resolve_tests.cpp edi_recipe_core)
add_edi_contract_test(recipe_craftsmen_tests tests/recipe_craftsmen_tests.cpp edi_recipe_core)
add_edi_contract_test(recipe_drafted_column_tests tests/recipe_drafted_column_tests.cpp edi_recipe_core)
target_compile_definitions(recipe_drafted_column_tests PRIVATE EDI_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/samples")

# The craftsmen library's Blender-free half (strict TOML reader + dry-run
# plan) smokes under plain python when one is available; the bpy half runs
# only inside Blender.
# The version goes INTO find_package: FindPython3 then rejects too-old
# interpreters and keeps searching, instead of stopping at the first hit
# and silently dropping the test when that one is < 3.11.
find_package(Python3 3.11 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(NAME edi_craft_dry_run_smoke
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/edi_craft_smoke.py)
    # The M0 realizer's pure tier (TOON parse + greybox plan + OBJ proof). The
    # bpy/OptiX render half needs a GPU and is exercised in Blender, not ctest.
    add_test(NAME edi_realize_smoke
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/edi_realize_smoke.py)
else()
    message(STATUS "edi_craft_dry_run_smoke skipped: python >= 3.11 not found")
endif()

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

# The op pipeline's CLI (R1-B05): widget-free, links the recipe core only.
add_executable(edi_recipe_ops
    src/tools/EdiRecipeOps.cpp
)
target_include_directories(edi_recipe_ops PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(edi_recipe_ops PRIVATE
    edi_recipe_core
)
add_test(
    NAME edi_recipe_ops_cli_smoke
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/recipe_ops_cli_smoke.sh $<TARGET_FILE:edi_recipe_ops> ${CMAKE_CURRENT_SOURCE_DIR}/samples
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
    src/io/ProfileStore.h
    src/io/ProfileStore.cpp
    src/core/DrawingCore.h
    src/core/DrawingDocumentController.cpp
    src/core/DrawingDocumentProjection.h
    src/core/DrawingDocumentProjection.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/ProcessRunStore.h
    src/io/ProcessRunStore.cpp
    src/io/SettingsStore.h
    src/io/SettingsStore.cpp
    src/io/TextSessionStore.h
    src/io/TextSessionStore.cpp
    src/widgets/EdiShellWindow.h
    src/widgets/EdiShellWindow.cpp
    src/widgets/EdiShellWindowPanels.cpp
    src/widgets/EdiShellWindowIo.cpp
    src/widgets/TextEditorFeature.h
    src/widgets/TextEditorFeature.cpp
    src/widgets/DraftingFeature.h
    src/widgets/DraftingFeaturePanels.cpp
    src/widgets/DraftingFeatureInspector.cpp
    src/widgets/ShellWidgetHelpers.h
    src/widgets/ShellWidgetHelpers.cpp
    src/widgets/ShellTheme.h
    src/widgets/ShellTheme.cpp
    src/widgets/ShellHost.h
    src/widgets/ShellHost.cpp
    src/widgets/SettingsFeature.h
    src/widgets/SettingsFeature.cpp
    src/widgets/ShellPanels.h
    src/widgets/ShellPanels.cpp
    src/widgets/BeltState.h
    src/widgets/BeltState.cpp
    src/widgets/BeltCrossWidget.h
    src/widgets/BeltCrossWidget.cpp
    src/widgets/FloatingPalette.h
    src/widgets/FloatingPalette.cpp
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
    src/widgets/DrawingCanvasRulerTicks.h
    src/widgets/DrawingCanvasRulerTicks.cpp
    src/widgets/DrawingCanvasProjectedStatus.h
    src/widgets/DrawingCanvasProjectedStatus.cpp
    src/widgets/DrawingCanvasProjectedPointer.h
    src/widgets/DrawingCanvasProjectedPointer.cpp
    src/widgets/DrawingCanvasChromeDims.h
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
    edi_recipe_core
    edi_scripting_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
target_compile_definitions(edi_shell_window_tests PRIVATE EDI_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/samples")
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
    src/widgets/DrawingCanvasRulerTicks.h
    src/widgets/DrawingCanvasRulerTicks.cpp
    src/widgets/DrawingCanvasProjectedStatus.h
    src/widgets/DrawingCanvasProjectedStatus.cpp
    src/widgets/DrawingCanvasProjectedPointer.h
    src/widgets/DrawingCanvasProjectedPointer.cpp
    src/widgets/DrawingCanvasChromeDims.h
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
    src/io/ProcessRunStore.h
    src/io/ProcessRunStore.cpp
    src/io/RoomSpecStore.h
    src/io/RoomSpecStore.cpp
    src/io/MapToonExport.h
    src/io/MapToonExport.cpp
    src/io/CryptGenerator.h
    src/io/CryptGenerator.cpp
    src/io/SettingsStore.h
    src/io/SettingsStore.cpp
    src/io/DrawingRecentFilesStore.h
    src/io/DrawingRecentFilesStore.cpp
    src/io/ShellLayoutStore.h
    src/io/ShellLayoutStore.cpp
    src/io/ProfileStore.h
    src/io/ProfileStore.cpp
    src/io/TextSessionStore.h
    src/io/TextSessionStore.cpp
    src/widgets/EdiShellWindow.h
    src/widgets/EdiShellWindow.cpp
    src/widgets/EdiShellWindowPanels.cpp
    src/widgets/EdiShellWindowIo.cpp
    src/widgets/TextEditorFeature.h
    src/widgets/TextEditorFeature.cpp
    src/widgets/DraftingFeature.h
    src/widgets/DraftingFeaturePanels.cpp
    src/widgets/DraftingFeatureInspector.cpp
    src/widgets/ShellWidgetHelpers.h
    src/widgets/ShellWidgetHelpers.cpp
    src/widgets/ShellTheme.h
    src/widgets/ShellTheme.cpp
    src/widgets/ShellHost.h
    src/widgets/ShellHost.cpp
    src/widgets/SettingsFeature.h
    src/widgets/SettingsFeature.cpp
    src/widgets/ShellPanels.h
    src/widgets/ShellPanels.cpp
    src/widgets/BeltState.h
    src/widgets/BeltState.cpp
    src/widgets/BeltCrossWidget.h
    src/widgets/BeltCrossWidget.cpp
    src/widgets/FloatingPalette.h
    src/widgets/FloatingPalette.cpp
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
    src/widgets/DrawingCanvasRulerTicks.h
    src/widgets/DrawingCanvasRulerTicks.cpp
    src/widgets/DrawingCanvasProjectedStatus.h
    src/widgets/DrawingCanvasProjectedStatus.cpp
    src/widgets/DrawingCanvasProjectedPointer.h
    src/widgets/DrawingCanvasProjectedPointer.cpp
    src/widgets/DrawingCanvasChromeDims.h
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
        edi_recipe_core
        edi_scripting_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
