cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(ESPViewer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch fmt
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

# Use vcpkg to find packages
find_package(OpenCV REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Add your include directories
include_directories(${OpenCV_INCLUDE_DIRS} include)

# Add the source files
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/microcv2.cpp
    src/qt5.cpp
)

# Link the necessary libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} Qt5::Widgets fmt::fmt)
