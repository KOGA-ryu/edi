cmake_minimum_required(VERSION 3.22)

project(EDI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
include(CTest)

set(DRAFTSMAN_SHELL_TARGET_NAME "edi")
set(DRAFTSMAN_SHELL_DATA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Project data root used for default profiles, themes, layouts, and review subjects.")

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
    src/io/DrawingDocumentStore.h
    src/io/DrawingDocumentStore.cpp
    src/io/DrawingRecentFilesStore.h
    src/io/DrawingRecentFilesStore.cpp
    src/io/ShellLayoutStore.h
    src/io/ShellLayoutStore.cpp
    src/io/TextEditorStore.h
    src/io/TextEditorStore.cpp
    src/widgets/DrawingCanvasWidget.h
    src/widgets/DrawingCanvasWidget.cpp
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
