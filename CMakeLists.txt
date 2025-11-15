cmake_minimum_required(VERSION 3.16)
project(smart_irrigation_system VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(smart_irrigation_system
    src/main.cpp
)

target_link_libraries(smart_irrigation_system PRIVATE Qt6::Widgets)
