/****************************************************************************

               (c) Cambridge Silicon Radio Ltd, 2006

               All rights reserved

FILE:          usb_intf.c

DESCRIPTION:   Linux USB driver interface to the BCHS stack.
               The interface functions are those directly involved in
               sending data to/from BCHS and the low-level USB driver.

REVISION:      $Revision: #3 $

****************************************************************************/

#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif

#include <linux/module.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/usb.h>
#include <asm/bitops.h>

/* Enable debug/verbose debugging */
#undef DEBUG_ENABLE
#undef DEBUG_VERBOSE

#include "../inc/sched/sched.h"
#include "../inc/sched/pmalloc.h"
#include "../inc/hci_hc_com.h"
#include "../inc/linux_usb_com.h"
#include "../inc/linux_usb_extra.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/usb_ch9.h>
#endif

/* Globals (device, settings and driver) */
extern csr_dev_t *csr_dev;
extern csr_usb_settings_t csr_setup;
extern struct usb_driver csr_usb_driver;

static DECLARE_COMPLETION(reader_thread_exit);

/*************************************************************
 * NAME:
 *      readerThread
 *
 * DESCRIPTION:
 *      This function runs as a thread, and is responsible for
 *      calling the bg_interrupt in the Scheduler signalling
 *      that data is ready for being fetched. The thread is signalled
 *      whenever data has arrived from the USB device.
 *
 * RETURNS:
 *      Thread completion code (zero)
 *
 *************************************************************/
int readerThread(void *parm)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    daemonize("readerd");
#else
    daemonize();
#endif

    set_bit(R_THREAD_RUNNING, &(csr_dev->flags));

    /* Do run while the boss says so */
    while (test_bit(R_THREAD_RUNNING, &(csr_dev->flags)))
    {
        /* Wait for background interrupts */
        while((test_bit(R_THREAD_RUNNING, &(csr_dev->flags))) &&
              (atomic_read(&(csr_dev->await_bg_int)) == 0))
        {
            DBG_VERBOSE("Reader thread entering sleep\n");
            if(wait_event_interruptible(csr_dev->queue.wait,
                                        !test_bit(R_THREAD_RUNNING, &(csr_dev->flags)) ||
                                        atomic_read(&(csr_dev->await_bg_int))!=0))
            {
                break;
            }
        }
        DBG_VERBOSE("Reader thread woke up\n");

        /* Fire the background interrupt */
        if(test_bit(R_THREAD_RUNNING, &(csr_dev->flags)))
        {
            if(csr_setup.running_with_scheduler)
            {
                bg_int1();
            }
            else
            {
                UsbDrv_Rx();
            }
            atomic_dec(&(csr_dev->await_bg_int));
        }
    }

    /* Done */
    clear_bit(R_THREAD_RUNNING, &(csr_dev->flags));
    complete_and_exit(&reader_thread_exit, 0);
    return 0;
}

/*************************************************************
 * NAME:
 *      UsbDriverStarted
 *
 * RETURNS:
 *      A bool indicating if the driver is started
 *
 *************************************************************/
bool_t UsbDriverStarted(void)
{
    return TRUE;
}

/*************************************************************
 * NAME:
 *      UsbDeviceProbed
 *
 * RETURNS:
 *      A bool indicating if the driver has been probed
 *
 *************************************************************/
