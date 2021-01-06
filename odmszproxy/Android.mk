# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(call is-vendor-board-platform,QCOM),true)
  LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
  #LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
endif
LOCAL_SRC_FILES:= \
	odmszproxy.c


LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libdl

LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= odmszproxy
LOCAL_MODULE_TAGS := optional

LOCAL_INIT_RC := odmszproxy.rc

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

ifeq ($(call is-vendor-board-platform,QCOM),true)
  LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
  
  #LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
endif
LOCAL_SRC_FILES:= \
	odmsz.c
LOCAL_VENDOR_MODULE := true

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libdl


LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= odmsz
LOCAL_MODULE_STEM := psh
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)




