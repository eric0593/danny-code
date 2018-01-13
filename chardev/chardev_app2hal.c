#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "chardev.h"
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/mutex.h>
#define CDEV_NAME "apptohal"

static int major=0;        /*设备的主设备号*/
static bool have_data = false; /*表明设备有足够数据可供读*/
static struct mutex mutex;

#ifndef CHARDEV_MAJOR
#define CHARDEV_MAJOR 0   /*预设的mem的主设备号*/
#endif

#ifndef CHARDEV_NR_DEVS
#define CHARDEV_NR_DEVS 1    /*设备数*/
#endif

#ifndef CHARDEV_SIZE
#define CHARDEV_SIZE (128)
#endif


module_param(major,int,S_IRUGO);    /*将主设备号作为参数传入到模块*/

/*自定义的字符驱动存储结构体*/
struct chardev_dev
{
	char *data; 					 
	unsigned long size; 
	wait_queue_head_t inq; 
};

static struct chardev_dev *p_chardev;        /*程序用结构体*/
static struct cdev cdev;					 /*字符设备驱动结构体(内核)*/	
static struct class *pclass;


/*字符驱动打开函数*/
static int chardev_open(struct inode *inode,struct file *filp)
{
	struct chardev_dev *dev;
    
    /*获取次设备号*/
    int num = MINOR(inode->i_rdev);

    if (num >= CHARDEV_NR_DEVS) 
            return -ENODEV;
    dev = &p_chardev[num];
    
    /*将设备描述结构指针赋值给文件私有数据指针*/
    filp->private_data = dev;
    //have_data = false;
    return 0; 
}


/*字符驱动释放函数*/
static int chardev_release(struct inode *inode,struct file *filp)
{
    return 0;
}


/*字符驱动特定指令函数*/
static long chardev_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
    //struct struct_chardev_dev *dev=filp->private_data;
    int result=0,ioarg=0;
    //char * chararg;
    /*检查指令是否合法*/
    if(_IOC_TYPE(cmd)!=MEM_MAGIC)   /*如果指令类型不等于预定义的幻数*/
        return -EINVAL;

    if(_IOC_NR(cmd)>MEM_MAXNR)        /*指令序号最大值*/
        return -EINVAL;

    /*检查权限*/
    if(_IOC_DIR(cmd) & _IOC_READ)
        result=!access_ok(VERIFY_WRITE,(void *)arg,_IOC_SIZE(cmd));
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
        result=!access_ok(VERIFY_READ,(void *)arg,_IOC_SIZE(cmd));
    
    if(result)
        return -EFAULT;


    /*执行指令*/
    switch(cmd)
    {
        case MEM_CLEAR:        /*清理命令*/
            printk(KERN_INFO "Kernel Execute 'MEM_CLEAR' Command!\n");
            break;
        case MEM_GETDATA:    /*获取数据ioarg-->arg*/
            ioarg=2016;
            result=__put_user(ioarg,(int *)arg);
            break;
        case MEM_SETDATA:    /*设置参数arg-->ioarg*/
            result=__get_user(ioarg,(int *)arg);
            printk(KERN_INFO "Kernel Execute 'MEM_SETDATA' Command,ioarg=%d!\n",ioarg);
            break;
//        case MEM_PRINTSTR:
//            result=__get_user(chararg,(char *)arg1);
//            printk(KERN_INFO "Kernel Execute 'MEM_PRINTSTR' Command,chararg=%s\n",chararg);
//            break;
        default:
            return -EINVAL;        /*返回错误*/
    }

    return 0;
}


/*字符驱动读取函数
struct file *filp:    文件指针
char __user *buf:    用户空间内存
size_t size:        读取的数量
loff_t *poss:        文件内指针偏移量
*/
static ssize_t chardev_read(struct file *filp,char __user *buf,size_t size,loff_t *ppos)
{
  unsigned long p =  *ppos;
  unsigned int count = size;
  int ret = 0;
    struct chardev_dev *dev=filp->private_data;    /*获取保存在filp中的字符驱动数据*/
    
	//mutex_lock(&mutex);
    if(p>=CHARDEV_SIZE)    /*如果当前指针大于文件内存数*/
        return 0;
    if(count>CHARDEV_SIZE-p)        /*要读取的数量大于剩余的数量*/
        count=CHARDEV_SIZE-p;
	while (!have_data) /* 没有数据可读，考虑为什么不用if，而用while */
	{
	    if (filp->f_flags & O_NONBLOCK)
	        return -EAGAIN;

		wait_event_interruptible(dev->inq,have_data);
	}

	printk("%s read %zd bytes(s) from %ld\n", CDEV_NAME, size, p);

	/*读数据到用户空间*/
	if (copy_to_user(buf, (void*)(dev->data + p), count))
	{
		ret =  - EFAULT;
	}
	else
	{
		*ppos += count;
		ret = count;

		printk("%s read %d bytes(s) from %ld\n",CDEV_NAME, count, p);
	}

	have_data = false; /* 表明不再有数据可读 */
	/* 唤醒写进程 */

	//mutex_unlock(&mutex);
	return ret;
}