bool_t UsbDeviceProbed(void)
{
    if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*************************************************************
 * NAME:
 *      UsbDriverStopRequested
 *
 * RETURNS:
 *      A bool indicating if the driver has been requested to stop
 *
 *************************************************************/
bool_t UsbDriverStopRequested(void)
{
    return FALSE;
}

/*************************************************************
 * NAME:
 *      ExtraUsbInterfaceReady
 *
 * DESCRIPTION
 *      Signal the driver that the extra interface (extension)
 *      is ready for action
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void ExtraUsbInterfaceReady(void)
{
    set_bit(EXTRA_INTERFACE_READY,&(csr_dev->endpoint_present));
}

/*************************************************************
 * NAME:
 *      SetupUsb
 *
 * DESCRIPTION
 *      Set the extra (extension) interface function pointers.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetupUsb(uint8_t interfaceNumber,
              void (*interfaceFunc)(void),
              probeExtraInterface_t pEI,
              disconnectExtraInterface_t dEI,
              startDrvExtraInterface_t sDEI,
              initExtraInterface_t iEI,
              setupExtraInterface_t sEI,
              exitExtraInterface_t eEI)
{
    printk(PRNPREFIX "Setup to use extra interface\n");

    csr_setup.ext_iface = TRUE;
    sEI(interfaceNumber, interfaceFunc);
    csr_setup.ext_probe = pEI;
    csr_setup.ext_disconnect = dEI;
    csr_setup.ext_start = sDEI;
    csr_setup.ext_init = iEI;
    csr_setup.ext_exit = eEI;
}

/*************************************************************
 * NAME:
 *      SetSupportedUsbDevices
 *
 * DESCRIPTION
 *      Replace the default list of supported USB devices
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetSupportedUsbDevices(struct usb_device_id *devs)
{
    csr_usb_driver.id_table = devs;
}

/*************************************************************
 * NAME:
 *      SetUsbDriverName
 *
 * DESCRIPTION
 *      Rename default USB driver name
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbDriverName(char *name)
{
    csr_usb_driver.name = name;
}

/*************************************************************
 * NAME:
 *      SetUsbMaxIsocOutFrames
 *
 * DESCRIPTION
 *      Set number of concurrent ISOC-tx frames
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbMaxIsocOutFrames(int out)
{
    csr_setup.max_isoc_out_frames = out;
}

/*************************************************************
 * NAME:
 *      SetUsbMaxIsocInFrames
 *
 * DESCRIPTION
 *      Set number of concurrent ISOC-rx frames
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbMaxIsocInFrames(int in)
{
    csr_setup.max_isoc_in_frames = in;
}

/*************************************************************
 * NAME:
 *      SetUsbIsocAltSetting
 *
 * DESCRIPTION
 *      Set alternate setting for ISOC interface
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbIsocAltSetting(int alt)
{
    csr_setup.isoc_alt_setting = alt;
}

/*************************************************************
 * NAME:
 *      CsrUsbInit
 *
 * DESCRIPTION
 *      Initialize (register) USB driver. When this function
 *      is called, the USB driver is registerd in the kernel
 *      and the other settings can be altered anymore
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
int CsrUsbInit(void)
{
    int err = 0;
    if(csr_dev == NULL)
    {


        printk(PRNPREFIX "Initialization\n");

        /* Allocate memory for our device and initialize it */
        csr_dev = pmalloc(sizeof(csr_dev_t));
        if(csr_dev == NULL)
        {
            return -ENOMEM;
        }
        memset(csr_dev, 0, sizeof(csr_dev_t));

        /* Initialize pending queue */
        QueuePrepare();

        csr_dev->isoc_reassembly = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        csr_dev->intr_reassembly = NULL;
#endif

        /* Initialize the flags */
        clear_bit(LISTEN_STARTED, &(csr_dev->flags));
        clear_bit(DEVICE_CONNECTED, &(csr_dev->flags));
        clear_bit(R_THREAD_RUNNING, &(csr_dev->flags));
        clear_bit(BULK_IN_READY, &(csr_dev->endpoint_present));
        clear_bit(BULK_OUT_READY, &(csr_dev->endpoint_present));
        clear_bit(INTR_IN_READY, &(csr_dev->endpoint_present));
        clear_bit(ISOC_IN_READY, &(csr_dev->endpoint_present));
        clear_bit(ISOC_OUT_READY, &(csr_dev->endpoint_present));
        clear_bit(EXTRA_INTERFACE_READY, &(csr_dev->endpoint_present));
        atomic_set(&(csr_dev->await_bg_int), 0);

        if(csr_setup.ext_iface == TRUE)
        {
            csr_setup.ext_init();
        }

        /* Set values for the control request header stuff */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
        csr_dev->ctrl_setup.bRequestType = BT_CTRL_REQUEST;
        csr_dev->ctrl_setup.bRequest = 0;
        csr_dev->ctrl_setup.wValue = 0;
        csr_dev->ctrl_setup.wIndex = 0;
#else
        csr_dev->ctrl_setup.requesttype = BT_CTRL_REQUEST;
        csr_dev->ctrl_setup.request = 0;
        csr_dev->ctrl_setup.value = 0;
        csr_dev->ctrl_setup.index = 0;
#endif

        /* The control endpoint is mandatory for all usb devices,
           and can therefore be setup statically */
        csr_dev->ctrl_ep = USB_ENDPOINT_XFER_CONTROL;

        /* Start the reader thread */
        kernel_thread(readerThread, NULL, 0);

        err = usb_register(&csr_usb_driver);
        if(err < 0)
        {
            printk(PRNPREFIX "Failed to register CSR USB driver, code %d\n",err);
        }
        else
        {
            printk(PRNPREFIX "CSR USB driver registered\n");
        }
    }
    else
    {
        printk(PRNPREFIX "CSR USB driver already registered\n");
    }
    return err;
}

