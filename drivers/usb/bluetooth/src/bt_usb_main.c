/******************************************************************************

                 (c) Cambridge Silicon Radio Ltd, 2006

                 All rights reserved

FILE:            bt_usb_main.c

DESCRIPTION:     USB character device node driver

REVISION:        $Revision: #6 $

******************************************************************************/

#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <asm/semaphore.h>
#include <linux/list.h>
#include <linux/uio.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <asm/bitops.h>

#undef BT_USB_DEBUG

#include "../inc/linux_usb_ioctl.h"
#include "../inc/sched/sched.h"
#include "../inc/sched/pmalloc.h"
#include "../inc/hci_hc_com.h"
#include "../inc/usb_com.h"
#include "../inc/linux_usb_extra.h"

typedef struct read_q_elm_t
{
    uint8_t channel;
    MessageStructure *msg;
    struct read_q_elm_t *next;
} read_q_elm_t;

typedef struct
{
    wait_queue_head_t read_q;
    read_q_elm_t *first;
    read_q_elm_t *last;
    uint16_t count;
    bool_t opened;
    struct semaphore access_sem;
} bt_usb_instance_t;

#define BT_USB_MAJOR 0

static int bt_usb_major = BT_USB_MAJOR;
static bt_usb_instance_t *inst = NULL;
static atomic_t available_open_count = ATOMIC_INIT(1);

extern csr_usb_settings_t csr_setup;

#ifdef BT_USB_DEBUG
static void dumpData(uint8_t *data, uint16_t len)
{
    char buffer[1000];
    int idx;
    int i;

    idx = 0;

    for(i=0;i<len;i++)
    {
        idx+=sprintf(&buffer[idx],"0x%02x ",data[i]);

        if (((i+1)%16) == 0)
        {
            printk("%s\n",buffer);
            idx=0;
        }
    }

    if (idx)
    {
        printk("%s\n",buffer);
    }
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
static ssize_t bt_usb_readv(struct kiocb *kiocbp, const struct iovec *io,
                            unsigned long count, loff_t f_pos)
#else
static ssize_t bt_usb_readv(struct file *filp, const struct iovec *io ,
                            unsigned long count, loff_t *f_pos)
#endif
{
    unsigned int i;
    ssize_t len;
    ssize_t no;
    read_q_elm_t *ptr;

    for (i = 0, len = 0; i < count; i++)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
        if (!access_ok(VERIFY_WRITE, io[i].iov_base, io[i].iov_len))
#else
        if (verify_area(VERIFY_WRITE, io[i].iov_base, io[i].iov_len))
#endif
        {
            return -EFAULT;
        }

        len += io[i].iov_len;
    }

    no = 0;

    while(no == 0)
    {
        if (down_interruptible(&inst->access_sem))
        {
            return(-ERESTARTSYS);
        }

        while(inst->first == NULL)
        {
#ifdef BT_USB_DEBUG
            printk("<1>Read queue empty,  count = %d\n",
                   inst->count);
#endif
            up(&inst->access_sem);

            if (wait_event_interruptible(inst->read_q,
                                         (inst->first != NULL)))
            {
                return(-ERESTARTSYS);
            }

            if (down_interruptible(&inst->access_sem))
            {
                return(-ERESTARTSYS);
            }
        }

        ptr = inst->first;

        inst->first = ptr->next;

        if (inst->last == ptr)
        {
            inst->last = NULL;
        }
        inst->count--;

        up(&inst->access_sem);

        no = copy_to_user(io[0].iov_base,&ptr->channel,sizeof(uint16_t));
        if(no == 0)
        {
            no = copy_to_user(io[1].iov_base,
                              ptr->msg->buf,
                              ptr->msg->buflen);
        }
        if(no == 0)
        {
#ifdef BT_USB_DEBUG
            printk("Channel=%d\n", ptr->channel);
            dumpData((uint8_t*)ptr->msg->buf,
                     ptr->msg->buflen);
#endif
            no = sizeof(uint16_t) + ptr->msg->buflen;
        }

        pfree(ptr->msg->buf);
        pfree(ptr->msg);
        pfree(ptr);

#ifdef BT_USB_DEBUG
        printk("<1>BT_USB: readv: no = %d\n",no);
#endif
    }

    return(no);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
static ssize_t bt_usb_writev(struct kiocb *kiocbp, const struct iovec *io,
                             unsigned long count,loff_t f_pos)
#else
static ssize_t bt_usb_writev(struct file *filp, const struct iovec *io,
                             unsigned long count,loff_t *f_pos)
#endif
{
    int i;
    uint8_t channel;
    ssize_t len=0;
    unsigned char *buf;
    uint16_t length;

#ifdef BT_USB_DEBUG
    printk("<1>BT_USB: writev, count = %ld\n",count);
#endif

    for (i = 0, len = 0; i < count; i++)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
        if (!access_ok(VERIFY_READ, io[i].iov_base, io[i].iov_len))
#else
        if (verify_area(VERIFY_READ, io[i].iov_base, io[i].iov_len))
#endif
        {
            return -EFAULT;
        }

        len+=io[i].iov_len;
    }

    /* Things can go wrong when getting data from userspace */
    i = copy_from_user(&channel,io[0].iov_base,io[0].iov_len);
    if(i == 0)
    {
        length = io[1].iov_len;
        buf = pmalloc(io[1].iov_len);
        i = copy_from_user(buf,io[1].iov_base,io[1].iov_len);

        if(i == 0)
        {
#ifdef BT_USB_DEBUG
            printk("Channel=%d\n",channel);
            dumpData(buf,length);
#endif
            UsbDrv_Tx(channel,buf,length);
            i = len;
        }
    }

    return i;
}

