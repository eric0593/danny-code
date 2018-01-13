/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_iflytek_alsa_jni_AlsaJni */

#ifndef _Included_com_iflytek_alsa_jni_AlsaJni
#define _Included_com_iflytek_alsa_jni_AlsaJni
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_iflytek_alsa_jni_AlsaJni
 * Method:    showJniLog
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_iflytek_alsa_jni_AlsaJni_showJniLog
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_iflytek_alsa_jni_AlsaJni
 * Method:    pcm_open
 * Signature: (IILjava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1open
  (JNIEnv *, jclass, jint, jint, jobject);

/*
 * Class:     com_iflytek_alsa_jni_AlsaJni
 * Method:    pcm_buffer_size
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1buffer_1size
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_iflytek_alsa_jni_AlsaJni
 * Method:    pcm_start_record
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1start_1record
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_iflytek_alsa_jni_AlsaJni
 * Method:    pcm_read
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1read
  (JNIEnv *, jclass, jbyteArray, jint);

/*
 * Class:     com_iflytek_alsa_jni_AlsaJni
 * Method:    pcm_close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_iflytek_alsa_jni_AlsaJni_pcm_1close
  (JNIEnv *, jclass, jint);

#ifdef __cplusplus
}
#endif
#endif