
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := odmsz
LOCAL_SRC_FILES:= macplugin.c

LOCAL_MODULE:= macplugin

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_SHARED_LIBRARIES :=  libcutils libutils libbase liblog
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR_EXECUTABLES)
include $(BUILD_EXECUTABLE)

