/****************************************************************************

               (c) Cambridge Silicon Radio Ltd, 2006

               All rights reserved

FILE:          usb_com.c

DESCRIPTION:   USB communication in the Linux kernel. Interfaces to the USB
               sub-system.

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
#include <linux/slab.h>
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
#include "../inc/linux_usb_extra.h"
#include "../inc/linux_usb_com.h"
#include "../inc/usr_config.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/usb_ch9.h>
#endif

/* Version Information */
#define DRIVER_VERSION          "0.4"
#define DRIVER_AUTHOR           "CSR plc."
#define DRIVER_NAME             "csr_usb"

/* isoc_alt_setting
 * This setting controls which ISOC (SCO) configuration to use.
 * We assume two streams and let the usr_config deceide
 * if we use 8bit or 16bit sound. The setting (and others) can be changed
 * dynamically. The possible values are defined by the HCI specification:
 *   1 : packetsize=8,  1 * 8bit
 *   2 : packetsize=17, 1 * 16bit / 2 * 8bit
 *   3 : packetsize=25, 3 * 8bit
 *   4 : packetsize=33, 2 * 16bit
 *   5 : packetsize=49, 3 * 16bit
 */
#if ((VOICE_SETTING & INPUTSAMPLESIZE_16BIT) == INPUTSAMPLESIZE_16BIT)
#define DEFAULT_ISOC_ALT 4
#else
#define DEFAULT_ISOC_ALT 2
#endif

/* Device instance */
csr_dev_t *csr_dev = NULL;

/* Default supported USB devices */
struct usb_device_id usb_default_devices[] =
{
    { USB_DEVICE(CSR_VENDOR_ID, CSR_PRODUCT_ID) },
    { }
};

/* Global settings with default */
csr_usb_settings_t csr_setup =
{
    .ext_iface                     = FALSE,
    .ext_probe                     = NULL,
    .ext_disconnect                = NULL,
    .ext_start                     = NULL,
    .ext_init                      = NULL,
    .ext_exit                      = NULL,
    .max_isoc_in_frames            = 3,
    .max_isoc_out_frames           = 3,
    .isoc_alt_setting              = DEFAULT_ISOC_ALT,
    .driver_name                   = DRIVER_NAME,
    .usb_devices                   = NULL
};


/* USB driver table */
struct usb_driver csr_usb_driver =
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
    .owner                        = THIS_MODULE,
#endif
#endif
    .name                         = DRIVER_NAME,
    .id_table                     = usb_default_devices,
    .probe                        = csrUsbProbe,
    .disconnect                   = csrUsbDisconnect
};

/* Local prototypes */
static int16_t usbRxIntr(void);
static int16_t usbRxBulk(uint8_t number);
static int16_t usbRxIsoc(uint8_t number);

/*************************************************************
 * NAME:
 *      usbCleanUrbs
 *
 * DESCRIPTION:
 *      This function is used to remove any outstanding receive-urbs
 *      which must be done on disconnects.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void usbCleanUrbs(void)
{
    int i;

    if(csr_dev->intr_urb != NULL)
    {
        DBG_PRINT("Unlink and free Rx INTR\n");
        URB_UNLINK(csr_dev->intr_urb);
        usb_free_urb(csr_dev->intr_urb);
        csr_dev->intr_urb = NULL;
    }
    for(i=0; i<MAX_RX_BULK_URBS; i++)
    {
        if(csr_dev->bulk_urb[i] != NULL)
        {
            DBG_PRINT("Unlink and free Rx BULK_%i\n", i);
            URB_UNLINK(csr_dev->bulk_urb[i]);
            usb_free_urb(csr_dev->bulk_urb[i]);
            csr_dev->bulk_urb[i] = NULL;
        }
    }
    for(i=0; i<MAX_RX_ISOC_URBS; i++)
    {
        if(csr_dev->isoc_urb[i] != NULL)
        {
            DBG_PRINT("Unlink and free Rx ISOC_%i\n", i);
            URB_UNLINK(csr_dev->isoc_urb[i]);
            usb_free_urb(csr_dev->isoc_urb[i]);
            csr_dev->isoc_urb[i] = NULL;
        }
    }
}


/*************************************************************
 * NAME:
 *      csrUsbDisconnect
 *
 * DESCRIPTION:
 *      This function is called by the OS when the USB device is
 *      removed from the system - hence some cleanup must be done.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
void csrUsbDisconnect(struct usb_interface *intf)
#else
void csrUsbDisconnect(struct usb_device *dev, void *p)
#endif
{
    int con;

    con = test_bit(DEVICE_CONNECTED, &(csr_dev->flags));
    if(con)
    {
        printk(PRNPREFIX "Disconnecting interface\n");
    }

    /* Set the flags indicating that the module must be stopped */
    clear_bit(DEVICE_CONNECTED, &(csr_dev->flags));
    clear_bit(LISTEN_STARTED, &(csr_dev->flags));
    clear_bit(BULK_IN_READY, &(csr_dev->endpoint_present));
    clear_bit(BULK_OUT_READY, &(csr_dev->endpoint_present));
    clear_bit(INTR_IN_READY, &(csr_dev->endpoint_present));
    clear_bit(ISOC_IN_READY, &(csr_dev->endpoint_present));
    clear_bit(ISOC_OUT_READY, &(csr_dev->endpoint_present));
    clear_bit(EXTRA_INTERFACE_READY, &(csr_dev->endpoint_present));
    clear_bit(DFU_READY, &(csr_dev->endpoint_present));

    /* Disconnect extra interface */
    if(csr_setup.ext_iface==TRUE)
    {
        csr_setup.ext_disconnect();
    }

    usbCleanUrbs();

    /* Empty the pending queue */
    if(con)
    {
        QueueFree();
    }

    /* Release reassembly buffers */
    if(csr_dev->isoc_reassembly != NULL)
    {
        pfree(csr_dev->isoc_reassembly);
        csr_dev->isoc_reassembly = NULL;
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    if(csr_dev->intr_reassembly != NULL)
    {
        pfree(csr_dev->intr_reassembly);
        csr_dev->intr_reassembly = NULL;
    }
#endif
}

