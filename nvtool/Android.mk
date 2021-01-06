LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := odmsz

LOCAL_SRC_FILES :=nvtool.cpp nv.cpp 

LOCAL_MODULE := nvtool
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := external/libcxx/include \
                    $(QC_PROP_ROOT)/diag/include \
                    $(QC_PROP_ROOT)/diag/src/ \
                    $(TARGET_OUT_HEADERS)/common/inc

LOCAL_SHARED_LIBRARIES := libcutils libutils libdiag libc++ liblog

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr


include $(BUILD_EXECUTABLE)
