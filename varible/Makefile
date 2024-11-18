LOCAL_MODULE= var
LOCAL_MODULE_PATH=$(BUILD_TOP)/system/core/varible
SRC_FILES=$(LOCAL_MODULE)
SRC_RECOVERY_FILES=$(LOCAL_MODULE)_recovery

#silence shit dude
varibles:
	@cp -r $(LOCAL_MODULE_PATH)/$(LOCAL_MODULE) $(TARGET_MAIN_RAMDISK)/usr/share/etc/bashrc
	@mkdir -p $(TARGET_RECOVERY_OUT)/etc
	@cp -r $(LOCAL_MODULE_PATH)/$(SRC_RECOVERY_FILES) $(TARGET_RECOVERY_OUT)/etc/.bashrc
.PHONY: all
