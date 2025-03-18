# CMakeLists.txt for libmodprobe

cmake_minimum_required(VERSION 3.18)

# Project name
project(libmodprobe_project)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure position-independent code (useful for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Rename the target to avoid conflicts
add_library(libmodprobe_static STATIC
    libmodprobe.cpp
    libmodprobe_ext.cpp
)

# Enable warnings as errors
target_compile_options(libmodprobe_static PRIVATE -Werror)

# Include directories
target_include_directories(libmodprobe_static PUBLIC include/)

# Link against the required libraries
target_link_libraries(libmodprobe_static PRIVATE base)

# Optional: Print build information
message(STATUS "Building libmodprobe as a static library")