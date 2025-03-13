cmake_minimum_required(VERSION 3.18)
project(AndroidCore CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Enable position-independent code (for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Ensure logging is built first, even if it is outside system/core
include(ExternalProject)

ExternalProject_Add(logging
    PREFIX ${CMAKE_BINARY_DIR}/logging
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../logging  # Adjust path based on where logging actually exists
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
    INSTALL_COMMAND ""
)


# Include the init module
add_subdirectory(init)

# Ensure init links against the built logging library
add_dependencies(init logging)
