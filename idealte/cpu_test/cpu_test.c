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
#include "pthread.h"



#include <sys/capability.h>
#include <linux/prctl.h>
#include <errno.h>
#include <sys/prctl.h>
#include <private/android_filesystem_config.h>



static void* RunThread(void* param)
{
	while (1)
	{
		printf("RunThread %s\n",(char*)param);
		usleep(100);
	};
	return NULL;
}


int main (int argc,char ** argv)
{   
    //switchUser();
    pthread_attr_t thread_attr;
    struct sched_param thread_param;
    pthread_t p1,p2,p3,p4;
    
    pthread_attr_init(&thread_attr);
    pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
    thread_param.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&thread_attr, &thread_param);    
    
    pthread_create(&p1, &thread_attr, RunThread, "p1");
		pthread_create(&p2, &thread_attr, RunThread, "p2");
		pthread_create(&p3, &thread_attr, RunThread, "p3");
		pthread_create(&p4, &thread_attr, RunThread, "p4");
    
		while(1)
		{
			;
		}
}

