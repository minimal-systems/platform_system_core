LOCAL_PATH := $(BUILD_TOP)/system/core/rootdir
TARGET_DEVICE_PROPS := $(BUILD_TOP)/device/$(TARGET_DEVICE)/props
SYSTEM_PROPS := $(TARGET_DEVICE_PROPS)/system.prop
VENDOR_PROPS := $(TARGET_DEVICE_PROPS)/vendor.prop
DEFAULT_PROPS := $(TARGET_DEVICE_PROPS)/default.prop

define MAKE_DIRS
	@mkdir -p $1
endef

define COPY_FILE
	@cp -r $1 $2
endef

define LINK_FILE
	@ln -sf $1 $2
endef

TARGET_RAMDISK_FOLDERS := $(TARGET_ROOT_OUT)
$(TARGET_RAMDISK_FOLDERS):
	$(call MAKE_DIRS,$(TARGET_ROOT_FOLDERS))
	$(call MAKE_DIRS,$(TARGET_MAIN_RAMDISK)/etc/init/hw)
	$(call MAKE_DIRS,$(TARGET_MAIN_RAMDISK)/usr/etc/init/hw)
	$(call MAKE_DIRS,$(TARGET_MAIN_RAMDISK)/usr/share/etc/init/hw)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk/containers)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk/debug_ramdisk)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk/dev)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk/mnt)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk/proc)
	$(call MAKE_DIRS,$(TARGET_PRODUCT_OUT)/ramdisk/sys)

	$(call COPY_FILE,$(TARGET_MAIN_RAMDISK)/bin/init,$(TARGET_PRODUCT_OUT)/ramdisk)
	$(call COPY_FILE,$(BUILD_TOP)/device/$(TARGET_DEVICE)/fstab/fstab.$(TARGET_DEVICE),$(TARGET_PRODUCT_OUT)/ramdisk/fstab.$(TARGET_DEVICE))
	$(call COPY_FILE,$(LOCAL_PATH)/*.rc,$(TARGET_MAIN_RAMDISK)/etc/init/hw)

	$(TARGET_ADDITIONAL_SYMLINKS)

	$(call COPY_FILE,$(VENDOR_PROPS),$(TARGET_MAIN_RAMDISK)/usr/share/build.prop)
	$(call COPY_FILE,$(SYSTEM_PROPS),$(TARGET_MAIN_RAMDISK)/etc/build.prop)
	$(call COPY_FILE,$(BUILD_TOP)/device/$(TARGET_DEVICE)/rootdir/init.*.rc,$(TARGET_MAIN_RAMDISK)/usr/share/etc/init/hw)
	@echo $(BOLD)"Target ramdisk folder: $@"${CL_RST}

.PHONY: $(TARGET_RAMDISK_FOLDERS)
