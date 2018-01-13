#ifndef _CHARMEM_H
#define _CHARMEM_H

#include <linux/ioctl.h>

/*ioctlָ��*/
#define MEM_MAGIC        'j'                                /*����*/
#define MEM_CLEAR        _IO(MEM_MAGIC,0x1a)                /*����ָ��*/
#define MEM_SETDATA        _IOW(MEM_MAGIC,0x1b,int)        /*����ָ��*/
#define MEM_GETDATA        _IOR(MEM_MAGIC,0x1c,int)        /*��ȡָ��*/
//#define MEM_PRINTSTR    _IOW(MEM_MAGIC,0x1d,char*)        /*�ַ���*/
#define MEM_MAXNR        0x1c                            /*ָ��������ֵ*/

#endif