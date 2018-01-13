#ifndef _CHARMEM_H
#define _CHARMEM_H

#include <linux/ioctl.h>

/*ioctl指令*/
#define MEM_MAGIC        'j'                                /*幻数*/
#define MEM_CLEAR        _IO(MEM_MAGIC,0x1a)                /*清理指令*/
#define MEM_SETDATA        _IOW(MEM_MAGIC,0x1b,int)        /*设置指令*/
#define MEM_GETDATA        _IOR(MEM_MAGIC,0x1c,int)        /*读取指令*/
//#define MEM_PRINTSTR    _IOW(MEM_MAGIC,0x1d,char*)        /*字符串*/
#define MEM_MAXNR        0x1c                            /*指令序号最大值*/

#endif