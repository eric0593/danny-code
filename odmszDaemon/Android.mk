
LOCAL_PATH:= $(call my-dir)

ifneq (ODMSZ_PREBUILT,true)

include $(CLEAR_VARS)

LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := odmszDaemon
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

else

include $(CLEAR_VARS)
LOCAL_MODULE        := odmszDaemon
LOCAL_MODULE_OWNER  := qcom
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := APPS
LOCAL_CERTIFICATE   := platform
LOCAL_MODULE_SUFFIX := .apk
LOCAL_SRC_FILES     := odmszDaemon.apk
LOCAL_MODULE_PATH   := $(PRODUCT_OUT)/system/app/
include $(BUILD_PREBUILT)

endif
# Use the following include to make our test apk.

