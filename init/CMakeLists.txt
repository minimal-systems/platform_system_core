cmake_minimum_required(VERSION 3.18)
project(init CXX)
set(BUILD_TOP "/run/media/kjones/build/android/minimal_android")

# Define source files for init
set(INIT_SOURCES
    init.cpp
    selinux.cpp
    init_parser.cpp
    first_stage_mount.cpp
    first_stage_init.cpp
    logprint.cpp
    logger_write.cpp
    property_manager.cpp
    fs_mgr.cpp
    verify.cpp
    util.cpp
    boot_clock.cpp
    libbase.cpp
)

# Compiler flags
add_definitions(
    -DTARGET_DEVICE="${TARGET_DEVICE}"
    -DALLOW_PERMISSIVE_SELINUX=1
    -DREBOOT_BOOTLOADER_ON_PANIC=0
)

add_compile_options(
    -g
    -Wno-format
    -Wno-misleading-indentation
    -Wno-return-type
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fno-math-errno
    -fno-unroll-loops
    -fmerge-all-constants
    -fno-ident
)

# Include directories
include_directories(
    ${BUILD_TOP}/system/logging/logd/logging/include
    ${BUILD_TOP}/system/core/init
)
# Add executable
add_executable(init ${INIT_SOURCES})

# Link against liblog explicitly

# Specify where to find liblog
link_directories(${BUILT_TOP}/system/core/build/logging/src/logging-build/logd)

# Linker flags
target_link_options(init PRIVATE
    -Wl,--gc-sections
    -Wl,--as-needed
    -Wl,--strip-debug
    -Wl,--hash-style=gnu
    -Wl,--exclude-libs,ALL
)

# Link additional system libraries
target_link_libraries(init PRIVATE ssl crypto)

# Set installation path for init
install(TARGETS init DESTINATION /run/media/kjones/build/android/minimal_android/out/target/product/generic/rootfs/usr/bin)
