LOCAL_PATH := $(call my-dir)
ifeq ($(yes),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  fpga_loader.cpp \
	hardware.cpp \
	i2c_main.cpp \
	i2c_core.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils \
    liblog

#LOCAL_CFLAGS += -O0 -g

# Define FM_AUDIO_PATH for configuring FM audio path.
# Valid values includes 0(ROUTE_NONE),1(ROUTE_DAC),2(ROUTE_I2S).
# If the flag is not defined here 1(ROUTE_DAC) will be taken as default.
#LOCAL_CFLAGS += -DFM_AUDIO_PATH=1

LOCAL_MODULE := libfpgaloader_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif



include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
  fpga_loader.cpp \
	hardware.cpp \
	i2c_main.cpp \
	i2c_core.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils \
    liblog

#LOCAL_CFLAGS += -O0 -g

# Define FM_AUDIO_PATH for configuring FM audio path.
# Valid values includes 0(ROUTE_NONE),1(ROUTE_DAC),2(ROUTE_I2S).
# If the flag is not defined here 1(ROUTE_DAC) will be taken as default.
#LOCAL_CFLAGS += -DFM_AUDIO_PATH=1
LOCAL_MODULE:= fpga_loader

LOCAL_MODULE_TAG := optional 


include $(BUILD_EXECUTABLE)