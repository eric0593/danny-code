#include <jni.h>
#include <stdio.h>
#include "android/log.h"
#include "com_test_camera_jni_campipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h> 
#include <string.h>
#include <pthread.h>
#include "scan3d.h"
#include <dlfcn.h>
#include <termios.h>

static const char *TAG="Scan3d";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)

pthread_t tid_read;
bool g_exit = false;
JavaVM *g_jvm = NULL;
jobject g_obj = NULL;
jclass g_cls = NULL;
//jmethodID callback = NULL;

//jmethodID callback = NULL;


void java_callback(int code,char * buf)
{

    JNIEnv *env=NULL;
    LOGI("callback Enter");
    int getEnvStat = g_jvm->GetEnv((void **) &env,JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED)
    {
        LOGI("JNI_EDETACHED");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGI("Attach failed");
            return;
        }
    }
 
    //jclass cls = env->FindClass("com/test/camera/jni/campipe");
    //if (cls == NULL) 
    //{
    //    LOGI("FindClass failed");
    //    return;
    //}
    
    jmethodID cb = env->GetStaticMethodID(g_cls, "onReceive", "(I[B)V");
    if (cb==NULL)
    {
         LOGI("GetStaticMethodID failed");
        return;
    }

    jbyteArray dataArray = env->NewByteArray(strlen(buf));
    env->SetByteArrayRegion(
                dataArray, 0, strlen(buf), (jbyte*) buf);
    env->CallStaticVoidMethod(g_cls,cb,0,dataArray);
    
    LOGI("jni callback exit");
    
}

static void sendCmd(char *cmd,int size)
{
    int fd;
    char buf[128] = {0};
    memcpy(buf,cmd,size);
    LOGI("sendCmd cmd=%c",buf[0]);

    fd = open("/dev/apptohal",O_RDWR);
    if (fd<0)
    {
        LOGI("File open apptohal failed. fd=%d",fd);
        return;
    }    
    write(fd, (void *)buf, 128);
    close(fd);
}

static void* read_thread(void* param)
{
    int fd; 
    fd_set rds;
    int ret,n=0;
    char Buf[128]={0};
    
    param =NULL;
    LOGI("read_thread start");

    fd = open("/dev/haltoapp",O_RDWR);
    if (fd<0)
    {
        LOGI("File open haltoapp failed. fd=%d",fd);
        return NULL;
    }
    
    FD_ZERO(&rds);
    FD_SET(fd, &rds);
    //struct timeval tv;
    //tv.tv_sec = 0; 
    //tv.tv_usec = 200*1000; 
    while (!g_exit)
    {
        ret = select(fd + 1, &rds, NULL, NULL, NULL);
        if (ret < 0)
        {
            LOGE("select error!\n");
            continue;
        }
        if (FD_ISSET(fd, &rds))
        {        
            lseek(fd,0,SEEK_SET);
            n = read(fd, Buf, 128);
            LOGI("Read from haltoapp -> %s",Buf);
            if (!strncmp(Buf,"reply",5))
                java_callback(0,Buf);
            if (!strncmp(Buf,"exit",4)&&g_exit)//cache exit in file
                break;
        }               
    }            
     
    close(fd);

    //buf_l[0] = 'd';
    LOGI ("read_thread exit \n");    
    pthread_join(tid_read,NULL);
    return NULL;
}





int init_read_thread(void) 
{ 

    pthread_attr_t attr;
    struct sched_param sched;
    
    LOGI("init_read_thread Enter");
    
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr,SCHED_RR);
    sched.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&attr,&sched);
    pthread_create(&tid_read, &attr, read_thread, (void*)NULL); 
    
    LOGI("init_read_thread exit");
    return 0;     
}


JNIEXPORT void JNICALL Java_com_test_camera_jni_campipe_init  
(JNIEnv* pEnv, jobject obj) 
{  
    LOGE("campipe_init");
    pEnv->GetJavaVM(&g_jvm);
    //g_obj = pEnv->NewGlobalRef(obj);
    jclass cls = pEnv->FindClass("com/test/camera/jni/campipe");
    g_cls = (jclass)pEnv->NewGlobalRef(cls);
    init_read_thread();
}  

JNIEXPORT void JNICALL Java_com_test_camera_jni_campipe_deinit  
(JNIEnv* pEnv, jobject obj) 
{  
    LOGE("campipe_deinit");
    //pEnv->DeleteGlobalRef(g_obj);
    pEnv->DeleteGlobalRef(g_cls); 
    g_exit = true;
    sendCmd("exit",4);
    pthread_join(tid_read,NULL);
}  


