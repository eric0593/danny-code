#ifndef __ODMSZ_COMMON_H__
#define __ODMSZ_COMMON_H__
#include "stdio.h"
#include "stdlib.h"
#include "log/log.h"
#define ODMSZ_DEBUG
#ifdef ODMSZ_ANDROID
#define ODMSZ_LOG_E(format, ...) ALOGE(format, ##__VA_ARGS__)
#ifdef ODMSZ_DEBUG
#define ODMSZ_LOG_D(format, ...) ALOGD(format, ##__VA_ARGS__)
#else
#define ODMSZ_LOG_D(format, ...)
#endif/*ODMSZ_DEBUG*/
#else
#define ODMSZ_LOG_E(format, ...) printf (format, ##__VA_ARGS__)
#ifdef ODMSZ_DEBUG
#define ODMSZ_LOG_D(format, ...) printf (format, ##__VA_ARGS__)
#else
#define ODMSZ_LOG_D(format, ...)
#endif/*ODMSZ_DEBUG*/
#endif/*ODMSZ_ANDROID*/

#define _LINE_LENGTH 1024
#define _MAX_SENT_BYTES 4096
#endif