/*************************************************************
 * NAME:
 *      usbTxComplete (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever transmission to the
 *      device has completed. It is a call back function,
 *      hence it is not called from this application. If there
 *      are data on the outgoing queue, they will be sent
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)) &&   \
     (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbTxComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbTxComplete(struct urb *urb)
#endif
{
    static char *type[] = { "CTRL", "BULK", "ISOC" };

    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        printk(PRNPREFIX "Tx %s completion error, status: %d\n",
               type[(int)urb->context], urb->status);
    }
    else
    {
        DBG_VERBOSE("Tx %s complete\n", type[(int)urb->context]);
    }

    /* Free the data no matter what */
    kfree(urb->transfer_buffer);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)
    /* Old kernels can have only one pending URB at the time, so we
     * use a simple tx wait barrier, which must be notified from here */
    up(&(csr_dev->barrier[(int)urb->context]));
#endif
}


/*************************************************************
 * NAME:
 *      fillIsocDesc
 *
 * DESCRIPTION:
 *      This function fills the ISOC description
 *
 * PARAM:
 *      isocUrb: A pointer to the urb that will have its description filled.
 *      size: Total size of transfer_buffer
 *      mtu: The length of the data to be sent (frame size)
 *      max: The number of frames
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void fillIsocDesc(struct urb *isocUrb,
                         int size,
                         int mtu,
                         int max)
{
    uint16_t offset;
    uint16_t i;

    for(i=0, offset = 0;
        (i < max) && (size >= mtu);
        i++, offset += mtu, size -= mtu)
    {
        isocUrb->iso_frame_desc[i].offset = offset;
        isocUrb->iso_frame_desc[i].length = mtu;
    }
    if(i < max && size)
    {
        isocUrb->iso_frame_desc[i].offset = offset;
        isocUrb->iso_frame_desc[i].length = size;
        i++;
    }
    isocUrb->number_of_packets = i;
}

/*************************************************************
 * NAME:
 *      usbTxIsoc
 *
 * DESCRIPTION:
 *      This function is called whenever a ISOC message (SCO) is
 *      to be send to the usb device.
 *
 * RETURNS:
 *      0 if successfull, otherwise an error message
 *
 *************************************************************/
int16_t usbTxIsoc(void *data, uint16_t length)
{
    struct urb *txisoc;
    uint8_t err;

    err = -ENOMEM;
    txisoc = URB_ALLOC(csr_setup.max_isoc_out_frames, GFP_KERNEL);
    if(txisoc != NULL)
    {
        /* ISOC URBs must be filled manually */
        txisoc->dev = csr_dev->dev;
        txisoc->pipe = usb_sndisocpipe(csr_dev->dev,
                                    csr_dev->isoc_out_ep);
        txisoc->transfer_buffer = data;
        txisoc->complete = usbTxComplete;
        txisoc->transfer_buffer_length = length;
        txisoc->interval = csr_dev->isoc_out_interval;
        txisoc->context = (int*)IDX_ISOC;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        txisoc->transfer_flags = URB_ISO_ASAP;
#else
        txisoc->transfer_flags = USB_ISO_ASAP;
#endif

        /* Setup frame size and offset */
        fillIsocDesc(txisoc,
                     length,
                     length / csr_setup.max_isoc_out_frames,
                     csr_setup.max_isoc_out_frames);

        err = URB_SUBMIT(txisoc, GFP_KERNEL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)
        down_interruptible(&(csr_dev->barrier[IDX_ISOC]));
#endif

        if(err !=0 )
        {
            printk(PRNPREFIX "Tx ISOC submit error, status: %d\n",
                   err);

            /* Attempt to resubmit */
            err = URB_SUBMIT(txisoc, GFP_KERNEL);
            if(err !=0 )
            {
                /* This failed too, so unlink in order to free and up(sem) */
                printk(PRNPREFIX "Tx ISOC re-submit error, status: %d\n",
                       err);
                URB_UNLINK(txisoc);
            }
        }
        else
        {
            DBG_VERBOSE("Transmitted %i ISOC bytes\n", length);
        }

        /* Free the URB */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        usb_free_urb(txisoc);
#endif
    }
    else
    {
        printk(PRNPREFIX "Tx ISOC alloc error\n");
    }
    return err;
}