JNIEXPORT void JNICALL Java_com_test_camera_jni_campipe_sendCmd
  (JNIEnv *pEnv, jobject obj, jbyteArray cmd)
{
    LOGE("campipe_send start!");
    char *buf = (char*) pEnv->GetByteArrayElements(cmd, NULL);
    jsize len  = pEnv->GetArrayLength(cmd); 
    sendCmd((char *)buf,len);
    pEnv->ReleaseByteArrayElements(cmd, (jbyte *)buf,  0);
    LOGE("campipe_send exit!"); 
}


/*serialport*/
static speed_t getBaudrate(jint baudrate)
{
    switch(baudrate) {
    case 0: return B0;
    case 50: return B50;
    case 75: return B75;
    case 110: return B110;
    case 134: return B134;
    case 150: return B150;
    case 200: return B200;
    case 300: return B300;
    case 600: return B600;
    case 1200: return B1200;
    case 1800: return B1800;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 500000: return B500000;
    case 576000: return B576000;
    case 921600: return B921600;
    case 1000000: return B1000000;
    case 1152000: return B1152000;
    case 1500000: return B1500000;
    case 2000000: return B2000000;
    case 2500000: return B2500000;
    case 3000000: return B3000000;
    case 3500000: return B3500000;
    case 4000000: return B4000000;
    default: return -1;
    }
}

/*
 * Class:     android_serialport_SerialPort
 * Method:    open
 * Signature: (Ljava/lang/String;II)Ljava/io/FileDescriptor;
 */
JNIEXPORT jobject JNICALL Java_com_test_camera_jni_campipe_open_1uart
  (JNIEnv *env, jclass thiz, jstring path, jint baudrate, jint flags)
{
    int fd;
    speed_t speed;
    jobject mFileDescriptor;

    /* Check arguments */
    {
        speed = getBaudrate(baudrate);
        if (speed == -1) {
            /* TODO: throw an exception */
            LOGE("Invalid baudrate");
            return NULL;
        }
    }

    /* Opening device */
    {
        jboolean iscopy;
        const char *path_utf = env->GetStringUTFChars(path, &iscopy);
        LOGD("Opening serial port %s with flags 0x%x", path_utf, O_RDWR | flags);
        fd = open(path_utf, O_RDWR | flags);
        LOGD("open() fd = %d", fd);
        env->ReleaseStringUTFChars(path, path_utf);
        if (fd == -1)
        {
            /* Throw an exception */
            LOGE("Cannot open port");
            /* TODO: throw an exception */
            return NULL;
        }
    }

    /* Configure device */
    {
        struct termios cfg;
        LOGD("Configuring serial port");
        if (tcgetattr(fd, &cfg))
        {
            LOGE("tcgetattr() failed");
            close(fd);
            /* TODO: throw an exception */
            return NULL;
        }

        cfmakeraw(&cfg);
        cfsetispeed(&cfg, speed);
        cfsetospeed(&cfg, speed);

        if (tcsetattr(fd, TCSANOW, &cfg))
        {
            LOGE("tcsetattr() failed");
            close(fd);
            /* TODO: throw an exception */
            return NULL;
        }
    }

    /* Create a corresponding file descriptor */
    {
        jclass cFileDescriptor = env->FindClass( "java/io/FileDescriptor");
        jmethodID iFileDescriptor = env->GetMethodID( cFileDescriptor, "<init>", "()V");
        jfieldID descriptorID = env->GetFieldID( cFileDescriptor, "descriptor", "I");
        mFileDescriptor = env->NewObject( cFileDescriptor, iFileDescriptor);
        env->SetIntField(mFileDescriptor, descriptorID, (jint)fd);
    }

    return mFileDescriptor;
}

/*
 * Class:     cedric_serial_SerialPort
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_test_camera_jni_campipe_close_uart
  (JNIEnv *env, jobject thiz)
{
    jclass SerialPortClass = env->GetObjectClass( thiz);
    jclass FileDescriptorClass = env->FindClass("java/io/FileDescriptor");

    jfieldID mFdID = env->GetFieldID(SerialPortClass, "mFd", "Ljava/io/FileDescriptor;");
    jfieldID descriptorID = env->GetFieldID(FileDescriptorClass, "descriptor", "I");

    jobject mFd = env->GetObjectField(thiz, mFdID);
    jint descriptor = env->GetIntField(mFd, descriptorID);

    LOGD("close(fd = %d)", descriptor);
    close(descriptor);
}