/*字符驱动写入函数*/
static ssize_t chardev_write(struct file *filp,const char __user *buf,size_t size,loff_t *ppos)
{
    unsigned long p=*ppos;
    unsigned int count=size;
    int ret = 0;

    struct chardev_dev *dev=filp->private_data;    /*获取保存在filp中的字符驱动数据*/
	//mutex_lock(&mutex);
    if(p>=CHARDEV_SIZE)            /*如果要写入的大小超过了预设的内存大小*/
        return 0;

    if(count>CHARDEV_SIZE-p)    /*如果要写入的大小超过了剩余的内存大小,则删掉剩余额数据*/
        count=CHARDEV_SIZE-p;

  printk("%s written %zd bytes(s) from %ld\n", CDEV_NAME,size, p);

  /*从用户空间写入数据*/
  if (copy_from_user(dev->data + p, buf, count))
    ret =  - EFAULT;
  else
  {
    *ppos += count;
    ret = count;
    
    printk("%s written %d bytes(s) from %ld\n",CDEV_NAME, count, p);
  }
  
  have_data = true; /* 有新的数据可读 */
    
    /* 唤醒读进程 */
  wake_up(&(dev->inq));
  //mutex_unlock(&mutex);

  return ret;
}


/*文件内指针移动函数*/
static loff_t chardev_llseek(struct file *filp,loff_t offset,int whence)
{ 
    loff_t newpos;

    switch(whence) {
      case 0: /* SEEK_SET */
        newpos = offset;
        break;

      case 1: /* SEEK_CUR */
        newpos = filp->f_pos + offset;
        break;

      case 2: /* SEEK_END */
        newpos = CHARDEV_SIZE -1 + offset;
        break;

      default: /* can't happen */
        return -EINVAL;
    }
    if ((newpos<0) || (newpos>CHARDEV_SIZE))
        return -EINVAL;
        
    filp->f_pos = newpos;
    return newpos;

}
static unsigned int chardev_poll(struct file *filp, poll_table *wait)
{
    struct chardev_dev  *dev = filp->private_data; 
    unsigned int mask = 0;
    
   /*将等待队列添加到poll_table表中 */
    	poll_wait(filp, &dev->inq,  wait);
     
    if (have_data)
        mask |= POLLIN | POLLRDNORM;  /* readable */


    return mask;
}
/*文件操作集*/
static const struct file_operations chardev_fops=
{
    .owner        = THIS_MODULE,    /*指向拥有这个模块的指针*/
    .llseek        = chardev_llseek,        /*文件内指针偏移操作*/
    .read        = chardev_read,        /*字符设备读取*/
    .write        = chardev_write,    /*字符设备写入*/
    .open        = chardev_open,        /*字符设备打开*/
    .release    = chardev_release,    /*字符设备释放*/
    .unlocked_ioctl    = chardev_ioctl,    /*向字符设备发出特定指令*/
    .poll = chardev_poll,

};







/*模块初始化*/
static int __init chardev_init(void)
{
	int result;
	int i;


	dev_t devno=MKDEV(major,0);
	printk ("chardev_init enter\n");
	
	mutex_init(&mutex);

	if(major)    /*如果主设备号已经预先设置*/
	{
	    result=register_chrdev_region(devno,CHARDEV_NR_DEVS,CDEV_NAME);    /*静态分配设备号*/
	}
	else
	{
	    result=alloc_chrdev_region(&devno,0,CHARDEV_NR_DEVS,CDEV_NAME);    /*动态分配设备号*/
	    major=MAJOR(devno);        /*获取分配到的主设备号*/
	}

	if(result<0)
	{
		printk ("register_chrdev_region or  alloc_chrdev_region failed\n");
	    return result;
	}
	
	printk ("chardev_init major=%d\n",major);

  /*初始化cdev结构*/
	cdev_init(&cdev, &chardev_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &chardev_fops;
  
  /* 注册字符设备 */
	cdev_add(&cdev, MKDEV(major, 0), CHARDEV_NR_DEVS);
  
    /*为驱动分配内核物理内存,大小为4K*/
    p_chardev=kzalloc(CHARDEV_NR_DEVS*sizeof(struct chardev_dev),GFP_KERNEL);
    if(!p_chardev)
    {
        result=-ENOMEM;
        goto fail_malloc;
    }

	memset(p_chardev, 0, sizeof(struct chardev_dev));
	 
	 /*为设备分配内存*/
	 for (i=0; i < CHARDEV_NR_DEVS; i++) 
	 {
		   p_chardev[i].size = CHARDEV_SIZE;
		   p_chardev[i].data = kmalloc(CHARDEV_SIZE, GFP_KERNEL);
		   memset(p_chardev[i].data, 0, CHARDEV_SIZE);
	 
		 /*初始化等待队列*/
		init_waitqueue_head(&(p_chardev[i].inq));
		//init_waitqueue_head(&(mem_devp[i].outq));
	 }


	pclass = class_create(THIS_MODULE, CDEV_NAME);
	device_create(pclass, NULL, devno,  NULL, CDEV_NAME);
	printk ("chardev_init success\n");
    return 0;

    fail_malloc:
    unregister_chrdev_region(devno,1);    /*销毁之前注册的字符设备*/
    return result;

}

/*模块注销函数*/
static void __exit chardev_exit(void)
{
    cdev_del(&cdev);    /*从系统中删除掉该字符设备*/
	device_destroy(pclass, MKDEV(major, 0));                 
    unregister_chrdev_region(MKDEV(major,0),1);    /*销毁之前注册的字符设备*/
	class_destroy(pclass);
	kfree(p_chardev); /*释放分配的内存*/
}


module_init(chardev_init);
module_exit(chardev_exit);

/*模块声明*/
MODULE_AUTHOR("EDISON REN");
MODULE_LICENSE("GPL");