/*************************************************************
 * NAME:
 *      usbTxCtrl (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever a control message
 *      (HCI-command) is to be send to the usb device.
 *
 * RETURNS:
 *      0 if successfull, otherwise an error message
 *
 *************************************************************/
int16_t usbTxCtrl(void *data, uint16_t length)
{
    int16_t err;
    struct urb *txctrl;

    err = -ENOMEM;
    txctrl = URB_ALLOC(0, GFP_KERNEL);
    if(txctrl != NULL)
    {
        /* Setup packet */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
        csr_dev->ctrl_setup.wLength = cpu_to_le16(length);
#else
        csr_dev->ctrl_setup.length = cpu_to_le16(length);
#endif

        usb_fill_control_urb(txctrl,
                             csr_dev->dev,
                             usb_sndctrlpipe(csr_dev->dev,
                                             csr_dev->ctrl_ep),
                             (unsigned char*)&csr_dev->ctrl_setup,
                             data,
                             length,
                             usbTxComplete,
                             (int*)IDX_CONTROL);

        err = URB_SUBMIT(txctrl, GFP_KERNEL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)
        down_interruptible(&(csr_dev->barrier[IDX_CONTROL]));
#endif

        if(err != 0)
        {
            printk(PRNPREFIX "Tx CTRL submit error, code %d\n", err);

            /* Attempt to resubmit */
            err = URB_SUBMIT(txctrl, GFP_KERNEL);
            if(err !=0 )
            {
                /* This failed too, so unlink in order to free and up(sem) */
                printk(PRNPREFIX "Tx CTRL re-submit error, status: %d\n",
                       err);
                URB_UNLINK(txctrl);
            }
        }
        else
        {
            DBG_VERBOSE("Transmitted %i CTRL bytes\n", length);
        }

        /* Free the URB */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        usb_free_urb(txctrl);
#endif
    }
    else
    {
        printk(PRNPREFIX "Tx CTRL alloc error, code %d\n", err);
    }

    return err;
}

/*************************************************************
 * NAME:
 *      usbTxBulk
 *
 * DESCRIPTION:
 *      This function is called whenever a bulk message (ACL) is
 *      to be send to the usb device.
 *
 * RETURNS:
 *      0 if successfull, otherwise an error message
 *
 *************************************************************/
int16_t usbTxBulk(void *data, uint16_t length)
{
    int16_t err;
    struct urb *txbulk;

    err = -ENOMEM;
    txbulk = URB_ALLOC(0, GFP_KERNEL);
    if(txbulk != NULL)
    {
        usb_fill_bulk_urb(txbulk,
                          csr_dev->dev,
                          usb_sndbulkpipe(csr_dev->dev,
                                          csr_dev->bulk_out_ep),
                          data,
                          length,
                          usbTxComplete,
                          (int*)IDX_BULK);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        txbulk->transfer_flags |= USB_QUEUE_BULK;
#endif
        err = URB_SUBMIT(txbulk, GFP_KERNEL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)
        down_interruptible(&(csr_dev->barrier[IDX_BULK]));
#endif

        if(err!=0)
        {
            printk(PRNPREFIX "Tx BULK submit error, code %d\n", err);

            /* Attempt to resubmit */
            err = URB_SUBMIT(txbulk, GFP_KERNEL);
            if(err !=0 )
            {
                /* This failed too, so unlink in order to free and up(sem) */
                printk(PRNPREFIX "Tx BULK re-submit error, status: %d\n",
                       err);
                URB_UNLINK(txbulk);
            }
        }
        else
        {
            DBG_VERBOSE("Transmitted %i BULK bytes\n", length);
        }

        /* Free the URB */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        usb_free_urb(txbulk);
#endif
    }
    else
    {
        printk(PRNPREFIX "Tx BULK alloc error, code %d\n", err);
    }

    return err;
}

