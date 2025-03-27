cmake_minimum_required(VERSION 3.5)
project(SonarViewer)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(SonarViewer main.cpp)
target_link_libraries(SonarViewer Qt5::Widgets)

