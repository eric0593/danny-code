#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _GNU_SOURCE
// To get ptsname grandpt and unlockpt definitions from stdlib.h
#define _GNU_SOURCE
#endif
#include <features.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cutils/properties.h>

//syslog
//#include <syslog.h>
#include <utils/Log.h>

#ifndef MC800
#define MC800 1
#endif

// basic mode flag for frame start and end
#define F_FLAG 0xF9

// bits: Poll/final, Command/Response, Extension

#define PF 0x0 //0
#define CR 0x02 //2
#define EA 0x01 //1
// the types of the frames
#define SABM 0x2f //47
#define UA 0x63 //99
#define DM 0x0F //15
#define DISC 0x43 //67
#define UIH 0xFF 
#define UI 0x03 //3
#define C_CLD 255
#define C_TEST 33
#define C_MSC 225
#define C_NSC 17

#define S_FC 2
#define S_RTC 4
#define S_RTR 8
#define S_IC 64
#define S_DV 128

#define COMMAND_IS(command, type) ((type & ~CR) == command)
#define PF_ISSET(frame) ((frame->control & PF) == PF)
#define FRAME_IS(type, frame) ((frame->control & ~PF) == type)


static int baudrates[] = { 
    0, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };

static speed_t baud_bits[] = {
    0, B9600, B19200, B38400, B57600, B115200, B230400, B460800,B921600 };

static int baudrate = 115200;
static int _debug = 1;
static int serial_fd;
pthread_mutex_t syslogdump_lock;
#define LOG_TAG "ideatool"

extern void reboot3gModem(void);

void IF_LOGD(const char *fmt, ...)
{
    if (_debug)
    {
        va_list ap;
        char buf[1024];    
        va_start(ap, fmt);
        vsnprintf(buf, 1024, fmt, ap);
        va_end(ap);
        ALOGD("%s",buf);
    }
}

int indexOfBaud(int baudrate)
{
    int i;

    for (i = 0; i < sizeof(baudrates) / sizeof(baudrates[0]); ++i) {
        if (baudrates[i] == baudrate)
            return i;
    }
    return 0;
}

static inline int tcdrain_fd(int fd) 
{
    return ioctl(fd, TCSBRK, 1);
}

void setAdvancedOptions(int fd, speed_t baud) {
    struct termios options;
    struct termios options_cpy;

    fcntl(fd, F_SETFL, 0);
    
    // get the parameters
    tcgetattr(fd, &options);
    
    // Do like minicom: set 0 in speed options
    cfsetispeed(&options, 0);
    cfsetospeed(&options, 0);
    
    options.c_iflag = IGNBRK;
    
    // Enable the receiver and set local mode and 8N1
    options.c_cflag = (CLOCAL | CREAD | CS8 | HUPCL);
    // enable hardware flow control (CNEW_RTCCTS)
    //options.c_cflag |= CRTSCTS;
    // Set speed
    options.c_cflag |= baud;
        
    // set raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    
    // set raw output
    options.c_oflag &= ~OPOST;
    options.c_oflag &= ~OLCUC;
    options.c_oflag &= ~ONLRET;
    options.c_oflag &= ~ONOCR;
    options.c_oflag &= ~OCRNL;
    
    // Set the new options for the port...
    options_cpy = options;
    tcsetattr(fd, TCSANOW, &options);
    options = options_cpy;
    
    // Do like minicom: set speed to 0 and back
    options.c_cflag &= ~baud;
    tcsetattr(fd, TCSANOW, &options);
    options = options_cpy;
    
    sleep(1);
    
    options.c_cflag |= baud;
    tcsetattr(fd, TCSANOW, &options);
}

int findInBuf(char* buf, int len, char* needle) {
    IF_LOGD("### Enter (%s) len=%d needle=%s", __func__,len,needle);

    int i;
    int needleMatchedPos=0;
    if (needle[0] == '\0') {
        return 1;
    }

    for (i=0;i<len;i++){
        if (needle[needleMatchedPos] == buf[i]) {
            needleMatchedPos++;
            if (needle[needleMatchedPos] == '\0') {
                // Entire needle was found
                return 1; 
            }      
        }else {
            needleMatchedPos=0;
        }
    }
    
    return 0;
}