/*************************************************************
 * NAME:
 *      reassembleIsoc
 *
 * DESCRIPTION:
 *      This function reassembles ISOC frames to SCO packets.
 *      Calling context is FIXME
 *
 * PARAMETERS:
 *      data: A pointer to the data to be reassembled
 *      length: The length of the data block
 *
 * RETURNS:
 *      Error code, zero is success
 *
 *************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
static int32_t reassembleIsoc(uint8_t *dat,
                              struct usb_iso_packet_descriptor *frameDesc)
#else
static int32_t reassembleIsoc(uint8_t *dat,
                              iso_packet_descriptor_t *frameDesc)
#endif
{
    uint8_t len;
    uint32_t length;
    uint8_t *data;

    data = dat + frameDesc->offset;
    length = frameDesc->actual_length;

    /* The frame was not ok, remove possible previously received data to
     * be reassembled with this part */
    if(frameDesc->status != 0)
    {
        if(csr_dev->isoc_reassembly != NULL)
        {
            pfree(csr_dev->isoc_reassembly);
            csr_dev->isoc_reassembly = NULL;
            csr_dev->isoc_total = 0;
            if(length <= csr_dev->isoc_remain)
            {
                csr_dev->isoc_remain -= length;
                return frameDesc->status;
            }
            else
            {
                length -= csr_dev->isoc_remain;
                data += length;
            }
        }
    }

    /* Status of frame is OK, but the previous received frame to be
     * reassembled with this one was not ok, hence discard this frame
     * as well */
    if((csr_dev->isoc_reassembly == NULL) &&
       (csr_dev->isoc_remain != 0))
    {
        if(length <= csr_dev->isoc_remain)
        {
            csr_dev->isoc_remain -= length;
            return -1;
        }
        else
        {
            length -= csr_dev->isoc_remain;
            data += length;
        }
    }

    /* Everything is fine - start normal reassembly procedure */
    while(length)
    {
        len = 0;
        /* Beginning of frame */
        if(csr_dev->isoc_reassembly==NULL)
        {
            if(length >= HCI_SCO_HDR_SIZE)
            {
                /* The size is specified in byte 3, therefore "+2" */
                csr_dev->isoc_total = *(uint8_t *)(data+2) + HCI_SCO_HDR_SIZE;
                csr_dev->isoc_remain = csr_dev->isoc_total;
            }
            else
            {
                return -EILSEQ;
            }

            csr_dev->isoc_reassembly = (uint8_t*)pmalloc(csr_dev->isoc_total);
        }
        /* Continuation of packet */
        else
        {
        }

        len = min(csr_dev->isoc_remain, length);
        memcpy(&csr_dev->isoc_reassembly[csr_dev->isoc_total - csr_dev->isoc_remain],
               data,
               len);
        csr_dev->isoc_remain -= len;

        /* The frame is complete */
        if(!csr_dev->isoc_remain)
        {
            /* Let the queue consume the data (don't free it!) */
            QueueAppend(BCSP_CHANNEL_SCO,
                        csr_dev->isoc_reassembly,
                        csr_dev->isoc_total,
                        FALSE);
            csr_dev->isoc_reassembly = NULL;
            atomic_inc(&(csr_dev->await_bg_int));
            wake_up_interruptible(&(csr_dev->queue.wait));
        }
        length -= len;
        data += len;
    }

    return 0;
}

/*************************************************************
 * NAME:
 *      reassembleIntr
 *
 * DESCRIPTION:
 *      Reassmeble fragmented INTR packets. This functionality
 *      is needed for 2.4.x kernels that does not handle this
 *      automatically.
 *
 * RETURNS:
 *
 *************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int32_t reassembleIntr(struct urb *urb)
{
    uint32_t length;
    uint32_t len;
    uint8_t  *data;

    data = urb->transfer_buffer;
    length = urb->actual_length;

    while(length)
    {
        len = 0;

        /* Beginning of frame */
        if(csr_dev->intr_reassembly==NULL)
        {
            if(length >= HCI_EVENT_HDR_SIZE)
            {
                /* The size is specified in byte 2, therefore "+1" */
                csr_dev->intr_total = *(uint8_t *)(data+1) +
                    HCI_EVENT_HDR_SIZE;
                csr_dev->intr_remain = csr_dev->intr_total;
            }
            else
            {
                return -EILSEQ;
            }

            csr_dev->intr_reassembly = (uint8_t*)pmalloc(csr_dev->intr_total);
        }
        /* Continuation of packet */
        else
        {
        }

        len = min(csr_dev->intr_remain, length);

        memcpy(&csr_dev->intr_reassembly[csr_dev->intr_total - csr_dev->intr_remain],
               data,
               len);

        csr_dev->intr_remain -= len;

        /* The frame is complete */
        if(csr_dev->intr_remain == 0)
        {
            /* Let the queue consume the data (don't free it!) */
            QueueAppend(BCSP_CHANNEL_HCI,
                        csr_dev->intr_reassembly,
                        csr_dev->intr_total,
                        FALSE);
            csr_dev->intr_reassembly = NULL;
            atomic_inc(&(csr_dev->await_bg_int));
            wake_up_interruptible(&(csr_dev->queue.wait));
        }
        length -= len;
        data += len;
    }
    return 0;
}
#endif

/*************************************************************
 * NAME:
 *      usbRxIntrComplete
 *
 * DESCRIPTION:
 *     This function is called asynchronous whenever hci
 *     events are present on the USB device. The received data is copied
 *     to a buffer structure and the scheduler is informed by issuing a
 *     bg_interrupt.  Finally, the function makes a new data read request
 *     to the device.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)) &&   \
     (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbRxIntrComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbRxIntrComplete(struct urb *urb)
#endif
{
    /* Data is available */
    if((urb->status == 0) && (urb->actual_length > 0))
    {
#if 0
        DBG_VERBOSE("Rx INTR complete, %d bytes received\n",
                    urb->actual_length);
#endif

        if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
            /* Copy data to queue so URB can be resubmitted */
            QueueAppend(BCSP_CHANNEL_HCI,
                        urb->transfer_buffer,
                        urb->actual_length,
                        TRUE);

            atomic_inc(&(csr_dev->await_bg_int));
            wake_up_interruptible(&(csr_dev->queue.wait));
#else
            reassembleIntr(urb);
#endif
        }
        else
        {
            DBG_PRINT("Received INTR data - not sending up!\n");
        }
    }
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        URB_UNLINK(urb);
#endif
        DBG_VERBOSE("Rx INTR complete error, code %d\n", urb->status);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
    {
        int16_t err;

        /* Resubmit the urb */
        urb->dev = csr_dev->dev;
        urb->actual_length = 0;
        err = URB_SUBMIT(urb, GFP_ATOMIC);

        /* Success or disconnect not errors */
        if((err !=0) && (err != -ENODEV))
        {
            printk(PRNPREFIX "Rx INTR resubmit error, status %d\n",
                   err);
        }
    }
