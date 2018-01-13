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

static int major=0;        /*�豸�����豸��*/
static bool have_data = false; /*�����豸���㹻���ݿɹ���*/
static struct mutex mutex;

#ifndef CHARDEV_MAJOR
#define CHARDEV_MAJOR 0   /*Ԥ���mem�����豸��*/
#endif

#ifndef CHARDEV_NR_DEVS
#define CHARDEV_NR_DEVS 1    /*�豸��*/
#endif

#ifndef CHARDEV_SIZE
#define CHARDEV_SIZE (128)
#endif


module_param(major,int,S_IRUGO);    /*�����豸����Ϊ�������뵽ģ��*/

/*�Զ�����ַ������洢�ṹ��*/
struct chardev_dev
{
	char *data; 					 
	unsigned long size; 
	wait_queue_head_t inq; 
};

static struct chardev_dev *p_chardev;        /*�����ýṹ��*/
static struct cdev cdev;					 /*�ַ��豸�����ṹ��(�ں�)*/	
static struct class *pclass;


/*�ַ������򿪺���*/
static int chardev_open(struct inode *inode,struct file *filp)
{
	struct chardev_dev *dev;
    
    /*��ȡ���豸��*/
    int num = MINOR(inode->i_rdev);

    if (num >= CHARDEV_NR_DEVS) 
            return -ENODEV;
    dev = &p_chardev[num];
    
    /*���豸�����ṹָ�븳ֵ���ļ�˽������ָ��*/
    filp->private_data = dev;
    //have_data = false;
    return 0; 
}


/*�ַ������ͷź���*/
static int chardev_release(struct inode *inode,struct file *filp)
{
    return 0;
}


/*�ַ������ض�ָ���*/
static long chardev_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
    //struct struct_chardev_dev *dev=filp->private_data;
    int result=0,ioarg=0;
    //char * chararg;
    /*���ָ���Ƿ�Ϸ�*/
    if(_IOC_TYPE(cmd)!=MEM_MAGIC)   /*���ָ�����Ͳ�����Ԥ����Ļ���*/
        return -EINVAL;

    if(_IOC_NR(cmd)>MEM_MAXNR)        /*ָ��������ֵ*/
        return -EINVAL;

    /*���Ȩ��*/
    if(_IOC_DIR(cmd) & _IOC_READ)
        result=!access_ok(VERIFY_WRITE,(void *)arg,_IOC_SIZE(cmd));
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
        result=!access_ok(VERIFY_READ,(void *)arg,_IOC_SIZE(cmd));
    
    if(result)
        return -EFAULT;


    /*ִ��ָ��*/
    switch(cmd)
    {
        case MEM_CLEAR:        /*��������*/
            printk(KERN_INFO "Kernel Execute 'MEM_CLEAR' Command!\n");
            break;
        case MEM_GETDATA:    /*��ȡ����ioarg-->arg*/
            ioarg=2016;
            result=__put_user(ioarg,(int *)arg);
            break;
        case MEM_SETDATA:    /*���ò���arg-->ioarg*/
            result=__get_user(ioarg,(int *)arg);
            printk(KERN_INFO "Kernel Execute 'MEM_SETDATA' Command,ioarg=%d!\n",ioarg);
            break;
//        case MEM_PRINTSTR:
//            result=__get_user(chararg,(char *)arg1);
//            printk(KERN_INFO "Kernel Execute 'MEM_PRINTSTR' Command,chararg=%s\n",chararg);
//            break;
        default:
            return -EINVAL;        /*���ش���*/
    }

    return 0;
}


/*�ַ�������ȡ����
struct file *filp:    �ļ�ָ��
char __user *buf:    �û��ռ��ڴ�
size_t size:        ��ȡ������
loff_t *poss:        �ļ���ָ��ƫ����
*/
static ssize_t chardev_read(struct file *filp,char __user *buf,size_t size,loff_t *ppos)
{
  unsigned long p =  *ppos;
  unsigned int count = size;
  int ret = 0;
    struct chardev_dev *dev=filp->private_data;    /*��ȡ������filp�е��ַ���������*/
    
	//mutex_lock(&mutex);
    if(p>=CHARDEV_SIZE)    /*�����ǰָ������ļ��ڴ���*/
        return 0;
    if(count>CHARDEV_SIZE-p)        /*Ҫ��ȡ����������ʣ�������*/
        count=CHARDEV_SIZE-p;
	while (!have_data) /* û�����ݿɶ�������Ϊʲô����if������while */
	{
	    if (filp->f_flags & O_NONBLOCK)
	        return -EAGAIN;

		wait_event_interruptible(dev->inq,have_data);
	}

	printk("%s read %zd bytes(s) from %ld\n", CDEV_NAME, size, p);

	/*�����ݵ��û��ռ�*/
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

	have_data = false; /* �������������ݿɶ� */
	/* ����д���� */

	//mutex_unlock(&mutex);
	return ret;
}


