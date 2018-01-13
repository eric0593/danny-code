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

/* ------------------------------------------------------------------------
** Macros
** ------------------------------------------------------------------------ */


#define QUEUE_BUFF_MULTIPLE 1000

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
static char *out_pcm_name;

/* ------------------------------------------------------------------------
** Function Definitions
** ------------------------------------------------------------------------ */ 
static void* QueueReadThread(void* param);
static void* RecordThread(void* param);
static void record_audio_cb(const void *audio, unsigned int audio_len, int err_code);
int start_record(struct pro_config pcm_cfg);
void record_stop(void *record_hd);


static void* QueueReadThread(void* param)
{
	RecordData *record = (RecordData *)param;
	int readLen = 0;
	printf("QueueReadThread record :%x\n", record);
	while(record->runing){
		char *data_buff = NULL;
		readLen = queue_read(record->queue, &data_buff);
		
		if (0 == readLen){
			printf("queue_read readLen = 0\n");
			usleep(16000);
			continue;
		}
		if (record->flame_size != readLen){
			printf("buff_size != readLen\n");
		}
		record->cb(data_buff, readLen, SUCCESS);
		free(data_buff);
	}
	printf("QueueReadThread end \n");
	return NULL;
}


static void* RecordThread(void* param)
{
	RecordData *record = (RecordData *)param;
	int ret = 0;
	
	printf("RecordThread record:%x\n", record);
	
	printf("sched_setaffinity return = %d\n", ret);
	while (record->runing){
		ret = pcm_read(record->handle, record->buffer, record->flame_size);
		
		if (ret != 0) {
			fprintf(stderr,"Error capturing sample\n");
			break;	
		}
		
		queue_write(record->queue, record->buffer, record->flame_size);
	}
	printf("RecordThread end\n");
	return NULL;
}

static void record_audio_cb(const void *audio, unsigned int audio_len, int err_code)
{

	if (SUCCESS == err_code){
		FILE *fp = fopen(out_pcm_name, "ab+");
		if (NULL == fp){
			printf("fopen error\n");
			return;
		}
		fwrite(audio, audio_len, 1, fp);
		fclose(fp);
	}

}

int start_record(struct pro_config pcm_cfg)
{
	int rc;  
	int size;  
	int ret = SUCCESS;
	pthread_attr_t thread_attr;
	struct sched_param thread_param;
	unsigned int frames;  
	char *buffer;  
    RecordData *record = NULL;
	record = (RecordData *)malloc(sizeof(RecordData));	
	if (NULL == record){
		return ERROR_OUT_OF_MEMORY;
	}
	memset(record, 0, sizeof(RecordData));
	record->cb = record_audio_cb;
	
	
		// pcm配置结构体
	struct pcm_config config;


	// 清空配置结构
    memset(&config, 0, sizeof(config));
	// 初始化配置列表
    config.channels = pcm_cfg.channels;
    config.rate = pcm_cfg.rate;
    config.period_size = pcm_cfg.period_size;
    config.period_count = pcm_cfg.period_count;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

	
	// 用配置参数打开声卡设备
    record->handle = pcm_open(pcm_cfg.card, pcm_cfg.device, PCM_IN, &config);
    if (!record->handle || !pcm_is_ready(record->handle)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n", pcm_get_error(record->handle));
        return 0;
    }
	// 获取周期大小
    record->flame_size = pcm_frames_to_bytes(record->handle, pcm_get_buffer_size(record->handle));
    printf("Capturing sample:%uch,%uhz,%ubit,%d\n", config.channels, config.rate,
           pcm_format_to_bits(config.format),size);

    // 构建一个足够大的区域缓存一个周期音频数据
	/* Use a buffer large enough to hold one period */  
	record->buff_size = record->flame_size*8;
	printf("record->buff_size=%d\n",record->buff_size);
	record->buffer = (char *) malloc(record->buff_size);  
	if (NULL == record->buffer){
		ret = ERROR_OUT_OF_MEMORY;
		goto error;
	}
	
    // 分配一个队列，用来接收缓存中数据
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
	
	printf("record_start record :%x\n", record);
	pthread_create(&record->tid_pcm_read, &thread_attr, RecordThread, (void*)record);
	pthread_create(&record->tid_queue_read, NULL, QueueReadThread, (void*)record);
	
	while(1)
	{}
	
	
goto exit;

error:
	printf("record start error %d\n", ret);
	record_stop(record);

exit:
	printf("record start out %d\n", ret);
	
	record_stop(record);
	return ret;
	
}
int main(int argc, char **argv)
{
	if (argc < 2) {
        fprintf(stderr, "Usage: %s file.wav [-D card] [-d device] [-c channels] "
                "[-r rate] [-b bits] [-p period_size] [-n n_periods]\n", argv[0]);
        return 1;
    }
	int card = 2;
    int device = 0;
    unsigned int channels = 2;
    unsigned int rate = 64000;

    unsigned int frames;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
   
	
	out_pcm_name = argv[1];
	struct pro_config param_cfg;
	
	/* parse command line arguments */
    argv += 2;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            argv++;
            if (*argv)
                device = atoi(*argv);
        } else if (strcmp(*argv, "-c") == 0) {
            argv++;
            if (*argv)
                channels = atoi(*argv);
        } else if (strcmp(*argv, "-r") == 0) {
            argv++;
            if (*argv)
                rate = atoi(*argv);
        } else if (strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                card = atoi(*argv);
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        } else if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
        }
        if (*argv)
            argv++;
    }
	
	param_cfg.card = card;
	param_cfg.device = device;
	param_cfg.channels = channels;
	param_cfg.rate = rate;
	param_cfg.period_size = period_size;
	param_cfg.period_count = period_count;
	
	
	printf("++++++++++++++++++++++++++\n");
	printf("card is %d\n",param_cfg.device);
	printf("device is %d\n",param_cfg.device);
	printf("Channle is %d\n",param_cfg.channels);
	printf("rate name is %d\n",param_cfg.rate);
	printf("period_size name is %d\n",param_cfg.period_size);
	printf("++++++++++++++++++++++++++\n");
	
	start_record(param_cfg);
	
	return 0;
	//==========================================
	
	
}

void record_stop(void *record_hd)
{
	RecordData *record = (RecordData*)record_hd;

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
	}
	printf("\nrecord_stop out\n");
}