#endif
}

/*************************************************************
 * NAME:
 *      usbRxBulkComplete
 *
 * DESCRIPTION:
 *      This function is called asynchronous whenever ACL data is
 *      present on the USB device. The received data is copied to a
 *      buffer structure and the scheduler is informed by issuing a
 *      bg_interrupt.  Finally, the function makes a new data read
 *      request to the device.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)) &&   \
     (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbRxBulkComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbRxBulkComplete(struct urb *urb)
#endif
{
    int16_t err;

    /* Data is available */
    if((urb->status == 0) && (urb->actual_length > 0))
    {
        DBG_VERBOSE("Rx BULK complete, %d bytes received\n",
                    urb->actual_length);

        if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
        {
            /* Copy data to queue so URB can be resubmitted */
            QueueAppend(BCSP_CHANNEL_ACL,
                        urb->transfer_buffer,
                        urb->actual_length,
                        TRUE);

            atomic_inc(&(csr_dev->await_bg_int));
            wake_up_interruptible(&(csr_dev->queue.wait));
        }
        else
        {
            DBG_PRINT("BULK data received - not sending up!\n");
        }
    }
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        DBG_VERBOSE("Rx BULK complete error, code %d\n", urb->status);
    }

    if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
    {
        /* Resubmit the urb */
        urb->dev = csr_dev->dev;
        err = URB_SUBMIT(urb, GFP_ATOMIC);

        /* Success or disconnect not errors */
        if((err != 0) && (err != -ENODEV))
        {
            printk(PRNPREFIX "Rx BULK resubmit error, code %d\n",
                   err);
        }
    }
}

/*************************************************************
 * NAME:
 *      usbRxIsocComplete
 *
 * DESCRIPTION:
 *      This function is called asynchronous whenever sco data
 *      are present on the USB device. The received data is copied to a
 *      buffer structure and the scheduler is informed by issuing a
 *      bg_interrupt.  Finally, the function makes a new data read request
 *      to the device.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)) &&   \
     (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbRxIsocComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbRxIsocComplete(struct urb *urb)
#endif
{
    int16_t err, i;

    /* Data is available */
    err = 0;
    if((urb->status == 0) && (urb->actual_length > 0))
    {
#if 0
        DBG_VERBOSE("Rx ISOC complete, %d bytes received\n",
                    urb->actual_length);
#endif

        if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
        {
            for(i=0; i < urb->number_of_packets; i++)
            {
                reassembleIsoc((uint8_t *)urb->transfer_buffer,
                               &urb->iso_frame_desc[i]);
            }
        }
        else
        {
            DBG_PRINT("ISOC data received - not sending up!\n");
        }
    }
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        URB_UNLINK(urb);
#endif
        DBG_VERBOSE("Rx ISOC complete error, code %d\n", urb->status);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if(test_bit(DEVICE_CONNECTED, &(csr_dev->flags)))
    {
        /* Resubmit the URB */
        urb->dev = csr_dev->dev;
        err = URB_SUBMIT(urb, GFP_ATOMIC);

        /* Success or disconnect not errors */
        if((err != 0) && (err != -ENODEV))
        {
            printk(PRNPREFIX "Rx ISOC resubmit error, code %d\n",
                   err);
        }
    }
#endif
}

/*************************************************************
 * NAME:
 *      usbRxIntr (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever a read for HCI events
 *      is wanted. The request will result in a asynchronous
 *      call of the usbRxComplete function, when data has been
 *      read from the device.
 *
 * RETURNS:
 *      0 on success, otherwise a number indicating the error
 *
 *************************************************************/
static int16_t usbRxIntr(void)
{
    int16_t err;
    void *buf;
    struct urb *rxintr;

    err = -ENOMEM;
    rxintr = URB_ALLOC(0, GFP_ATOMIC);
    buf = pmalloc(MAX_HCI_EVENT_SIZE);

    if((rxintr != NULL) && (buf != NULL))
    {
        usb_fill_int_urb(rxintr,
                         csr_dev->dev,
                         usb_rcvintpipe(csr_dev->dev,
                                        csr_dev->intr_ep),
                         buf,
                         csr_dev->intr_size,
                         usbRxIntrComplete,
                         rxintr,
                         csr_dev->intr_interval);


        err = URB_SUBMIT(rxintr, GFP_ATOMIC);
        if(err != 0)
        {
            URB_UNLINK(rxintr);
            usb_free_urb(rxintr);
            csr_dev->intr_urb = NULL;
            printk(PRNPREFIX "Rx INTR submit error, code %d\n", err);
        }
        else
        {
            csr_dev->intr_urb = rxintr;
        }
    }
    else
    {
        printk(PRNPREFIX "Rx INTR alloc error, code %d\n", err);
        if(rxintr)
        {
            pfree(rxintr);
        }
        if(buf)
        {
            pfree(buf);
        }
    }

    return err;
}

