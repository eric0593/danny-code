/* =========================================================================

                             REVISION

when            who              why
--------        ---------        -------------------------------------------
2015/10/15     kunzhang        Created.
2016/10/14     leqi            Modify for Record.
============================================================================ */
/* ------------------------------------------------------------------------
** Includes
** ------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include "asoundlib.h"
#include "pthread.h"
#include "AudioQueue/cae_thread.h"
#include "AudioQueue/AudioQueue.h"
#include "record.h"
#include "error.h"
#include "com_iflytek_alsa_jni_AlsaJni.h"
#include "JniLog.h"

#include <sys/capability.h>
#include <linux/prctl.h>
#include <errno.h>
#include <sys/prctl.h>
#include <private/android_filesystem_config.h>

/* ------------------------------------------------------------------------
** Macros
** ------------------------------------------------------------------------ */


#define QUEUE_BUFF_MULTIPLE 10

/* ------------------------------------------------------------------------
** Types
** ------------------------------------------------------------------------ */
typedef struct _RecordData{
    struct pcm *handle;
    audio_queue_t *queue;
    char *queue_buff;
    char *buffer; 
    int buff_size;
    int flame_size;
    unsigned int frames; 
    record_audio_fn cb;
    void *user_data;
    pthread_t tid_pcm_read;
    pthread_t tid_queue_read;
    int runing;
}RecordData;

/* ------------------------------------------------------------------------
** Global Variable Definitions
** ------------------------------------------------------------------------ */
static char *out_pcm_name="/sdcard/record_demo.pcm";
static int g_fwrite = false;
/* ------------------------------------------------------------------------
** Function Definitions
** ------------------------------------------------------------------------ */ 
static void* QueueReadThread(void* param);
static void* RecordThread(void* param);
static void record_audio_cb(const void *audio, unsigned int audio_len, int err_code);


void switchUser() {
    gid_t groups[] = {AID_NET_ADMIN,AID_NET_RAW,AID_INET};
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setgroups(sizeof(groups)/sizeof(groups[0]),groups);
    setuid(10000);

    struct __user_cap_header_struct header;

    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW);
    cap.inheritable = 0;
    capset(&header, &cap);

}


static void* QueueReadThread(void* param)
{
    RecordData *record = (RecordData *)param;
    int readLen = 0;    
    char *data_buff = NULL;
    
    LOGE("QueueReadThread record :%x\n", record);   
    data_buff = (char *) malloc(record->flame_size);
    if (data_buff==NULL)
    {       
        LOGE("QueueReadThread data_buff malloc failed\n");
        return NULL;
    }
    while(record->runing){
        readLen = queue_read(record->queue, (char*) data_buff, (int) record->flame_size);
        
        if (0 == readLen){
            LOGE("queue_read readLen = 0\n");
            usleep(16000);
            continue;
        }
        if (record->flame_size != readLen){
            LOGE("buff_size != readLen\n");
        }
        record->cb(data_buff, readLen, SUCCESS);
    }
    
    free(data_buff);
    LOGE("QueueReadThread end \n");
    return NULL;
}


static void* RecordThread(void* param)
{
    RecordData *record = (RecordData *)param;
    int ret = 0;
    
    LOGE("RecordThread record:%x\n", record);
    
    LOGE("sched_setaffinity return = %d\n", ret);
    while (record->runing){		
        ret = pcm_read(record->handle, record->buffer, record->flame_size);
        LOGE("pcm_read size=%d buffer: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",record->flame_size,
							record->buffer[0],record->buffer[1],record->buffer[2],record->buffer[3],
							record->buffer[4],record->buffer[5],record->buffer[6],record->buffer[7]);   		
        if (ret != 0) {
            LOGE("Error capturing sample\n");
            break;  
        }
        
        queue_write(record->queue, record->buffer, record->flame_size);
    }
    LOGE("RecordThread end\n");
    return NULL;
}

static void record_audio_cb(const void *audio, unsigned int audio_len, int err_code)
{

    if (SUCCESS == err_code){
        FILE *fp = fopen(out_pcm_name, "ab+");
        if (NULL == fp){
            LOGE("fopen error\n");
            return;
        }
        fwrite(audio, audio_len, 1, fp);
        fclose(fp);
    }

}

void * pcm_open(int card,int sampleRate)
{
    struct pcm_config config;
    
    RecordData *record = NULL;
    record = (RecordData *)malloc(sizeof(RecordData));  
    if (NULL == record){
        return NULL;
    }
    memset(record, 0, sizeof(RecordData));
    record->cb = record_audio_cb;


    // 清空配置结构
    memset(&config, 0, sizeof(config));
    // 初始化配置列表
    config.channels = 2;
    config.rate = sampleRate;
    config.period_size = 1536;
    config.period_count = 8;
    config.format = PCM_FORMAT_S24_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    
    LOGE("pcm_open card=%d sampleRate=%d\n", card,sampleRate);
    // 用配置参数打开声卡设备
    record->handle = pcm_open(card, 0, PCM_IN, &config);
    if (!record->handle || !pcm_is_ready(record->handle)) {
        LOGE("Unable to open PCM device (%s)\n", pcm_get_error(record->handle));
        return NULL;
    }
    return record;
}

void record_stop(void *param)
{
    RecordData *record = (RecordData *)param;

    if (NULL != record){
        record->runing = 0; 
        pthread_join(record->tid_pcm_read, NULL);
        pthread_join(record->tid_queue_read, NULL); 
        pcm_close(record->handle); 

        if (NULL != record->queue){
            queue_destroy(record->queue);
        }
    
        if (NULL != record->buffer){
            free(record->buffer);
        }
        free(record);
        record = NULL;
    }
    LOGE("\nrecord_stop out\n");
}


