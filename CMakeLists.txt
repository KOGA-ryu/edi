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
    src/core/DrawingCore.cpp
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
    add_executable(drawing_core_smoke
        tests/drawing_core_smoke.cpp
        src/core/DrawingCore.h
        src/core/DrawingCore.cpp
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