/*************************************************************
 * NAME:
 *      usbRxBulk (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever a read for BULK data
 *      is wanted. The request will result in a asynchronous
 *      call of the usbRxComplete function, when data has been
 *      read from the device.
 *
 * PARAMS:
 *      number: is the urb number in the bulk urb array
 *
 * RETURNS:
 *      0 on success, otherwise a number indicating the error
 *
 *************************************************************/
static int16_t usbRxBulk(uint8_t number)
{
    int16_t err;
    void *buf;
    struct urb *rxbulk;

    err = -ENOMEM;
    rxbulk = URB_ALLOC(0, GFP_ATOMIC);
    buf = pmalloc(MAX_HCI_ACL_SIZE);
    if((rxbulk != NULL) && (buf != NULL))
    {
        usb_fill_bulk_urb(rxbulk,
                          csr_dev->dev,
                          usb_rcvbulkpipe(csr_dev->dev,
                                          csr_dev->bulk_in_ep),
                          buf,
                          MAX_HCI_ACL_SIZE,
                          usbRxBulkComplete,
                          rxbulk);

        err = URB_SUBMIT(rxbulk, GFP_ATOMIC);
        if(err != 0)
        {
            URB_UNLINK(rxbulk);
            usb_free_urb(rxbulk);
            csr_dev->bulk_urb[number] = NULL;
            printk(PRNPREFIX "Rx BULK submit error, code %d\n",
                   err);
        }
        else
        {
            csr_dev->bulk_urb[number] = rxbulk;
        }
    }
    else
    {
        printk(PRNPREFIX "Rx BULK alloc error, code %d\n", err);
        if(rxbulk)
        {
            pfree(rxbulk);
        }
        if(buf)
        {
            pfree(buf);
        }
    }
    return err;
}

/*************************************************************
 * NAME:
 *      usbRxIsoc
 *
 * DESCRIPTION:
 *      This function is called whenever a read for ISOC data
 *      is wanted. The request will result in an asynchronous
 *      call of the usbRxComplete function, when data has been
 *      read from the device.
 *
 * RETURNS:
 *      0 on success, otherwise a number indicating the error
 *
 *************************************************************/
static int16_t usbRxIsoc(uint8_t number)
{
    int16_t err;
    uint32_t size;
    void *buf;
    struct urb *rxisoc;

    err = -ENOMEM;
    size = csr_dev->isoc_in_size * csr_setup.max_isoc_in_frames;
    rxisoc = URB_ALLOC(csr_setup.max_isoc_in_frames, GFP_ATOMIC);
    buf = pmalloc(size);
    if((rxisoc != NULL) && (buf != NULL))
    {
        rxisoc->dev                     = csr_dev->dev;
        rxisoc->context                 = NULL;
        rxisoc->pipe                    = usb_rcvisocpipe(csr_dev->dev,
                                                          csr_dev->isoc_in_ep);
        rxisoc->complete                = usbRxIsocComplete;
        rxisoc->interval                = csr_dev->isoc_in_interval;
        rxisoc->transfer_buffer         = buf;
        rxisoc->transfer_buffer_length  = size;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        rxisoc->transfer_flags          = URB_ISO_ASAP;
#else
        rxisoc->transfer_flags          = USB_ISO_ASAP;
#endif

        fillIsocDesc(rxisoc,
                     size,
                     size / csr_dev->isoc_in_size,
                     csr_setup.max_isoc_in_frames);

        err = URB_SUBMIT(rxisoc, GFP_ATOMIC);
        if(err!=0)
        {
            URB_UNLINK(rxisoc);
            usb_free_urb(rxisoc);
            csr_dev->isoc_urb[number] = NULL;

            printk(PRNPREFIX "Rx ISOC submit error, code %d\n",
                   err);
        }
        else
        {
            csr_dev->isoc_urb[number] = rxisoc;
        }
    }
    else
    {
        printk(PRNPREFIX "Rx ISOC alloc error, code %d\n",
               err);
        if(rxisoc)
        {
            pfree(rxisoc);
        }
        if(buf)
        {
            pfree(buf);
        }
    }
    return err;
}

/*************************************************************
 * NAME:
 *      startListen
 *
 * DESCRIPTION:
 *      This functions transmits the first receive URBs.
 *      It is called when the probe function has found all interfaces.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void startListen(void)
{
    int res;
    int i;

    set_bit(LISTEN_STARTED, &(csr_dev->flags));

    /* Start listening for ACL and HCI events */
    res = usbRxIntr();
    if(res != 0)
    {
        printk(PRNPREFIX "Error starting Rx INTR, code %d\n", res);
    }
    else
    {
        for(i=0; (i < MAX_RX_BULK_URBS) && (res == 0); i++)
        {
            res = usbRxBulk(i);
            if(res != 0)
            {
                printk(PRNPREFIX "Error starting Rx BULK, code %d\n", res);
            }
        }

        for(i=0; (i < MAX_RX_ISOC_URBS) && (res == 0); i++)
        {
            res = usbRxIsoc(i);
            if(res != 0)
            {
                printk(PRNPREFIX "Error starting Rx ISOC, code %d\n",res);
            }
        }
    }

    DBG_PRINT("Listen loop started, code %i\n", res);
}

