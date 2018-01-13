ifeq ($(OEM_BUILD_ideatool),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= ideatool.c talkto6270.c
LOCAL_MODULE:= ideatool



LOCAL_MODULE_TAG := optional
LOCAL_SHARED_LIBRARIES := libc libcutils libutils

include $(BUILD_EXECUTABLE)
endif