static int syslogdump(
    const char *prefix,
    const unsigned char *ptr,
    unsigned int length)
{
    IF_LOGD( "Enter ::: %s\n" , __FUNCTION__);
    char buffer[100];
    unsigned int offset = 0l;
    int i;

    if(!_debug)
        return 0;
    
    pthread_mutex_lock(&syslogdump_lock);   //new lock
    while (offset < length)
    {
        int off;
        strcpy(buffer, prefix);
        off = strlen(buffer);
        snprintf(buffer + off, sizeof(buffer) - off, "%08x: ", offset);
        off = strlen(buffer);
        for (i = 0; i < 16; i++)
        {
            if (offset + i < length){
                snprintf(buffer + off, sizeof(buffer) - off, "%02x%c", ptr[offset + i], i == 7 ? '-' : ' ');
                }
            else{
                snprintf(buffer + off, sizeof(buffer) - off, " .%c", i == 7 ? '-' : ' ');
                }
            off = strlen(buffer);
        }
        snprintf(buffer + off, sizeof(buffer) - off, " ");
        off = strlen(buffer);
        for (i = 0; i < 16; i++)
            if (offset + i < length)
            {
                snprintf(buffer + off, sizeof(buffer) - off, "%c", (ptr[offset + i] < ' ') ? '.' : ptr[offset + i]);
                off = strlen(buffer);
            }
        offset += 16;
        IF_LOGD("%s", buffer);
    }
    pthread_mutex_unlock(&syslogdump_lock);/*new lock*/

    return 0;
}


int open_serialport(char *dev)
{
    IF_LOGD( "Enter ::: %s\n" , __FUNCTION__);

    int fd;
    
    fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd != -1)
    {
        int index = indexOfBaud(baudrate);

        IF_LOGD("serial opened  index = %d\n",index );
        if (index > 0) {
            // Switch the baud rate to zero and back up to wake up 
            // the modem
            //setAdvancedOptions(fd, baud_bits[index]);
            setAdvancedOptions(fd, baud_bits[5]);
        } else {
            struct termios options;
            // The old way. Let's not change baud settings
            fcntl(fd, F_SETFL, 0);
            
            // get the parameters
            tcgetattr(fd, &options);                       
        
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG);
        //      ios.c_lflag = 0; /* disable ECHO, ICANON, etc... */
        options.c_oflag &= (~(ONLCR)); /* Stop \n -> \r\n translation on output */
        options.c_iflag &= (~(ICRNL | INLCR)); /* Stop \r -> \n & \n -> \r translation on input */
        options.c_iflag |=  IXOFF; /* Ignore \r & XON/XOFF on input */
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;

        options.c_iflag &=~(INLCR|IGNCR|ICRNL);
        options.c_oflag &=~(ONLCR|OCRNL);   
        cfsetispeed(&options, B115200);
        // Set the new options for the port...
        tcsetattr(fd, TCSANOW, &options);
        }
    }
    return fd;
}

const unsigned char r_crctable[256] = { //reversed, 8-bit, poly=0x07 
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 
    0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B, 
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 
    0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67, 
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 
    0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43, 
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 
    0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F, 
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 
    0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B, 
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 
    0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17, 
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 
    0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33, 
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 
    0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F, 
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 
    0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B, 
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 
    0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87, 
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 
    0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3, 
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 
    0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF, 
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 
    0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB, 
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 
    0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7, 
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 
    0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3, 
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 
    0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF };

unsigned char make_fcs(const unsigned char *input, int count) {
    unsigned char fcs = 0xFF;
    int i;
    for (i = 0; i < count; i++) {
        fcs = r_crctable[fcs^input[i]];
    }
    return (0xFF-fcs);
}