/*************************************************************
 * NAME:
 *      handleIsocInEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for ISOC-IN endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleIsocInEndpoint(int alt,
                                 struct usb_endpoint_descriptor *endpoint,
                                 int ifnum)
{
    if(alt == csr_setup.isoc_alt_setting)
    {
        csr_dev->isoc_reassembly = NULL;
        if(usb_set_interface(csr_dev->dev, ifnum,
                             csr_setup.isoc_alt_setting))
        {
            printk("Cannot set ISOC interface alternate settings\n");
        }
        else
        {
            printk(PRNPREFIX "Has found an ISOC in endpoint\n");

            /*
             * The interval of the USB descriptor of the isochronous
             * interface is expressed as 2**(N-1), where N is the value to
             * read from the descriptor.
             * When setting up the isochronous interface we must specify the
             * interval as number of (micro)frames, hence we must convert
             * N to nr of frames.
             */
            if(endpoint->bInterval==1)
            {
                csr_dev->isoc_in_interval=1;
            }
            else
            {
                csr_dev->isoc_in_interval = 1 << (endpoint->bInterval-1);
            }
            printk(PRNPREFIX "   Interval: %d\n",
                   csr_dev->isoc_in_interval);

            csr_dev->isoc_in_size = endpoint->wMaxPacketSize;

            printk(PRNPREFIX "   MPS: %d\n",csr_dev->isoc_in_size);
            csr_dev->isoc_in_ep = endpoint->bEndpointAddress;

            set_bit(ISOC_IN_READY, &csr_dev->endpoint_present);
        }
    }
}

/*************************************************************
 * NAME:
 *      handleIsocOutEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for ISOC-OUT endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleIsocOutEndpoint(int alt,
                                  struct usb_endpoint_descriptor *endpoint)
{
    if(alt == csr_setup.isoc_alt_setting)
    {
        printk(PRNPREFIX "Has found an ISOC out endpoint\n");

        csr_dev->isoc_out_size = endpoint->wMaxPacketSize;

        if(endpoint->bInterval==1)
        {
            csr_dev->isoc_out_interval = 1;
        }
        else
        {
            csr_dev->isoc_out_interval = 1 << (endpoint->bInterval-1);
        }

        printk(PRNPREFIX "   Interval: %d\n",csr_dev->isoc_out_interval);
        printk(PRNPREFIX "   MPS: %d\n",csr_dev->isoc_out_size);

        csr_dev->isoc_out_ep = endpoint->bEndpointAddress;

        set_bit(ISOC_OUT_READY, &csr_dev->endpoint_present);
    }
}

/*************************************************************
 * NAME:
 *      handleBulkInEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for BULK-IN endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleBulkInEndpoint(int alt,
                                 struct usb_endpoint_descriptor *endpoint)
{
    printk(PRNPREFIX "Has found a BULK in endpoint\n");
    printk(PRNPREFIX "   Interval: %d\n", endpoint->bInterval);
    printk(PRNPREFIX "   MPS: %d\n", endpoint->wMaxPacketSize);
    csr_dev->bulk_in_ep = endpoint->bEndpointAddress;

    set_bit(BULK_IN_READY, &csr_dev->endpoint_present);
}

/*************************************************************
 * NAME:
 *      handleBulkOutEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for BULK-OUT endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleBulkOutEndpoint(int alt,
                                  struct usb_endpoint_descriptor *endpoint)
{
    printk(PRNPREFIX "Has found a BULK out endpoint\n");
    printk(PRNPREFIX "   Interval: %d\n", endpoint->bInterval);
    printk(PRNPREFIX "   MPS: %d\n", endpoint->wMaxPacketSize);
    csr_dev->bulk_out_ep = endpoint->bEndpointAddress;

    set_bit(BULK_OUT_READY, &csr_dev->endpoint_present);
}

/*************************************************************
 * NAME:
 *      handleIntrInEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for INTR-IN endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleIntrInEndpoint(int alt,
                                 struct usb_endpoint_descriptor *endpoint)
{
    printk(PRNPREFIX "Has found an INTR in endpoint\n");
    printk(PRNPREFIX "   Interval: %d\n", endpoint->bInterval);
    printk(PRNPREFIX "   MPS: %d\n", endpoint->wMaxPacketSize);
    csr_dev->intr_ep = endpoint->bEndpointAddress;
    csr_dev->intr_interval = endpoint->bInterval;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    csr_dev->intr_size = MAX_HCI_EVENT_SIZE;
#else
    csr_dev->intr_size = endpoint->wMaxPacketSize;
#endif

    set_bit(INTR_IN_READY, &csr_dev->endpoint_present);
}

/*************************************************************
 * NAME:
 *      handleDfuInterface
 *
 * DESCRIPTION:
 *      Probe helper for DFU interfaces
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleDfuInterface(int ifnum)
{
    printk(PRNPREFIX "Has found a DFU interface\n");
    printk(PRNPREFIX "   Interface: %d\n", ifnum);
    csr_dev->dfu_iface = ifnum;
    set_bit(DFU_READY, &csr_dev->endpoint_present);
}

/*************************************************************
 * NAME:
 *      csrUsbProbe
 *
 * DESCRIPTION:
 *      Is called whenever a USB device is inserted, which
 *      is believed to be of interest for this driver.
 *
 * RETURNS:
 *
 *
 *************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
int csrUsbProbe(struct usb_interface *intf,
                const struct usb_device_id *id)
#else
void *csrUsbProbe(struct usb_device *uDev,
                  unsigned int ifnum,
                  const struct usb_device_id *id)
#endif
{
    uint8_t i;
    uint8_t num_ep;
    int alt;
    struct usb_endpoint_descriptor *endpoint;
    struct usb_interface_descriptor *ifaceDesc;
    bool_t res;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    struct usb_host_interface *host;
    struct usb_device *uDev = interface_to_usbdev(intf);
    unsigned int ifnum;
#else
    struct usb_interface *intf = usb_ifnum_to_if(uDev,ifnum);
#endif

    printk(PRNPREFIX "Probing interface\n");
    res = FALSE;
    csr_dev->dev = uDev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)
    for(i=0; i<3; i++)
    {
        init_MUTEX(&(csr_dev->barrier[i]));
    }
#endif

    for(alt=0; alt<intf->num_altsetting; alt++)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        host = &(intf->altsetting[alt]);
        ifaceDesc = &(host->desc);
        num_ep = ifaceDesc->bNumEndpoints;
        ifnum = ifaceDesc->bInterfaceNumber;
#else
        ifaceDesc = &(intf->altsetting[alt]);
        num_ep = ifaceDesc->bNumEndpoints;
#endif

        /* Detect DFU interface */
        if((ifaceDesc->bNumEndpoints == 0) &&
           (ifaceDesc->bInterfaceClass == DFU_IFACE_CLASS) &&
           (ifaceDesc->bInterfaceSubClass == DFU_IFACE_SUB_CLASS) &&
           (ifaceDesc->bInterfaceProtocol == DFU_IFACE_PROTOCOL))
        {
            if(!test_bit(DFU_READY, &csr_dev->endpoint_present))
            {
                handleDfuInterface(ifnum);
            }
        }

        /* Scan endpoints */
        for (i=0; i < num_ep; i++)
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
            endpoint = &host->endpoint[i].desc;
