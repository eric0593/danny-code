//#define ODMSZ_ANDROID
#define LOG_TAG "odmsz"

#include "odmszcommon.h"
#include "odmszproxy.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>

static void usage(char *progname);
static int do_monitor(int sock);
static int do_cmd(int sock, int argc, char **argv);

int main(int argc, char **argv) {
    int sock;
    int cmdOffset = 0;

    if (argc < 2)
        usage(argv[0]);

    // try interpreting the first arg as the socket name - if it fails go back to netd

    if ((sock = socket_local_client("odmsz",
                                     ANDROID_SOCKET_NAMESPACE_RESERVED,
                                     SOCK_STREAM)) < 0) {
        ODMSZ_LOG_E("Error connecting (%s)\n", strerror(errno));
        exit(4);
    } else {
        if (argc < 2) usage(argv[0]);
    }
    exit(do_cmd(sock, argc-cmdOffset, &(argv[cmdOffset])));
}

static int do_cmd(int sock, int argc, char **argv) {
    char * final_cmd = strdup("");
    int i;

    for (i = 1; i < argc; i++) {
  
        bool needs_quoting = strchr(argv[i], ' ');
        const char *format = needs_quoting ? "%s\"%s\"%s" : "%s%s%s";
        char *tmp_final_cmd;
        if (asprintf(&tmp_final_cmd, format, final_cmd, argv[i],
                     (i == (argc - 1)) ? "" : " ") < 0) {
            int res = errno;
            ODMSZ_LOG_E("failed asprintf");
            free(final_cmd);
            return res;
        }
        free(final_cmd);
        final_cmd = tmp_final_cmd;
    }
    
    //ODMSZ_LOG_D("-------->>>>>>>>\n %s\n",final_cmd);
    if (write(sock, final_cmd, strlen(final_cmd) + 1) < 0) {
        int res = errno;
        ODMSZ_LOG_E("write failed %d\n ",res);
        free(final_cmd);
        return res;
    }
    free(final_cmd);

    return do_monitor(sock);
}

static int do_monitor(int sock) {
    char *buffer = malloc(4096);
    
    struct epoll_event ev;
    struct epoll_event events[20];
    int epfd=epoll_create(256);
    
    ev.data.fd=sock;
    ev.events=EPOLLET|EPOLLOUT;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sock,&ev);
    for(;;)
    {
        int rc = 0;
        int nfds=epoll_wait(epfd,events,20,500);
        for(int i=0;i<nfds;++i)
        {   
            if(events[i].events&EPOLLOUT) 
            {
               if ((rc = read(sock, buffer, 4096)) <= 0) {
                    int res = errno;
                    if (rc == 0)
                        ODMSZ_LOG_E("Lost connection to odmsz - did it crash?\n");
                    else
                        ODMSZ_LOG_E("Error reading data (%s)\n", strerror(errno));
                    free(buffer);
                    if (rc == 0)
                        return ECONNRESET;
                    return res;
                }

                //ODMSZ_LOG_D("<<<<<<<<--------\n",buffer);
                ODMSZ_LOG_D("%s\n",buffer);
                goto finish;
            }  
    
            else if(events[i].events&EPOLLIN)
            {
                ODMSZ_LOG_D("EPOLLIN\n");
            }
    
        } 
    }

finish:    
    free(buffer);
    return 0;
}

static void usage(char *progname) {
    ODMSZ_LOG_D("%s\n",progname);
    exit(1);
}