/*�ַ�����д�뺯��*/
static ssize_t chardev_write(struct file *filp,const char __user *buf,size_t size,loff_t *ppos)
{
    unsigned long p=*ppos;
    unsigned int count=size;
    int ret = 0;

    struct chardev_dev *dev=filp->private_data;    /*��ȡ������filp�е��ַ���������*/
	//mutex_lock(&mutex);
    if(p>=CHARDEV_SIZE)            /*���Ҫд��Ĵ�С������Ԥ����ڴ��С*/
        return 0;

    if(count>CHARDEV_SIZE-p)    /*���Ҫд��Ĵ�С������ʣ����ڴ��С,��ɾ��ʣ�������*/
        count=CHARDEV_SIZE-p;

  printk("%s written %zd bytes(s) from %ld\n", CDEV_NAME,size, p);

  /*���û��ռ�д������*/
  if (copy_from_user(dev->data + p, buf, count))
    ret =  - EFAULT;
  else
  {
    *ppos += count;
    ret = count;
    
    printk("%s written %d bytes(s) from %ld\n",CDEV_NAME, count, p);
  }
  
  have_data = true; /* ���µ����ݿɶ� */
    
    /* ���Ѷ����� */
  wake_up(&(dev->inq));
  //mutex_unlock(&mutex);

  return ret;
}


/*�ļ���ָ���ƶ�����*/
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
    
   /*���ȴ�������ӵ�poll_table���� */
    	poll_wait(filp, &dev->inq,  wait);
     
    if (have_data)
        mask |= POLLIN | POLLRDNORM;  /* readable */


    return mask;
}
/*�ļ�������*/
static const struct file_operations chardev_fops=
{
    .owner        = THIS_MODULE,    /*ָ��ӵ�����ģ���ָ��*/
    .llseek        = chardev_llseek,        /*�ļ���ָ��ƫ�Ʋ���*/
    .read        = chardev_read,        /*�ַ��豸��ȡ*/
    .write        = chardev_write,    /*�ַ��豸д��*/
    .open        = chardev_open,        /*�ַ��豸��*/
    .release    = chardev_release,    /*�ַ��豸�ͷ�*/
    .unlocked_ioctl    = chardev_ioctl,    /*���ַ��豸�����ض�ָ��*/
    .poll = chardev_poll,

};







/*ģ���ʼ��*/
static int __init chardev_init(void)
{
	int result;
	int i;


	dev_t devno=MKDEV(major,0);
	printk ("chardev_init enter\n");
	
	mutex_init(&mutex);

	if(major)    /*������豸���Ѿ�Ԥ������*/
	{
	    result=register_chrdev_region(devno,CHARDEV_NR_DEVS,CDEV_NAME);    /*��̬�����豸��*/
	}
	else
	{
	    result=alloc_chrdev_region(&devno,0,CHARDEV_NR_DEVS,CDEV_NAME);    /*��̬�����豸��*/
	    major=MAJOR(devno);        /*��ȡ���䵽�����豸��*/
	}

	if(result<0)
	{
		printk ("register_chrdev_region or  alloc_chrdev_region failed\n");
	    return result;
	}
	
	printk ("chardev_init major=%d\n",major);

  /*��ʼ��cdev�ṹ*/
	cdev_init(&cdev, &chardev_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &chardev_fops;
  
  /* ע���ַ��豸 */
	cdev_add(&cdev, MKDEV(major, 0), CHARDEV_NR_DEVS);
  
    /*Ϊ���������ں������ڴ�,��СΪ4K*/
    p_chardev=kzalloc(CHARDEV_NR_DEVS*sizeof(struct chardev_dev),GFP_KERNEL);
    if(!p_chardev)
    {
        result=-ENOMEM;
        goto fail_malloc;
    }

	memset(p_chardev, 0, sizeof(struct chardev_dev));
	 
	 /*Ϊ�豸�����ڴ�*/
	 for (i=0; i < CHARDEV_NR_DEVS; i++) 
	 {
		   p_chardev[i].size = CHARDEV_SIZE;
		   p_chardev[i].data = kmalloc(CHARDEV_SIZE, GFP_KERNEL);
		   memset(p_chardev[i].data, 0, CHARDEV_SIZE);
	 
		 /*��ʼ���ȴ�����*/
		init_waitqueue_head(&(p_chardev[i].inq));
		//init_waitqueue_head(&(mem_devp[i].outq));
	 }


	pclass = class_create(THIS_MODULE, CDEV_NAME);
	device_create(pclass, NULL, devno,  NULL, CDEV_NAME);
	printk ("chardev_init success\n");
    return 0;

    fail_malloc:
    unregister_chrdev_region(devno,1);    /*����֮ǰע����ַ��豸*/
    return result;

}

/*ģ��ע������*/
static void __exit chardev_exit(void)
{
    cdev_del(&cdev);    /*��ϵͳ��ɾ�������ַ��豸*/
	device_destroy(pclass, MKDEV(major, 0));                 
    unregister_chrdev_region(MKDEV(major,0),1);    /*����֮ǰע����ַ��豸*/
	class_destroy(pclass);
	kfree(p_chardev); /*�ͷŷ�����ڴ�*/
}


module_init(chardev_init);
module_exit(chardev_exit);

/*ģ������*/
MODULE_AUTHOR("EDISON REN");
MODULE_LICENSE("GPL");