#else
            endpoint = &ifaceDesc->endpoint[i];
#endif

            /* BULK in */
            if((endpoint->bEndpointAddress & USB_DIR_IN) &&
               ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                == USB_ENDPOINT_XFER_BULK))
            {
                if(csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(csr_dev, ifnum, intf, alt);
                }
                if((res == FALSE) &&
                    !test_bit(BULK_IN_READY, &csr_dev->endpoint_present))
                {
                    handleBulkInEndpoint(alt,endpoint);
                }
            }

            /* BULK out */
            else if(!(endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_BULK))
            {
                if(csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(csr_dev, ifnum, intf, alt);
                }
                if((res == FALSE) &&
                    !test_bit(BULK_OUT_READY, &csr_dev->endpoint_present))
                {
                    handleBulkOutEndpoint(alt,endpoint);
                }
            }

            /* INTR in */
            else if((endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_INT))
            {
                if(csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(csr_dev, ifnum, intf, alt);
                }
                if((res == FALSE) &&
                   !test_bit(INTR_IN_READY, &csr_dev->endpoint_present))
                {
                    handleIntrInEndpoint(alt,endpoint);
                }
            }

            /* ISOC in */
            else if((endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_ISOC))
            {
                if(csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(csr_dev, ifnum, intf, alt);
                }
                if((res == FALSE) &&
                   !test_bit(ISOC_IN_READY, &csr_dev->endpoint_present))
                {
                    handleIsocInEndpoint(alt, endpoint, ifnum);
                }
            }

            /* ISOC out */
            else if(!(endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_ISOC))
            {
                if(csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(csr_dev, ifnum,
                                              intf, alt);
                }
                if((res == FALSE) &&
                   !test_bit(ISOC_OUT_READY, &csr_dev->endpoint_present))
                {
                    handleIsocOutEndpoint(alt,endpoint);
                }
            }
            else
            {
                printk(PRNPREFIX "Unknown endpoint\n");
            }
        }
    }

    if(test_bit(BULK_IN_READY, &csr_dev->endpoint_present) &&
       test_bit(BULK_OUT_READY, &csr_dev->endpoint_present) &&
       test_bit(ISOC_OUT_READY, &csr_dev->endpoint_present) &&
       test_bit(ISOC_IN_READY, &csr_dev->endpoint_present) &&
       test_bit(INTR_IN_READY, &csr_dev->endpoint_present) &&
       !test_bit(LISTEN_STARTED, &(csr_dev->flags)))
    {
        DBG_PRINT("All required endpoints found, starting loop\n");
        set_bit(DEVICE_CONNECTED, &(csr_dev->flags));
        startListen();
    }


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    return 0;
#else
    return(csr_dev);
#endif
}
