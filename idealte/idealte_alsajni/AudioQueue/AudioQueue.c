/*
 * AudioQueue.cpp
 *
 *  Created on: 2015年8月17日
 *      Author: ican
 */

#include "AudioQueue.h"

/**
 * base 分配给队列的内存块首地址，内存长度为sizeof(audio_queue_t) + capacity
 * capacity 队列数据区大小，队列实际容量为capacity - 1
 */
audio_queue_t* queue_init(void* base, int capacity)
{
	if (NULL == base) {
		return NULL;
	}

	audio_queue_t* queue = (audio_queue_t*) base;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	pthread_mutex_init(&(queue->mutex), &attr);

	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);
	pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_PRIVATE);
	pthread_cond_init(&(queue->cond), &condattr);

	queue->capacity = capacity;
	queue->front = 0;
	queue->rear = 0;
	queue->more = true;

	return queue;
}

char* queue_get_write_addr(audio_queue_t* queue)
{
	if (NULL != queue) {
		char* queueBase = (char*)(queue + 1);
		char* begin = &((queueBase)[queue->rear]);

		return begin;
	}
	return NULL;
}

void queue_destroy(audio_queue_t* queue)
{
	if (NULL != queue) {
		pthread_mutex_destroy(&(queue->mutex));
	}
}

int queue_real_capacity(audio_queue_t* queue)
{
	return queue->capacity - 1;
}

int queue_front(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int front = (queue->front);
	pthread_mutex_unlock(&(queue->mutex));

	return front;
}

int queue_rear(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int rear = (queue->rear);
	pthread_mutex_unlock(&(queue->mutex));

	return rear;
}

int queue_len(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int len = (queue->rear - queue->front + queue->capacity) % queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));

	return len;
}

int queue_len_unlock(audio_queue_t* queue)
{
	return (queue->rear - queue->front + queue->capacity) % queue->capacity;
}

int queue_left(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int left = queue->capacity - 1 - queue_len_unlock(queue);
	pthread_mutex_unlock(&(queue->mutex));

	return left;
}

int queue_left_unlock(audio_queue_t* queue)
{
	return queue->capacity - 1 - queue_len_unlock(queue);
}

int queue_empty(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int empty = queue->rear == queue->front;
	pthread_mutex_unlock(&(queue->mutex));

	return empty;
}

int queue_full(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int full = ((queue->rear + 1 + queue->capacity) % queue->capacity == queue->front);
	pthread_mutex_unlock(&(queue->mutex));

	return full;
}

int queue_write(audio_queue_t* queue, char data[], int dataLen)
{
	if (queue == NULL || data == NULL || dataLen <= 0) {
		return false;
	}

	pthread_mutex_lock(&(queue->mutex));
	if (queue_left_unlock(queue) < dataLen) {
		pthread_mutex_unlock(&(queue->mutex));
		return false;
	}

	// 计算数据区起始地址
	char* queueBase = (char*)(queue + 1);
	char* begin = &((queueBase)[queue->rear]);
	if (queue->rear + dataLen <= queue->capacity - 1) {
		memcpy(begin, data, dataLen);
	} else {
		// 分两段写入
		int dataLen1 = queue->capacity - 1 - queue->rear + 1;
		int dataLen2 = dataLen - dataLen1;

		memcpy(begin, data, dataLen1);
		memcpy(queueBase, data + dataLen1, dataLen2);
	}

	queue->rear = (queue->rear + dataLen) % queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));

	pthread_cond_signal(&(queue->cond));

	return true;
}

int queue_read(audio_queue_t* queue, char data[], int readLen)
{
	if (queue == NULL || data == NULL || readLen <= 0) {
		return 0;
	}
	pthread_mutex_lock(&(queue->mutex));

	// 计算数据区起始地址
	char* queueBase = (char*)(queue + 1);
	char* begin = &((queueBase)[queue->front]);
	int queueLen = queue_len_unlock(queue);

//	if (readLen > queueLen) {
//		readLen = queueLen;
//	}

	// 队列中的数据长度不够，则继续等待
	while (queueLen < readLen) {
		pthread_cond_wait(&(queue->cond), &(queue->mutex));
		queueLen = queue_len_unlock(queue);
	}

	if (queue->front + readLen <= queue->capacity - 1) {
		memcpy(data, begin, readLen);
	} else {
		// 分两段读取
		int readLen1 = queue->capacity - 1 - queue->front + 1;
		int readLen2 = readLen - readLen1;

		memcpy(data, begin, readLen1);
		memcpy(&data[readLen1], queueBase, readLen2);
	}

	queue->front = (queue->front + readLen) % queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));

	return readLen;
}

void queue_set_more(audio_queue_t* queue, int more)
{
	pthread_mutex_lock(&(queue->mutex));
	queue->more = more;
	pthread_mutex_unlock(&(queue->mutex));
}

int queue_get_more(audio_queue_t* queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int end = queue->more;
	pthread_mutex_unlock(&(queue->mutex));

	return end;
}
