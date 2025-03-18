cmake_minimum_required(VERSION 3.18)
project(AndroidCore CXX)

option(USE_CLANG "Build application with clang" ON) # Default ON

# Set Android build top directory
set(BUILD_TOP "/run/media/kjones/build/android/minimal_android")

# Define rootfs install location
set(ROOTFS_INSTALL_DIR "${BUILD_TOP}/out/target/product/generic/rootfs")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable position-independent code (for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Ensure logging is built first, even if it is outside system/core
include(ExternalProject)

# Logging external project
ExternalProject_Add(logging
    PREFIX ${CMAKE_BINARY_DIR}/logging
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../logging  # Adjust path based on actual location
    CMAKE_ARGS 
        -DCMAKE_INSTALL_PREFIX=/usr
        -DCMAKE_C_COMPILER=clang
        -DCMAKE_CXX_COMPILER=clang++
        -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold
        -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=mold
    INSTALL_COMMAND make install DESTDIR=${ROOTFS_INSTALL_DIR}
)

# Add libcap as an external project (forcing installation to rootfs)
ExternalProject_Add(libcap
    PREFIX ${CMAKE_BINARY_DIR}/libcap
    SOURCE_DIR ${BUILD_TOP}/external/libcap
    CMAKE_ARGS 
        -DCMAKE_INSTALL_PREFIX=/usr  # Keep /usr so DESTDIR redirects properly
        -DCMAKE_C_COMPILER=clang
        -DCMAKE_CXX_COMPILER=clang++
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_SHARED_LIBS=ON
    BUILD_COMMAND make
    INSTALL_COMMAND make install DESTDIR=${ROOTFS_INSTALL_DIR}  # Redirect install location
)

# Retrieve libcap install directory properties
ExternalProject_Get_Property(libcap INSTALL_DIR)

# Include the init module
add_subdirectory(init)

# Ensure init links against the built logging and libcap libraries
add_dependencies(init logging libcap)

# Install init binary to rootfs/usr/bin
install(TARGETS init RUNTIME DESTINATION ${ROOTFS_INSTALL_DIR}/usr/bin)

# Install libcap libraries from its correct install location
install(FILES 
    ${ROOTFS_INSTALL_DIR}/usr/lib/libcap.so
    ${ROOTFS_INSTALL_DIR}/usr/lib/libcap.a
    DESTINATION ${ROOTFS_INSTALL_DIR}/usr/lib
)

# Install libcap headers
install(DIRECTORY ${ROOTFS_INSTALL_DIR}/usr/include/ DESTINATION ${ROOTFS_INSTALL_DIR}/usr/include)

# Ensure logging libraries are installed to rootfs/usr/lib
install(DIRECTORY ${CMAKE_BINARY_DIR}/logging/lib/ DESTINATION ${ROOTFS_INSTALL_DIR}/usr/lib)
