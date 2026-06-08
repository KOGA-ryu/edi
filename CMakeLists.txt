cmake_minimum_required(VERSION 3.22)

project(QtQmlRegionSplit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
include(CTest)

set(DRAFTSMAN_SHELL_TARGET_NAME "qt_qml_region_split" CACHE STRING "Draftsman shell executable target name.")
set(DRAFTSMAN_SHELL_QML_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "Shared Draftsman QML shell source directory.")
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

qt_add_executable(${DRAFTSMAN_SHELL_TARGET_NAME}
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
    QML_SOURCE_DIR="${DRAFTSMAN_SHELL_QML_SOURCE_DIR}"
    PROJECT_SOURCE_DIR="${DRAFTSMAN_SHELL_DATA_ROOT_DIR}"
)

target_link_libraries(${DRAFTSMAN_SHELL_TARGET_NAME}
    PRIVATE
        drawing_canvas_core
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

add_executable(edi_validate
    src/tools/EdiValidate.cpp
)

target_link_libraries(edi_validate
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

    add_executable(no_javascript_files_tests
        tests/no_javascript_files_tests.cpp
    )

    target_compile_definitions(no_javascript_files_tests PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(NAME no_javascript_files_tests COMMAND no_javascript_files_tests)
    set_tests_properties(no_javascript_files_tests PROPERTIES DISABLED TRUE)

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

    find_program(NODE_EXECUTABLE node)
    if(DRAFTSMAN_ENABLE_DRAWING_JS_HARNESS_TESTS AND NODE_EXECUTABLE)
        add_test(
            NAME drawing_canvas_interaction_metrics_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_interaction_metrics_tests.js
        )
        add_test(
            NAME drawing_canvas_interaction_script_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_interaction_script_tests.js
        )
        add_test(
            NAME drawing_canvas_interaction_replay_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_interaction_replay_tests.js
        )
        add_test(
            NAME drawing_canvas_interaction_telemetry_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_interaction_telemetry_tests.js
        )
        add_test(
            NAME drawing_canvas_metric_reducer_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_metric_reducer_tests.js
        )
        add_test(
            NAME drawing_canvas_metric_report_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_metric_report_tests.js
        )
        add_test(
            NAME drawing_canvas_gui_metric_capture_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_gui_metric_capture_tests.js
        )
        add_test(
            NAME drawing_canvas_gui_metric_capture_cli_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_gui_metric_capture_cli_tests.js
        )
        add_test(
            NAME drawing_canvas_tool_script_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_tool_script_tests.js
        )
        add_test(
            NAME drawing_canvas_tool_script_driver_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_tool_script_driver_tests.js
        )
        add_test(
            NAME drawing_canvas_control_script_launch_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_control_script_launch_tests.js
        )
        add_test(
            NAME drawing_canvas_control_workflow_launch_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_control_workflow_launch_tests.js
        )
        add_test(
            NAME drawing_canvas_control_workflow_cli_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_control_workflow_cli_tests.js
        )
        add_test(
            NAME drawing_canvas_control_workflow_coverage_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_control_workflow_coverage_tests.js
        )
        add_test(
            NAME drawing_canvas_control_workflow_baseline_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_control_workflow_baseline_tests.js
        )
        add_test(
            NAME drawing_canvas_control_negative_launch_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_control_negative_launch_tests.js
        )
        set(DRAWING_JS_HARNESS_TEST_NAMES
            drawing_canvas_interaction_metrics_tests
            drawing_canvas_interaction_script_tests
            drawing_canvas_interaction_replay_tests
            drawing_canvas_interaction_telemetry_tests
            drawing_canvas_metric_reducer_tests
            drawing_canvas_metric_report_tests
            drawing_canvas_gui_metric_capture_tests
            drawing_canvas_gui_metric_capture_cli_tests
            drawing_canvas_tool_script_tests
            drawing_canvas_tool_script_driver_tests
            drawing_canvas_control_script_launch_tests
            drawing_canvas_control_workflow_launch_tests
            drawing_canvas_control_workflow_cli_tests
            drawing_canvas_control_workflow_coverage_tests
            drawing_canvas_control_workflow_baseline_tests
            drawing_canvas_control_negative_launch_tests
        )
        set_tests_properties(${DRAWING_JS_HARNESS_TEST_NAMES}
            PROPERTIES ENVIRONMENT "DRAFTSMAN_ENABLE_DRAWING_HARNESS=1"
        )
    endif()

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
