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
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

if(BUILD_TESTING)
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
    if(NODE_EXECUTABLE)
        add_test(
            NAME drawing_canvas_viewport_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_viewport_tests.js
        )
        add_test(
            NAME drawing_canvas_hit_test_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_hit_test_tests.js
        )
        add_test(
            NAME drawing_canvas_snap_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_snap_tests.js
        )
        add_test(
            NAME drawing_canvas_handles_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_handles_tests.js
        )
        add_test(
            NAME drawing_canvas_gesture_state_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_gesture_state_tests.js
        )
        add_test(
            NAME drawing_canvas_projection_tests
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_canvas_projection_tests.js
        )
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