static int bt_usb_open(struct inode *inode, struct file *filp)
{
    int retval;

#ifdef BT_USB_DEBUG
    printk("<1>BT_USB: Open called\n");
#endif

    if (!atomic_dec_and_test(&available_open_count))
    {
        atomic_inc(&available_open_count);
        retval = -EBUSY;
    }
    else
    {
        down(&inst->access_sem);
        inst->opened = TRUE;
        up(&inst->access_sem);
        retval = 0;
    }

    return(retval);
}

static int bt_usb_release(struct inode *inode, struct file *filp)
{
    read_q_elm_t *ptr;
    read_q_elm_t *next;

#ifdef BT_USB_DEBUG
    printk("<1>BT_USB: Release called\n");
#endif

    down(&inst->access_sem);

    ptr = inst->first;
    while(ptr)
    {
        next = ptr->next;

        pfree(ptr->msg->buf);
        pfree(ptr->msg);
        pfree(ptr);

        ptr=next;
    }

    inst->opened=FALSE;
    up(&inst->access_sem);
    atomic_inc(&available_open_count);

    return 0;
}

static int bt_usb_ioctl(struct inode *inode, struct file *filp,
                        unsigned int cmd, unsigned long arg)

{
    int ret;

#ifdef BT_USB_DEBUG
    printk("<1>BT_USB: IOCTL called\n");
#endif

    /* Check command */
    if((_IOC_TYPE(cmd) != BT_USB_IOC_MAGIC) ||
       (_IOC_NR(cmd) > BT_USB_IOC_MAX_NO))
    {
        return -ENOTTY;
    }

    /* Handle command */
    ret = -ENOTTY;
    switch(cmd)
    {
        case BT_USB_IOC_ENTER_DFU:
            {
                ret = UsbDrv_EnterDFU();
            }

        case BT_USB_IOC_RESET:
            {
                ret = 0;
                UsbDrv_Reset();
            }

        default:
            {
                printk("<1>BT_USB: Unknown IOCTL\n");
                break;
            }
    }

    return ret;
}

static struct file_operations bt_usb_fops =
{
    owner     : THIS_MODULE,
    ioctl     : bt_usb_ioctl,
    open      : bt_usb_open,
    release   : bt_usb_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    aio_read  : bt_usb_readv,
    aio_write : bt_usb_writev,
#else
    writev    : bt_usb_writev,
    readv     : bt_usb_readv,
#endif
};

/* Dummy function not used */
TaskDefinitionType * sched_setup_tasks(uint16_t * numberOfTasks)
{
    *numberOfTasks = 0;
    return(NULL);
}

void hcCom_ReceiveMsg(void *msg, unsigned bcspChannel, unsigned rel)
{
    read_q_elm_t *p;

    p = pmalloc(sizeof(read_q_elm_t));
    p->channel = bcspChannel;
    p->msg = msg;

#ifdef BT_USB_DEBUG
    printk("<1>BT_USB: Received %i bytes for channel %i\n",
           p->msg->buflen, p->channel);
    dumpData((uint8_t*)p->msg->buf, p->msg->buflen);
#endif

    p->next = NULL;

    if (down_interruptible(&inst->access_sem))
    {
        pfree(p->msg->buf);
        pfree(p->msg);
        pfree(p);
    }
    else
    {
        if (inst->opened)
        {
            if (inst->first == NULL)
            {
                inst->count++;
                inst->first = p;
                inst->last = p;
            }
            else
            {
                inst->count++;
                inst->last->next = p;
                inst->last = p;
            }

            wake_up_interruptible(&inst->read_q);
        }
        else
        {
            pfree(p->msg->buf);
            pfree(p->msg);
            pfree(p);
        }

        up(&inst->access_sem);
    }
}

static int __init bt_usb_init(void)
{
    int result;

    printk("<1>BT_USB initialized\n");

    inst = pmalloc(sizeof(bt_usb_instance_t));
    inst->first = NULL;
    inst->last = NULL;
    inst->count = 0;
    inst->opened = FALSE;
    init_waitqueue_head(&inst->read_q);
    sema_init(&inst->access_sem,1);

    result = register_chrdev(bt_usb_major,"bt_usb",&bt_usb_fops);
    if (result < 0)
    {
        printk(KERN_WARNING "BT_USB: can't get major %d\n",bt_usb_major);
        return(result);
    }

    if (bt_usb_major == 0)
    {
        bt_usb_major = result;
    }

    csr_setup.running_with_scheduler = FALSE;
    /* init_sched(); */
    CsrUsbInit();

    if(!UsbDrv_Start(NULL))
    {
        printk("<1>BlueCore transport failed! Exiting...\n");
    }
    else
    {
        printk("<1>BlueCore transport started...\n");
    }

    return(0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#else
#undef CONFIG_MODULE_UNLOAD
#define CONFIG_MODULE_UNLOAD
#endif

#ifdef CONFIG_MODULE_UNLOAD
static void __exit bt_usb_exit(void)
{
    CsrUsbExit();

    unregister_chrdev(bt_usb_major,"bt_usb");

    printk("<1>BlueCore transport stopped.\n");
}
module_exit(bt_usb_exit);
#endif

module_init(bt_usb_init);
MODULE_LICENSE("Proprietary");

