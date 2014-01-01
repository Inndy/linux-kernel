// Linux Device Driver Template/Skeleton
// Kernel Module

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define SD_CONN_MAJOR 241
#define SD_CONN_NAME "sd_conn"

extern atomic_t conn_status;
static unsigned int counter = 0;
static int data;
int ext_status;

#if (HUMAX_MODIFY == y)
int front_port_identified = 0;
int rear_port_identified = 0;

EXPORT_SYMBOL(front_port_identified);
EXPORT_SYMBOL(rear_port_identified);
#endif
DECLARE_WAIT_QUEUE_HEAD(sd_conn_wait);
int no_connection_change = 1;

static int sd_conn_open (struct inode *inode, struct file *file) 
{
	//printk("sd_conn_open()\n");
	return 0;
}
 
static int sd_conn_release (struct inode *inode, struct file *file) 
{
	//printk("sd_conn_release()\n");
	return 0;
}

#if (HUMAX_MODIFY == y)
#define IOCTL_GET_STATUS 9
#endif
#define IOCTL_GET_COUNTER 10
static int sd_conn_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long *arg)
{
	switch(cmd) 
	{
#if (HUMAX_MODIFY == y)
		case IOCTL_GET_STATUS :
			{
				int err;
				no_connection_change = 1;
				ext_status = atomic_read(&conn_status);
				ext_status = (ext_status & 0x3fffffff) | (front_port_identified?0x80000000:0x00000000) | (rear_port_identified?0x40000000:0x00000000);
				err = copy_to_user((void*)arg,&ext_status,sizeof(int));
				if (err != 0)
					return -EFAULT;
				
				counter = 0;
				// printk("conn status front:%d  rear:%d\n",front_port_identified, rear_port_identified);
			}
			break;
#endif
		case IOCTL_GET_COUNTER:
			*arg = (unsigned long)jiffies;
			break;
			
		default:  /* redundant, as cmd was checked against MAXNR */
			return -1;
	}

	return 0;
}


static ssize_t sd_conn_read (struct file *file, char *buf,size_t count, loff_t *ppos) 
{
	int len, err;

	if(no_connection_change == 0)
	{
		no_connection_change = 1;
		ext_status = atomic_read(&conn_status);
		if( count != 4) 
			return 0;
		err = copy_to_user(buf,&ext_status,count);
		if (err != 0)
			return -EFAULT;
		
		len  = count;
		counter = 0;
	}
	else
	{
		interruptible_sleep_on(&sd_conn_wait);
		if(no_connection_change == 0)
		{
			no_connection_change = 1;
			ext_status = atomic_read(&conn_status);
			//printk("sd_conn: ext_status = 0x%x (%d)\n",ext_status,count);
			if( count != 4) 
				return 0;
			err = copy_to_user(buf,&ext_status,count);
			if (err != 0)
				return -EFAULT;
			
			len  = count;
			counter = 0;
		}
		else
		{
			//unexpected condition occured!
			len = -1;
		}
	}

	return len;
}

static ssize_t sd_conn_write (struct file *file, const char *buf,
		size_t count, loff_t *ppos) {
	//printk("sd_conn_write()\n");
	no_connection_change = 0;
	wake_up_interruptible(&sd_conn_wait);					
	return count;
}


struct file_operations sd_conn_fops = {
	.owner	=	THIS_MODULE,
	.llseek	=	NULL,
	.read		=	sd_conn_read,
	.write	=	sd_conn_write,
	.readdir	=	NULL,
	.poll		=	NULL,
	.ioctl	=	sd_conn_ioctl,
	.mmap		=	NULL,
	.open		=	sd_conn_open,
	.flush	=	NULL,
	.release	=	sd_conn_release,
	.fsync	=	NULL,
	.fasync	=	NULL,
	.lock		=	NULL,
	.readv	=	NULL,
	.writev	=	NULL,
};

static int __init sd_conn_init_module (void) {
	int i;
	int ret;
	//printk("initializing module\n");
	
	i = register_chrdev (SD_CONN_MAJOR, SD_CONN_NAME, &sd_conn_fops);
	if (i != 0) return - EIO;
	
	return 0;
}

static void __exit sd_conn_cleanup_module (void) {
	//printk("cleaning up module\n");
	unregister_chrdev (SD_CONN_MAJOR, SD_CONN_NAME);
}

module_init(sd_conn_init_module);
module_exit(sd_conn_cleanup_module);
MODULE_AUTHOR("www.humaxdigital.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux Device Driver for get scsi disk connection status");