int start_record(void *param,int readSize, int queueSize)
{
    int rc;  
    int size;  
    int ret = SUCCESS;
    pthread_attr_t thread_attr;
    struct sched_param thread_param;
    unsigned int frames;  
    char *buffer;  

    RecordData *record = (RecordData *)param;

    if (record==NULL||!record->handle)
    {
        return ERROR_FAIL;
    }
    
    record->flame_size = pcm_frames_to_bytes(record->handle, pcm_get_buffer_size(record->handle));
    //LOGE("Capturing sample:%uch,%uhz,%ubit,%d\n", config.channels, config.rate,pcm_format_to_bits(config.format),size);
           
    /* Use a buffer large enough to hold one period */  
    record->buff_size = record->flame_size*8;
    LOGE("record->buff_size=%d\n",record->buff_size);
    record->buffer = (char *) malloc(record->buff_size);  
    if (NULL == record->buffer){
        ret = ERROR_OUT_OF_MEMORY;
        goto error;
    }
    
    record->queue_buff = (char *) malloc(sizeof(audio_queue_t) + record->buff_size * QUEUE_BUFF_MULTIPLE + 1);
    if (NULL == record->queue_buff){
        ret = ERROR_OUT_OF_MEMORY;
        goto error;
    }
    record->queue = queue_init(record->queue_buff, record->buff_size * QUEUE_BUFF_MULTIPLE + 1);
    
    pthread_attr_init(&thread_attr);
    pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
    thread_param.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&thread_attr, &thread_param);    
    record->runing = 1;
    
    LOGE("record_start record :%x\n", record);
    pthread_create(&record->tid_pcm_read, &thread_attr, RecordThread, (void*)record);
    if (g_fwrite)
    {
        pthread_create(&record->tid_queue_read, NULL, QueueReadThread, (void*)record);
    }
    
goto exit;

error:
    LOGE("record start error %d\n", ret);
    record_stop((void *)record);

exit:
    LOGE("record start out %d\n", ret);
    return ret;
    
}




void setPcmHandle(JNIEnv *pEnv, jobject alsaRecorder, int handle)
{
    jclass alsaRecorderClass = pEnv->GetObjectClass(alsaRecorder);
    jfieldID pcmHandle = pEnv->GetStaticFieldID(alsaRecorderClass, "pcmHandle", "I");
    pEnv->SetStaticIntField(alsaRecorderClass, pcmHandle, handle);
}

JNIEXPORT void JNICALL Java_com_iflytek_alsa_jni_AlsaJni_showJniLog
  (JNIEnv *pEnv, jclass clazz, jboolean show)
{
    gSetShowLog(show);
}


JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1open
  (JNIEnv *pEnv, jclass clazz, jint card, jint sampleRate,jobject alsaRecorder)
{
    LOGE ("card=%d,sampleRate=%d,alsaRecorder=%x",card,sampleRate,(int)alsaRecorder);
    void *record =pcm_open(card,sampleRate);
    if (record != NULL)
    {
        setPcmHandle(pEnv,alsaRecorder,(int)record);
        return SUCCESS;
    }
    return ERROR_FAIL;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1buffer_1size
  (JNIEnv *pEnv, jclass clazz, jint pcmHandle)
{
    int size = 0;
    if (NULL != pcmHandle) {
        RecordData* record = (RecordData*) pcmHandle;
        size = pcm_frames_to_bytes(record->handle, pcm_get_buffer_size(record->handle));
    } else {
        LOGE("pcm_buffer_size | pcmHandle is null");
        size = -1;
    }

    return size;
}


JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1read
  (JNIEnv *pEnv, jclass clazz, jint pcmHandle, jbyteArray dataArray, jint count)
{
    
    if (NULL != pcmHandle) {
        RecordData* record = (RecordData*) pcmHandle;
        if (NULL == record->queue)
        {
            LOGE("pcm_read | audioQueue is null!");
            return 0;
        }

        void* dataVoid = (void*) pEnv->GetByteArrayElements(dataArray, NULL);
        int readLen = queue_read(record->queue, (char*) dataVoid, (int) count);
        char * printStr = (char*)dataVoid;
        pEnv->ReleaseByteArrayElements(dataArray, (signed char*)dataVoid, 0);
		LOGE("alsarecorder read size=%d buffer: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",readLen,
													printStr[0],printStr[1],printStr[2],printStr[3],
													printStr[4],printStr[5],printStr[6],printStr[7]); 

        return readLen;
    }
    return 0;
}

JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1start_1record
  (JNIEnv *pEnv, jclass clazz,jint pcmHandle,jint readSize, jint queueSize)
{
    return start_record((void *)pcmHandle,readSize,queueSize);
}


JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1close
  (JNIEnv *pEnv, jclass clazz,jint pcmHandle)
{
    record_stop((void *)pcmHandle);
    return 0;
}




int main (int argc,char ** argv)
{   
    //switchUser();
    
    RecordData* record = (RecordData*)pcm_open(0,96000);
    if (record == NULL)
    {
        LOGE("pcm open failed, exit\n");
        return 0;
    }

    int frame_size = pcm_frames_to_bytes(record->handle, pcm_get_buffer_size(record->handle));
    start_record((void *)record,frame_size,frame_size*50);
    while(1)
    {
    
    };
}

