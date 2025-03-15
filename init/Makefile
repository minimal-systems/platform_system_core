LOCAL_PATH := system/core/init

include $(CLEAR_VARS)

LOCAL_MODULE := init

LOCAL_SRC_FILES := \
    init.cpp \
    selinux.cpp \
    init_parser.cpp \
    first_stage_mount.cpp \
    first_stage_init.cpp \
    logprint.cpp \
    logger_write.cpp \
    property_manager.cpp \
    fs_mgr.cpp \
    verify.cpp \
    util.cpp \
    boot_clock.cpp \
    libbase.cpp

LOCAL_CFLAGS := \
    -g \
    -Wno-format \
    -Wno-misleading-indentation \
    -Wno-return-type \
    -fno-unwind-tables \
    -fno-asynchronous-unwind-tables \
    -fno-math-errno \
    -fno-unroll-loops \
    -fmerge-all-constants \
    -fno-ident \
    -DTARGET_DEVICE=\"$(TARGET_DEVICE)\" \
    -DALLOW_PERMISSIVE_SELINUX=1 \
    -DREBOOT_BOOTLOADER_ON_PANIC=0

LOCAL_CLANG := true

LOCAL_C_INCLUDES := \
    logging/include

# Optimization flags for faster linking with mold linker
LOCAL_LDFLAGS := \
    -fuse-ld=mold \
    -Wl,--gc-sections \
    -Wl,--as-needed \
    -Wl,--strip-debug \
    -Wl,--hash-style=gnu \
    -Wl,--exclude-libs,ALL \
    -lssl -lcrypto

INCLUDE_RECOVERY_EXECUTABLE := true

include $(BUILD_EXECUTABLE)
