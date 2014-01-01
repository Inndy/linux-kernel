/*
 * asm/brcm/usb.h
 *
 * Copyright (C) 2001 Broadcom Corporation
 *                    Sasi Kumar <sasik@broadcom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * BRCM adaption of the usb-ohci stack to support the direct (non-PCI) 
 * USB Host Controllers.  
 *
 * Include this file in usb-ohci.c BEFORE including usb-ohci.h.  This is
 * done to fake out the check for the CONFIG_PCI macro and avoid having 
 * to change the usb-ohci.h file!
 * 
 * 02-08-2002   SK     Created
*/

#define BRCM_HC
 
#ifdef BRCM_HC    //****************************************************

#include <linux/pci.h>

#if defined(CONFIG_BCM97110)
#include <asm/brcm/bcm97110/bcm7110.h>
#include <asm/brcm/bcm97110/bcmintrnum.h>
#elif defined(CONFIG_BCM97115)
#include <asm/brcm/bcm97115/bcm7115.h>
#include <asm/brcm/bcm97115/bcmintrnum.h>
#elif defined(CONFIG_BCM97315)
#include <asm/brcm/bcm97315/bcm7315.h>
#include <asm/brcm/bcm97315/bcmintrnum.h>
#elif defined(CONFIG_BCM97320)
#include <asm/brcm/bcm97320/bcm7320.h>
#include <asm/brcm/bcm97320/bcmintrnum.h>
#endif

#define CONFIG_PCI

#define HC_BASE_ADDR             USB_HOST_ADR_BASE
#define HC_END_ADDR              (USB_HOST_ADR_BASE+0xfff)
#define HC_INT_VECTOR            BCM_LINUX_USB_HOST_IRQ
#ifdef CONFIG_BCM97320
 #define HC2_BASE_ADDR            USB_HOST2_ADR_BASE
 #define HC2_END_ADDR             (USB_HOST2_ADR_BASE+0xfff)
 #define HC2_INT_VECTOR           BCM_LINUX_USB_HOST2_IRQ
#endif

#define NonOhciSetup            63              //reg...mem offset = 0xfc
enum { BABO_LE=0x01, FNHW_LE=0x02, FNBO_REV=0x04, WABO_LE=0x08,
       IOC=0x10, IPP=0X20 };
#if defined (__MIPSEB__)
  #ifdef CONFIG_DP522		//E* board expects high active port power signal
    #define NON_OHCI_SETUP_REG_VAL		(IOC)
  #else
    #define NON_OHCI_SETUP_REG_VAL		(IPP+IOC)
  #endif
#else
  #ifdef CONFIG_DP522
    #define NON_OHCI_SETUP_REG_VAL		(IOC+FNHW_LE+BABO_LE)
  #else
    #define NON_OHCI_SETUP_REG_VAL		(IPP+IOC+FNHW_LE+BABO_LE)
  #endif
#endif

#define DRIVER_MODS "Modified for BRCM USB Host Controller"

#ifdef CONFIG_ROOT_NFS
static void usb_nfs_dev_wait(void)
{
	int i;

    printk("usb-ohci.c: ROOT-NFS delay for device enumeration...\n");
   	for(i=0; i<4000; ++i)
    {
   	    schedule();     //let other threads run
        udelay(1000);
   	}
}
#endif

//some PCI prototypes
struct pci_pool * pci_pool_create (const char *, struct pci_dev *,
								   size_t, size_t, size_t, int);
void pci_pool_destroy (struct pci_pool *);
void * pci_pool_alloc (struct pci_pool *, int, dma_addr_t *);
void pci_pool_free (struct pci_pool *, void *, dma_addr_t);

#endif //BRCM_HC  //****************************************************
