#define ODMSZ_ANDROID
#define LOG_TAG "odmszproxy"

#include "odmszcommon.h"
#include "odmszproxy.h"
#include <sys/socket.h>
#include <cutils/properties.h>
#include <sys/poll.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/un.h>

#include <cutils/sockets.h>

static int run_cmd(char *cmd,char **result)
{
    FILE *file;
    int Length = 0;
    char line[_LINE_LENGTH]={0};
    char *temp = NULL;
    file = popen(cmd, "r");
    temp = (char *)malloc(_MAX_SENT_BYTES);
    memset(temp,0x00,_MAX_SENT_BYTES);
    if (NULL != file&&temp!=NULL)
    {
        while (fgets(line, _LINE_LENGTH, file) != NULL)
        {
            //ODMSZ_LOG_D("line = %s\n", line);
             // line is end with '\n''\0'
            if (Length+strlen(line)<_MAX_SENT_BYTES)
            {
                //ODMSZ_LOG_D("%d : %s", strlen(line),line);
                strcat(temp+Length,line);
                Length += strlen(line);
            }
        }

        ODMSZ_LOG_D("total result size = %d\n", Length);
    }
    else
    {
        ODMSZ_LOG_D("run command %s failed", cmd);
        free(temp);
        return 1;
    }
    *result = temp;
    pclose(file);
    return 0;
}

static int read_request(int fd) {
    struct ucred cr;
    socklen_t len = sizeof(cr);
    int status = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &len);
    if (status != 0) {
        ODMSZ_LOG_E("cannot get credentials\n");
        return -1;
    }

    ODMSZ_LOG_D("reading tid\n");
    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct pollfd pollfds[1];
    pollfds[0].fd = fd;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    status = poll(pollfds, 1, 3000);
    if (status != 1) {
        ODMSZ_LOG_E("timed out reading tid (from pid=%d uid=%d)\n", cr.pid, cr.uid);
        return -1;
    }

    char msg[256];
    memset(msg, 0, sizeof(msg));
    status = read(fd, msg, sizeof(msg));
    if (status < 0) {
        ODMSZ_LOG_E("read failure? %s (pid=%d uid=%d)\n", strerror(errno), cr.pid, cr.uid);
        return -1;
    }

    ODMSZ_LOG_D("request cmd [%s] from (pid=%d uid=%d)\n", msg, cr.pid, cr.uid);
    char *result;
    if (!run_cmd(msg,&result))
    {
        //ODMSZ_LOG_D("write to client %s\n", result);
        if (write(fd, result, strlen(result) + 1) < 0) {
            int res = errno;
            ODMSZ_LOG_D("write failed errno %d\n",errno);
            free(result);
            return res;
        }
        free(result);
    }
    
    return 0;
}


int main()
{
    //int server_fd = socket_local_server("odmsz", ANDROID_SOCKET_NAMESPACE_RESERVED,SOCK_STREAM ); 
    int server_fd = android_get_control_socket("odmsz");                                     
    if (server_fd == -1) 
    {
        ODMSZ_LOG_D("odmszproxy socket connect failed\n");
        return 1;
    }
    ODMSZ_LOG_D("odmszproxy socket server fd %d\n",server_fd);
    listen(server_fd,4);
    for (;;) {
        ODMSZ_LOG_D("waiting for connection\n");
        int fd = accept(server_fd, NULL, NULL);
        if (fd == -1) {
          ODMSZ_LOG_D("accept failed: %s\n", strerror(errno));
          continue;
        }
        read_request(fd);
    }


    return 0;
}