/*************************************************************
 * NAME:
 *      CsrUsbExit
 *
 * DESCRIPTION
 *      Shutdown the USB driver and unregister it from the kernel
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void CsrUsbExit(void)
{
    wait_queue_head_t settle_wait;
    printk(PRNPREFIX "Shutting down USB driver\n");

    /* Set the flag indicating that the module must be stopped */
    clear_bit(DEVICE_CONNECTED, &(csr_dev->flags));

    usb_deregister(&csr_usb_driver);

    if(test_bit(R_THREAD_RUNNING, &(csr_dev->flags)))
    {
        DBG_PRINT("Stopping the reader thread\n");
        clear_bit(R_THREAD_RUNNING, &(csr_dev->flags));
        wake_up_interruptible(&(csr_dev->queue.wait));

        wait_for_completion(&reader_thread_exit);
        DBG_PRINT("The reader thread has been stopped\n");
    }
    else
    {
        DBG_PRINT("Reader thread was not running\n");
    }

    register_bg_int(1, NULL);
    init_waitqueue_head(&settle_wait);

    /* Let thing settle, before freeing memory */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(2*HZ);
#else
    sleep_on_timeout(&settle_wait,HZ*2);
#endif

    if(csr_setup.ext_iface == TRUE)
    {
        csr_setup.ext_exit();
    }

    /* Make sure everything has been freed */
    QueueFree();
    pfree(csr_dev);
    csr_dev = NULL;
}

/*************************************************************
 * NAME:
 *      UsbDrv_Stop
 *
 * DESCRIPTION
 *      Stop USB device driver. This is not supported!
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void UsbDrv_Stop(void)
{
    return;
}

/*************************************************************
 * NAME:
 *      UsbDrv_Reset
 *
 * DESCRIPTION
 *      Perform an USB bus reset of the device
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void UsbDrv_Reset(void)
{
    printk(PRNPREFIX "Resetting USB driver...\n");
    usb_reset_device(csr_dev->dev);
}

/*************************************************************
 * NAME:
 *      UsbDrv_Rx
 *
 * DESCRIPTION:
 *      This function is called by a background interrupt each
 *      time a packet (ACL, SCO or HCI) is received from the
 *      USB device. The function pass the data to the higher
 *      layers.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void UsbDrv_Rx(void)
{
    char *tmpbuf;
    struct usb_qe *qe;

    if((atomic_read(&(csr_dev->queue.count)) > 0 ) &&
       test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
    {
        do
        {
            qe = QueuePop();
            if(qe != NULL)
            {
                DBG_VERBOSE("Sent %i bytes to stack\n",
                            qe->msg->buflen);

                /* Send to stack, which consumes it */
                tmpbuf = qe->msg->buf;
                hcCom_ReceiveMsg(qe->msg,
                                 qe->chan,
                                 0);

                /* The SCO data itself must be freed */
                if((csr_setup.running_with_scheduler) &&
                   (qe->chan == BCSP_CHANNEL_SCO))
                {
                    pfree(tmpbuf);
                }

                /* Free the queue-element */
                pfree(qe);
            }
        }
        while(qe != NULL);
    }
}

/*************************************************************
 * NAME:
 *      UsbDrv_Tx
 *
 * DESCRIPTION:
 *      Interface to the above protocol stack, when data is to
 *      be transmitted over the USB interface. Both ACL, SCO
 *      and HCI commands are supported. The type of data is
 *      specified by the channel parameter. A pointer to the
 *      data must be specified in the data parameter. Finally,
 *      the size parameter specifies the size of the data.
 *
 * RETURNS:
 *      TRUE if the data can be buffered for transmission,
 *      otherwise FALSE
 *************************************************************/
bool_t UsbDrv_Tx(uint8_t channel, uint8_t *data, uint16_t size)
{
    bool_t res;

    DBG_VERBOSE("Have received %i bytes from BCHS\n",size);

    res = FALSE;
    if((size > 0) &&
       test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
    {
        if(channel == BCSP_CHANNEL_HCI)
        {
            usbTxCtrl(data, size);
            res = TRUE;
        }
        else if(channel == BCSP_CHANNEL_ACL)
        {
            usbTxBulk(data, size);
            res = TRUE;
        }
        else if(channel == BCSP_CHANNEL_SCO)
        {
            usbTxIsoc(data, size);
            res = TRUE;
        }
    }

    return res;
}

/*************************************************************
 * NAME:
 *      UsbDrv_Start
 *
 * DESCRIPTION:
 *      This function is called to initialize this module. The
 *      deviceName parameter is not used, but is only present
 *      in order to provide the same interface to BCHS.
 *
 * RETURNS:
 *      TRUE if the device was opened successfully,
 *      otherwise FALSE
 *************************************************************/
bool_t UsbDrv_Start(char *deviceName)
{
    if(csr_dev == NULL)
    {
        CsrUsbInit();
    }
    DBG_PRINT("The driver has been started by BCHS\n");

    register_bg_int(1, UsbDrv_Rx);

    if(csr_setup.ext_iface == TRUE)
    {
        csr_setup.ext_start();
    }

    /* Force a "fake" background interrupt so we deliver any pending
     * packets to the stack. This more or less kickstarts everything */
    atomic_inc(&(csr_dev->await_bg_int));
    wake_up_interruptible(&(csr_dev->queue.wait));

    return TRUE;
}
