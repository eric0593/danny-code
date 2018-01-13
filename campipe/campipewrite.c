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

int main() 
{ 
    int fd; 
    //fd_set rds;    //声明描述符集合
    int ret;
    char Buf[128];
    
    fd = open("/dev/haltoapp",O_RDWR);
    while(strcmp(Buf,"exit"))
    {
    	memset(Buf,0x00,sizeof(Buf));
    	gets(Buf);
    	printf("Input Buf: %s\n",Buf);
    	lseek(fd,0,SEEK_SET);
    	write(fd, Buf, sizeof(Buf)); 
    }
    /*初始化Buf*/  
     
    /*检测结果*/ 
    printf("exit\n");
    
    close(fd); 
     
    return 0;     
}