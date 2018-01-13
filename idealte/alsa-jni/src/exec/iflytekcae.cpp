/*
 * iflytekcae.cpp
 *
 *  Created on: 2015年10月10日
 *      Author: admin
 */

#include "cae.h"
#include "../tinyalsa/asoundlib.h"
#include "../queue/AudioQueue.h"
#include "../log/JniLog.h"
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <asm/signal.h>

#define SAMPLING_RATE_8K    8000
#define SAMPLING_RATE_11K   11025
#define SAMPLING_RATE_16K   16000
#define SAMPLING_RATE_44K   44100
#define SAMPLING_RATE_48K   48000
#define SAMPLING_RATE_96K   96000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 声卡设置
struct pcm_config pcm_config;
// pcm设备全局指针
struct pcm* gPCM = NULL;
// 音频缓冲队列
struct audio_queue_t* gAudioQueue = NULL;
// 队列所在内存首地址，用于delete内存
char* gQueueBase = NULL;

CAE_HANDLE gCAEHandle = NULL;

pcm* open_pcm(int card)
{
	pcm_config.channels = 2;
	pcm_config.rate = SAMPLING_RATE_96K;
	pcm_config.period_size = 1536;
	pcm_config.period_count = 8;
	pcm_config.format = PCM_FORMAT_S24_LE;
	pcm_config.start_threshold = 0;
	pcm_config.stop_threshold = 0;
	pcm_config.silence_threshold = 0;

	struct pcm* pcm = pcm_open(card, 0, PCM_IN, &pcm_config);
	if (NULL == pcm || !pcm_is_ready(pcm)) {
		LOGE("pcm_open | unable to open PCM IN Device %s", pcm_get_error(pcm));
		return NULL;
	}

	LOGE("pcm_open | pcm card %d open success", card);
	return pcm;
}

int get_buffer_size(pcm* pcm)
{
	int size = 0;
	if (NULL != pcm) {
		size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
	} else {
		LOGE("getBufferSize | pcm is null");
		size = -1;
	}

	return size;
}

// 唤醒回调函数
void ivw_cbf(short angle, short channel, float power, short CMScore, short beam, void *userData)
{
	LOGD("ivw_cbf | wakeup success");
}

// 音频回调函数
void audio_cbf(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData)
{

}

CAE_HANDLE createCAE(char* resPath)
{
	CAE_HANDLE cae;
	CAENew(&cae, resPath, ivw_cbf, audio_cbf, NULL, NULL);
	LOGD("CAENew | cae handle:%d", (int) cae);

	return cae;
}

void* CAEThreadFunc(void* param)
{
	if (NULL == gAudioQueue)
	{
		LOGE("CAEThreadFunc | audioQueue is null!");
		return NULL;
	}

	int bufferSize = (int) param;
	int outLen;
	char* data = new char[bufferSize];
	char* out = new char[bufferSize];

	while (true)
	{
		int readLen = queue_read(gAudioQueue, data, (int) bufferSize);
		LOGD("CAEThreadFunc | queue_read %d bytes", readLen);

		if (NULL != gCAEHandle) {
			int ret = CAEAudioWrite(gCAEHandle, data, bufferSize);
			LOGD("CAEAudioWrite | ret = %d", ret);

			CAEExtract16K((void*)data, bufferSize, 0, (void*) out, &outLen);
		}
	}

	delete data;
	delete out;
}

void* ReadThreadFunc(void* param)
{
	int bufferSize = (int) param;

	char buffer[bufferSize];
	while (true)
	{
		pcm_read(gPCM, buffer, bufferSize);
		if (NULL != gAudioQueue) {
			queue_write(gAudioQueue, buffer, bufferSize);
		}
	}
}

bool is_pcm_opened(char* pcmDriverFile)
{
	char cmd[80];
	sprintf(cmd, "lsof |grep /dev/snd/%s", pcmDriverFile);

	FILE* fp = popen(cmd, "r");
	if (NULL == fp) {
		printf("cmd exec failed\n");
		return false;
	}

	char buffer[200];
	fgets(buffer, sizeof(buffer), fp);
	pclose(fp);

	if (strstr(buffer, pcmDriverFile) != NULL) {
		return true;
	}

	return false;
}

void* PcmOpenThreadFunc(void* param)
{
	const int card = 3;
	pcm* pcm = open_pcm(card);
	gPCM = pcm;

	pthread_cond_signal(&cond);

	printf("signal end.\n");

	return NULL;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("arguments less than 2. usage:%s resPath\n", argv[0]);
		return -1;
	}

	if (is_pcm_opened("pcmC3D0c")) {
		printf("pcm device already opened, cannot open twice.\n");
		return -1;
	}

	gCAEHandle = createCAE(argv[1]);
	printf("cae create success, handle = %d\n", (int) gCAEHandle);

	pthread_t pcmOpenTid;
	pthread_create(&pcmOpenTid, NULL, PcmOpenThreadFunc, NULL);

	timeval now;
	timespec spec;

	gettimeofday(&now, NULL);
	spec.tv_sec = now.tv_sec + 5;
	spec.tv_nsec = 0;

	pthread_mutex_lock(&mutex);

	printf("wait open result.\n");
	pthread_cond_timedwait(&cond, &mutex, &spec);

	pthread_mutex_unlock(&mutex);

	if (NULL == gPCM) {
		printf("pcm device already opened, cannot open twice.\n");
		return -1;
	}

	int bufferSize = get_buffer_size(gPCM);
	int queueSize = bufferSize * 4;

	gQueueBase = new char[sizeof(audio_queue_t) + queueSize + 1];
	gAudioQueue = queue_init(gQueueBase, queueSize + 1);

	pthread_t tid1;
	int ret = pthread_create(&tid1, NULL, CAEThreadFunc, (void*) bufferSize);

	pthread_attr_t thread_attr;
	struct sched_param thread_param;

	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	thread_param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_attr_setschedparam(&thread_attr, &thread_param);

	pthread_t tid2;
	pthread_create(&tid2, &thread_attr, ReadThreadFunc, (void*) bufferSize);

	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);

//	char buffer[bufferSize];
//	while (true)
//	{
//		pcm_read(pcm, buffer, bufferSize);
//		if (NULL != gAudioQueue) {
//			queue_write(gAudioQueue, buffer, bufferSize);
//		}
//	}

	return 0;
}
