LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
#
LOCAL_MODULE    := libalsa-jni
#### Add all source file names to be included in lib separated by a whitespace
LOCAL_SRC_FILES := src/com_iflytek_alsa_jni_AlsaJni.cpp src/queue/AudioQueue.c
#
LOCAL_LDLIBS := -llog -L$(LOCAL_PATH)/ -ltinyalsa
#
include $(BUILD_SHARED_LIBRARY)

#include $(CLEAR_VARS)
##
#LOCAL_MODULE    := iflytekcae
##### Add all source file names to be included in lib separated by a whitespace
#LOCAL_SRC_FILES := src/exec/iflytekcae.cpp src/queue/AudioQueue.c
##
#LOCAL_CFLAGS += -fno-stack-protector
##
#LOCAL_LDLIBS := -llog -L$(LOCAL_PATH)/ -ltinyalsa -L$(LOCAL_PATH)/ -lcae
##
#include $(BUILD_EXECUTABLE)
