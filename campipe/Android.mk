LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= campiperead.c 
LOCAL_MODULE:= campiperead

LOCAL_MODULE_TAG := optional
LOCAL_SHARED_LIBRARIES := libc libcutils libutils

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= campipewrite.c 
LOCAL_MODULE:= campipewrite

LOCAL_MODULE_TAG := optional
LOCAL_SHARED_LIBRARIES := libc libcutils libutils

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := com_test_camera_jni_campipe.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils \
    liblog 
    
LOCAL_MODULE := libcampipe-jni
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH:= x86
LOCAL_PROGUARD_ENABLED:= disabled
include $(BUILD_SHARED_LIBRARY)
