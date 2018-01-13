/*
 * JniLog.h
 *
 *  Created on: 2015年10月9日
 *      Author: admin
 */

#ifndef JNI_SRC_LOG_JNILOG_H_
#define JNI_SRC_LOG_JNILOG_H_

#include <android/log.h>
#include <jni.h>

bool gShowLog = JNI_TRUE;

#define LOG_TAG "Alsa-Jni"
#define LOGV(...)  if (gShowLog) { \
						__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__); \
					}
#define LOGI(...)  if (gShowLog) { \
						__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); \
					}
#define LOGD(...)  if (gShowLog) { \
						__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); \
					}
#define LOGE(...)  if (gShowLog) { \
						__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); \
					}

void gSetShowLog(bool show)
{
	gShowLog = show;
}

#endif /* JNI_SRC_LOG_JNILOG_H_ */
