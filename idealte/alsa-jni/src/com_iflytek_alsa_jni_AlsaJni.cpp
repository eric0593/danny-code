/*
 * com_iflytek_alsa_jni_AlsaJni.cpp
 *
 *  Created on: 2015年10月9日
 *      Author: admin
 */

#include "../com_iflytek_alsa_jni_AlsaJni.h"
#include "tinyalsa/asoundlib.h"
#include "queue/AudioQueue.h"
#include "log/JniLog.h"
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>

#define SAMPLING_RATE_8K    8000
#define SAMPLING_RATE_11K   11025
#define SAMPLING_RATE_16K   16000
#define SAMPLING_RATE_44K   44100
#define SAMPLING_RATE_48K   48000
#define SAMPLING_RATE_64K   64000
#define SAMPLING_RATE_96K   96000

// 声卡设置
struct pcm_config pcm_config;
// pcm设备全局指针
struct pcm* gPCM = NULL;
// 音频缓冲队列
struct audio_queue_t* gAudioQueue = NULL;
// 队列所在内存首地址，用于delete内存
char* gQueueBase = NULL;

bool gIsRecording = false;
bool gStopRecord = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 将pcm指针保存到java对象的pcmHandle变量
void setPcmHandle(JNIEnv *pEnv, jobject alsaRecorder, int handle)
{
	jclass alsaRecorderClass = pEnv->GetObjectClass(alsaRecorder);
	jfieldID pcmHandle = pEnv->GetStaticFieldID(alsaRecorderClass, "pcmHandle", "I");
	pEnv->SetStaticIntField(alsaRecorderClass, pcmHandle, handle);
}

// 录音线程函数，不断从pcm设置读取数据，写入到队列
void* RecordThreadFunc(void* param)
{
//	cpu_set_t mask;
//	CPU_ZERO(&mask);
//	CPU_SET(0,&mask);
//	sched_setaffinity(0, sizeof(mask), &mask);

	int readSize = 0;
	if (NULL != gPCM)
	{
		readSize = (int) param;
	} else {
		return NULL;
	}

	gIsRecording = true;
	char* buffer = new char[readSize];
	while (!gStopRecord)
	{
		pthread_mutex_lock(&mutex);

		int ret;
		if (NULL != gPCM)
		{
			ret = pcm_read(gPCM, buffer, readSize);
		}

		pthread_mutex_unlock(&mutex);
        
    LOGE("pcm_read size=%d buffer: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",readSize,
							buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
							 
         
		if (0 != ret)
		{
			LOGE("RecordThreadProc | pcm_read ret = %d", ret);
			break;
		}

		if (NULL != gAudioQueue) {
			queue_write(gAudioQueue, buffer, readSize);
		}
	}

	gIsRecording = false;
	delete buffer;
	return NULL;
}

JNIEXPORT void JNICALL Java_com_iflytek_alsa_jni_AlsaJni_showJniLog
  (JNIEnv *pEnv, jclass clazz, jboolean show)
{
	gSetShowLog(show);
}

JNIEXPORT jboolean JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1is_1opened
  (JNIEnv *pEnv, jclass clazz, jstring pcmDriver)
{
	const char* pcmDriverChar = pEnv->GetStringUTFChars(pcmDriver, NULL);
	char cmd[80];

	execl("/system/xbin/su", "");
	sprintf(cmd, "lsof /dev/snd/%s\n", pcmDriverChar);
	FILE* fp = popen(cmd, "r");
	if (NULL == fp) {
		LOGE("pcm_is_opened | \"%s\" execute failed.", cmd);
		return false;
	}

	char buffer[1000];
	fread(buffer, sizeof(buffer), 1, fp);
	pclose(fp);

	LOGD("pcm_is_opened | cmd ret = %s.", buffer);

	bool isOpened = false;
	if (strstr(buffer, pcmDriverChar) != NULL) {
		isOpened = true;
	}
	pEnv->ReleaseStringUTFChars(pcmDriver, pcmDriverChar);

	return isOpened;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1open
  (JNIEnv *pEnv, jclass clazz, jint card, jint sampleRate, jobject alsaRecorder)
{
	pcm_config.channels = 2;
	pcm_config.rate = sampleRate;
	pcm_config.period_size = 1536;
	pcm_config.period_count = 8;
	pcm_config.format = PCM_FORMAT_S24_LE;
	pcm_config.start_threshold = 0;
	pcm_config.stop_threshold = 0;
	pcm_config.silence_threshold = 0;

	struct pcm* pcm = pcm_open(card, 0, PCM_IN, &pcm_config);	
	setPcmHandle(pEnv, alsaRecorder, (int) pcm);
    LOGE("pcm_open | open PCM Device period_size=%d", pcm_config.period_size);
	if (NULL == pcm || !pcm_is_ready(pcm)) {
		LOGE("pcm_open | unable to open PCM IN Device %s", pcm_get_error(pcm));
		return -EINVAL;
	}
	LOGD("pcm_open | pcm card %d open success", card);

	gPCM = pcm;
	gStopRecord = false;

	return 0;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1buffer_1size
  (JNIEnv *pEnv, jclass clazz, jint pcmHandle)
{
	int size = 0;
	if (NULL != pcmHandle) {
		pcm* pcm = (struct pcm*) pcmHandle;
		size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
	} else {
		LOGE("pcm_buffer_size | pcmHandle is null");
		size = -1;
	}

	return size;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1read
  (JNIEnv *pEnv, jclass clazz, jbyteArray dataArray, jint count)
{
	if (NULL == gAudioQueue)
	{
		LOGE("pcm_read | audioQueue is null!");
		return 0;
	}

	void* dataVoid = (void*) pEnv->GetByteArrayElements(dataArray, NULL);
	int readLen = queue_read(gAudioQueue, (char*) dataVoid, (int) count);
	pEnv->ReleaseByteArrayElements(dataArray, (signed char*)dataVoid, 0);

	return readLen;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1start_1record
  (JNIEnv *pEnv, jclass clazz, jint readSize, jint queueSize)
{
	// 防止重复start
	if (gIsRecording)
	{
		LOGE("start_record | recording thread already started!");
		return -1;
	}

	gQueueBase = new char[sizeof(audio_queue_t) + queueSize + 1];
	gAudioQueue = queue_init(gQueueBase, queueSize + 1);

	pthread_attr_t thread_attr;
	struct sched_param thread_param;

	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	thread_param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_attr_setschedparam(&thread_attr, &thread_param);

	pthread_t tid;
	int ret = pthread_create(&tid, &thread_attr, RecordThreadFunc, (void*) readSize);

	return ret;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1close
  (JNIEnv *pEnv, jclass clazz, jint pcmHandle)
{
	pthread_mutex_lock(&mutex);

	gStopRecord = true;

	int ret = 0;
	if (NULL != pcmHandle)
	{
		ret = pcm_close((pcm*) pcmHandle);
		gPCM = NULL;
	}

	if (NULL != gAudioQueue)
	{
		queue_destroy(gAudioQueue);
		gAudioQueue = NULL;
		delete gQueueBase;
		gQueueBase = NULL;
	}

	pthread_mutex_unlock(&mutex);

	return ret;
}
