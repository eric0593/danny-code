
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= macplugin.c 
LOCAL_MODULE:= macplugin

LOCAL_MODULE_TAG := optional
LOCAL_SHARED_LIBRARIES := libc libcutils libutils libbase liblog
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR_EXECUTABLES)
include $(BUILD_EXECUTABLE)

