cmake_minimum_required(VERSION 3.16)
project(smart_irrigation_system VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/core
)


qt_add_executable(smart_irrigation_system
    src/main.cpp
    src/core/app_controller.cpp       
    include/app_controller.h
    src/gui/mainwindow.h
    src/gui/mainwindow.cpp          
)

target_link_libraries(smart_irrigation_system PRIVATE Qt6::Widgets)
