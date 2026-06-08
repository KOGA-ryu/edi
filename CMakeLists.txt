cmake_minimum_required(VERSION 3.22)

project(EDI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
include(CTest)

set(DRAFTSMAN_SHELL_TARGET_NAME "qt_qml_region_split" CACHE STRING "Draftsman shell executable target name.")
set(DRAFTSMAN_SHELL_DATA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Project data root used for default profiles, themes, layouts, and review subjects.")
option(DRAFTSMAN_ENABLE_DRAWING_JS_HARNESS_TESTS "Register optional JS drawing telemetry, tooling, and workflow harness tests." OFF)
option(DRAFTSMAN_ENABLE_LEGACY_JS_CANVAS_TESTS "Compatibility alias for DRAFTSMAN_ENABLE_DRAWING_JS_HARNESS_TESTS." OFF)
if(DRAFTSMAN_ENABLE_LEGACY_JS_CANVAS_TESTS)
    set(DRAFTSMAN_ENABLE_DRAWING_JS_HARNESS_TESTS ON CACHE BOOL "Register optional JS drawing telemetry, tooling, and workflow harness tests." FORCE)
endif()

add_library(drawing_canvas_core STATIC
    src/canvas/DrawingCanvasTypes.h
    src/canvas/DrawingCanvasTypes.cpp
    src/canvas/DrawingCanvasViewport.h
    src/canvas/DrawingCanvasViewport.cpp
    src/canvas/DrawingCanvasHandles.h
    src/canvas/DrawingCanvasHandles.cpp
    src/canvas/DrawingCanvasHitTest.h
    src/canvas/DrawingCanvasHitTest.cpp
    src/canvas/DrawingCanvasSnap.h
    src/canvas/DrawingCanvasSnap.cpp
    src/canvas/DrawingCanvasProjection.h
    src/canvas/DrawingCanvasProjection.cpp
    src/canvas/DrawingCanvasGestureState.h
    src/canvas/DrawingCanvasGestureState.cpp
    src/canvas/DrawingCanvasRuntimeAdapter.h
    src/canvas/DrawingCanvasRuntimeAdapter.cpp
)

target_include_directories(drawing_canvas_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(drawing_canvas_core
    PUBLIC
        Qt6::Core
)

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

add_executable(${DRAFTSMAN_SHELL_TARGET_NAME}
    app/main.cpp
    src/core/DrawingCore.h
    src/core/DrawingCoreInternal.h
    src/core/DrawingObjectModel.h
    src/core/DrawingObjectModel.cpp
    src/core/DrawingCore.cpp
    src/core/DrawingCoreCommon.cpp
    src/core/DrawingDocumentController.cpp
    src/core/DrawingGeometry.cpp
    src/core/DrawingObjectOps.cpp
    src/core/DrawingCommands.cpp
    src/core/DrawingModelBuilder.cpp
    src/core/DrawingSvgExport.cpp
    src/export/BlenderSvgBundleTemplates.h
    src/export/BlenderSvgBundleTemplates.cpp
    src/export/BlenderSvgBundleExport.h
    src/export/BlenderSvgBundleExport.cpp
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/DrawingRecentFilesStore.h
    src/io/DrawingRecentFilesStore.cpp
    src/io/ShellLayoutStore.h
    src/io/ShellLayoutStore.cpp
    src/io/TextEditorStore.h
    src/io/TextEditorStore.cpp
)

target_include_directories(${DRAFTSMAN_SHELL_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${DRAFTSMAN_SHELL_TARGET_NAME} PRIVATE
    PROJECT_SOURCE_DIR="${DRAFTSMAN_SHELL_DATA_ROOT_DIR}"
)

target_link_libraries(${DRAFTSMAN_SHELL_TARGET_NAME}
    PRIVATE
        drawing_canvas_core
        drawing_runtime_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

add_executable(edi_validate
    src/tools/EdiValidate.cpp
)

target_link_libraries(edi_validate
    PRIVATE
        Qt6::Core
)

add_executable(drawing_control_workflow_report
    src/tools/DrawingControlWorkflowReport.cpp
)

target_compile_definitions(drawing_control_workflow_report PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(drawing_control_workflow_report
    PRIVATE
        Qt6::Core
)

if(BUILD_TESTING)
    add_executable(drawing_canvas_contract_tests
        tests/drawing_canvas_contract_tests.cpp
    )

    target_include_directories(drawing_canvas_contract_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(drawing_canvas_contract_tests
        PRIVATE
            drawing_canvas_core
            Qt6::Core
    )

    add_test(NAME drawing_canvas_contract_tests COMMAND drawing_canvas_contract_tests)

    add_executable(drawing_canvas_adapter_tests
        tests/drawing_canvas_adapter_tests.cpp
    )

    target_include_directories(drawing_canvas_adapter_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(drawing_canvas_adapter_tests
        PRIVATE
            drawing_canvas_core
            Qt6::Core
    )

    add_test(NAME drawing_canvas_adapter_tests COMMAND drawing_canvas_adapter_tests)

    add_executable(drawing_runtime_core_tests
        tests/drawing_runtime_core_tests.cpp
    )

    target_include_directories(drawing_runtime_core_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(drawing_runtime_core_tests
        PRIVATE
            drawing_runtime_core
            Qt6::Core
    )

    add_test(NAME drawing_runtime_core_tests COMMAND drawing_runtime_core_tests)

    add_executable(drawing_tool_script_runtime_tests
        tests/drawing_tool_script_runtime_tests.cpp
    )

    target_include_directories(drawing_tool_script_runtime_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_definitions(drawing_tool_script_runtime_tests PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(drawing_tool_script_runtime_tests
        PRIVATE
            drawing_runtime_core
            Qt6::Core
    )

    add_test(NAME drawing_tool_script_runtime_tests COMMAND drawing_tool_script_runtime_tests)

    add_executable(no_javascript_files_tests
        tests/no_javascript_files_tests.cpp
    )

    target_compile_definitions(no_javascript_files_tests PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(NAME no_javascript_files_tests COMMAND no_javascript_files_tests)
    set_tests_properties(no_javascript_files_tests PROPERTIES DISABLED TRUE)

    add_test(
        NAME drawing_control_workflow_report_dry_run_tests
        COMMAND drawing_control_workflow_report --all --dry-run --compact
    )

    add_test(
        NAME drawing_control_workflow_report_disabled_baseline_tests
        COMMAND drawing_control_workflow_report --all --compare-baseline --failures-only
    )

    add_executable(drawing_object_model_tests
        tests/drawing_object_model_tests.cpp
        src/core/DrawingObjectModel.h
        src/core/DrawingObjectModel.cpp
    )

    target_include_directories(drawing_object_model_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(drawing_object_model_tests
        PRIVATE
            Qt6::Core
    )

    add_test(NAME drawing_object_model_tests COMMAND drawing_object_model_tests)

    add_executable(drawing_core_smoke
        tests/drawing_core_smoke.cpp
        src/core/DrawingCore.h
        src/core/DrawingCoreInternal.h
        src/core/DrawingObjectModel.h
        src/core/DrawingObjectModel.cpp
        src/core/DrawingCore.cpp
        src/core/DrawingCoreCommon.cpp
        src/core/DrawingDocumentController.cpp
        src/core/DrawingGeometry.cpp
        src/core/DrawingObjectOps.cpp
        src/core/DrawingCommands.cpp
        src/core/DrawingModelBuilder.cpp
        src/core/DrawingSvgExport.cpp
    )

    target_include_directories(drawing_core_smoke PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(drawing_core_smoke
        PRIVATE
            Qt6::Core
    )

    add_test(NAME drawing_core_smoke COMMAND drawing_core_smoke)
endif()