int write_frame(int channel, const char *input, int count, unsigned char type)
{
    IF_LOGD( "Enter ::: %s\n" , __FUNCTION__);
    //mc800 
    // AT\r = F9 0B FF 07 41 54 0D 41 F9
    
    // flag, EA=1 C channel, frame type, length 1-2
    unsigned char prefix[5] = { F_FLAG, EA | CR, 0, 0, 0};
    unsigned char postfix[2] = { 0xFF, F_FLAG };
    int prefix_length = 4;
    int c = 0,i=0;
    int hh=0;
    char frame[4096]={0};

    IF_LOGD("send frame to ch: %d count:=%d input = %s\n", channel,count,input); 
    //channel=channel-1;
    // EA=1, Command, let's add address 
    prefix[1] = prefix[1] | ((63 & (unsigned char) channel) << 2); 

    // let's set control field 
    prefix[2] = type; 

    // let's not use too big frames 
    //count = min(max_frame_size, count); 

    // length 
    if (count > 127) 
    {
        prefix_length = 5; 
        prefix[3] = ((0x007F & count) << 1); 
        prefix[4] = (0x7F80 & count) >> 7; 
    }
    else 
    {
        prefix[3] = 1 | (count << 1); 
    }
    for (i=0;i<prefix_length;i++)
        IF_LOGD("prefix=0x%2x,",prefix[i]);
    for (i=0;i<count;i++)
        IF_LOGD("input=0x%2x,",input[i]);
    // CRC checksum 
    postfix[0] = make_fcs(prefix + 1, prefix_length - 1); 
    IF_LOGD("postfix=0x%2x,",postfix[0]);
    IF_LOGD("postfix=0x%2x",postfix[1]);

    c = write(serial_fd, prefix, prefix_length); 
    if (c != prefix_length) 
    {
        IF_LOGD("Couldn't write the whole prefix to the serial port for the virtual port %d. Wrote only %d  bytes.", channel, c); 
        return 0; 
    }
    if (count > 0) 
    {
        c = write(serial_fd, input, count); 
        if (count != c) 
        {
            IF_LOGD("Couldn't write all data to the serial port from the virtual port %d. Wrote only %d bytes.\n", channel, c); 
            return 0; 
        }
    }
    c = write(serial_fd, postfix, 2); 
    if (c != 2) 
    {

        IF_LOGD("Couldn't write the whole postfix to the serial port for the virtual port %d. Wrote only %d bytes.", channel, c); 
        return 0; 
    }    

    return count; 
}




int at_command(int fd, char *cmd, int to)
{

    //IF_LOGD( "Enter ::: %s\n" , __FUNCTION__);

    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[1024];
    int sel, len, i;
    int returnCode = 0;
    int wrote = 0;

    //IF_LOGD("is in %s\n", __FUNCTION__); 

    wrote = write(fd, cmd, strlen(cmd)); 
    IF_LOGD("### wrote command %s %d \n", cmd, wrote);

    tcdrain_fd(fd);
    sleep(1);

    for (i = 0; i < 100; i++)
    {

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = to;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0)
        {

            if (FD_ISSET(fd, &rfds))
            {
                memset(buf, 0, sizeof(buf));
                len = read(fd, buf, sizeof(buf));
               
                syslogdump("read frame to ch:",buf,len);
                if(strstr(cmd, "AT^MBOOT?\r\n") != NULL)
                {
                    //IF_LOGD("---------- at^mboot -----------\n");
                    if (findInBuf(buf, len, "1"))
                        returnCode = 1;
                    if (findInBuf(buf, len, "ERROR"))
                        returnCode = -1;
                    break;
                }
                else
                {
                    //IF_LOGD("---------- not at^mboot ----------\n");
                    if (findInBuf(buf, len, "OK"))
                    {
                        returnCode = 1;
                        break;
                    }
                }
                if (findInBuf(buf, len, "ERROR"))
                    break;
            }

        }

    }

    return returnCode;
}

int talkto6270 (void)
{
    int ret = 0,i=0;
    char poff_cmd[]= "AT^KEYPOWER=1\r\n";
    char poff_cmd_mux[]={0xf9,0x07,0xff,0x1f,0x41,0x54,0x5e,0x4b,0x45,0x59,0x50,0x4f,0x57,0x45,0x52,0x3d,0x31,0x0d,0x0a,0xd4,0xf9};
    unsigned char at[10]={0xF9,0x07,0xFF,0x07,0x41,0x54,0x0D,0xC6,0xF9};
    int reboot_try = 0;
    
    serial_fd = open_serialport("/dev/ttyHSL0");
    if (serial_fd< 0)
    {   
        IF_LOGD("open_serialport /dev/ttyHSL0 failed");
        ret= -1;
        return ret;
    }    
    while (1)   
    {
        for (i=0;i<3;i++)
        {            
            if(at_command(serial_fd,poff_cmd,10000))
            {            
                IF_LOGD("AT command %s success",poff_cmd);
                ret=0;   
                break;
            }

            if(at_command(serial_fd,poff_cmd_mux,10000))
            {            
                IF_LOGD("AT command poff_cmd_mux success");
                ret=0;   
                break;
            }
    
            usleep(500*1000);
        }
        
        IF_LOGD("talkto6270 have try times=%d",i);
        if (i<3)
        {
            break;
        }      
        if (reboot_try<10)
        {
            reboot3gModem();
            poweron3gModem();
            reboot_try++;                   
        }
        else
        {     
            property_set("persist.sys.utmsphone.shutdown", "abnormal");
            return -1;
        }
        //poweron3gModem();
        usleep(15*1000*1000);
    } 
    close(serial_fd);
    return ret;
}


