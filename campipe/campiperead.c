#include <stdio.h>
#include <stdlib.h>
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
static int bExit=0;

static void* ReadThread(void* param)
{
    int fd; 
    fd_set rds;    //声明描述符集合
    int ret; 
    char Buf[128]; 
    param =NULL;
    strcpy(Buf,"memdev is char dev!"); 
    printf("BUF: %s\n",Buf); 
     
    /*打开设备文件*/ 
    fd = open("/dev/haltoapp",O_RDWR); 
    
    FD_ZERO(&rds);   //清空描述符集合
    FD_SET(fd, &rds); //设置描述符集合
 
    /*清除Buf*/ 
    strcpy(Buf,"Buf is NULL!"); 
    printf("Read BUF1: %s\n",Buf); 
		while (1)
		{
			ret = select(fd + 1, &rds, NULL, NULL, NULL);//调用select（）监控函数
	    if (ret < 0)  
	    {
	        printf("select error!\n");
	        exit(1);
	    }
	    if (FD_ISSET(fd, &rds))   //测试fd1是否可读  
	    {
	    	  lseek(fd,0,SEEK_SET);
	        read(fd, Buf, sizeof(Buf));
	    }
	        
	        
	    printf("Read BUF: %s\n",Buf);  
	    if (!strcmp(Buf,"exit"))
	    	break;
	           
    }            
     
    /*检测结果*/ 
    
    close(fd); 
    printf ("ReadThread exit\n");
    bExit = 1;
    return NULL;
}

int main() 
{ 

    pthread_attr_t thread_attr;
    struct sched_param thread_param;
    pthread_t tid_read;
    
    pthread_attr_init(&thread_attr);
    pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
    thread_param.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&thread_attr, &thread_param); 
	pthread_create(&tid_read, &thread_attr, ReadThread, (void*)NULL);         
    /*初始化Buf*/ 
    
    while(!bExit)
    {
    	usleep (1000*1000);
    	//printf ("campiperead running bExit=%d\n",bExit);
    };
	
    pthread_join(tid_read,NULL);
     
    return 0;     
}