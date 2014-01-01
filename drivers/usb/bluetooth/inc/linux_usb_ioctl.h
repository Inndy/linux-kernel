#ifndef LINUX_USB_IOCTL_H_
#define LINUX_USB_IOCTL_H_
/*****************************************************

              Cambridge Silicon Radio Ltd. 2006

              All rights reserved

FILE:         linux_usb_ioctl.h

DESCRIPTION:  IOCTL definitions for the Linux kernel USB
              driver.

REVISION:     $Revision: #1 $ 

******************************************************/

#ifdef __linux__

#include <linux/ioctl.h>
#include "sched/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_USB_IOC_MAGIC     'c'
#define BT_USB_IOC_ENTER_DFU _IO(BT_USB_IOC_MAGIC, 0)
#define BT_USB_IOC_RESET     _IO(BT_USB_IOC_MAGIC, 1)
#define BT_USB_IOC_MAX_NO    (2)

#ifdef __cplusplus
}
#endif

#endif

#endif
