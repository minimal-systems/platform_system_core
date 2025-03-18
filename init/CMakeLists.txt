cmake_minimum_required(VERSION 3.18)
project(init CXX)

set(BUILD_TOP "/run/media/kjones/build/android/minimal_android")
set(ROOTFS_INSTALL_DIR "${BUILD_TOP}/out/target/product/generic/rootfs")

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
    modprobe.cpp
    exthandler.cpp
    libmodprobe_ext.cpp
    reboot_utils.cpp
    capabilities.cpp
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

# Locate installed liblog and libcap from AndroidCore
set(LIBLOG_DIR "${ROOTFS_INSTALL_DIR}/usr/lib")
set(LIBCAP_DIR "${ROOTFS_INSTALL_DIR}/usr/lib")
set(LIBLOG_INCLUDE_DIR "${ROOTFS_INSTALL_DIR}/usr/include")
set(LIBCAP_INCLUDE_DIR "${ROOTFS_INSTALL_DIR}/usr/include")

# Ensure liblog and libcap headers are included
include_directories(${LIBLOG_INCLUDE_DIR} ${LIBCAP_INCLUDE_DIR})

include_directories(
    ${BUILD_TOP}/system/logging/logd/logging/include
    ${BUILD_TOP}/system/logging/logd/logging/include/log
    ${BUILD_TOP}/system/logging/logd/logging/include/minimal_systems
    ${BUILD_TOP}/system/logging/logd/logging/include/private
    ${BUILD_TOP}/system/logging/logd
)

include_directories(${BUILD_TOP}/external/libcap/include)


# Add executable
add_executable(init ${INIT_SOURCES})

# Linker flags
target_link_options(init PRIVATE
    -Wl,--gc-sections
    -Wl,--as-needed
    -Wl,--strip-debug
    -Wl,--hash-style=gnu
    -Wl,--exclude-libs,ALL
)

# Link to prebuilt liblog and libcap
target_link_libraries(init PRIVATE
    ${LIBLOG_DIR}/liblog.so
    ${LIBCAP_DIR}/libcap.so
    ssl crypto
)

# Install init binary
install(TARGETS init RUNTIME DESTINATION ${ROOTFS_INSTALL_DIR}/usr/bin)
