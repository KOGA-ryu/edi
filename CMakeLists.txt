cmake_minimum_required(VERSION 3.22)

project(QtQmlRegionSplit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(qt_qml_region_split
    app/main.cpp
)

target_compile_definitions(qt_qml_region_split PRIVATE
    QML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src"
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(qt_qml_region_split
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)
