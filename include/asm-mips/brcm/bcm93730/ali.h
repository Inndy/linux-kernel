/*
 * include/asm/brcm/ali.h
 *
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Ali1543C defines
 * 
 * 10-19-2001   SJH    Created 
 */

#ifndef __ASM_BRCM_ALI_H
#define __ASM_BRCM_ALI_H

/*
 * Ali PCI registers offsets from QSR_PCI_IO_ADDR
 */
#define ALI_CONFIG_KEY_BEG0	0x51
#define ALI_CONFIG_KEY_BEG1	0x23
#define ALI_CONFIG_KEY_END	0xBB
#define ALI_CONFIG_SEL_DEV	0x07

/*
 * Ali control ports
 */
#define oALI_PARALLEL_DATA	0x0378
#define oALI_PARALLEL_STATUS 	0x0379
#define oALI_PARALLEL_CONTROL 	0x037A
#define oALI_CONFIG_PORT     	0x03F0
#define oALI_DATA_PORT       	0x03F1

/*
 * Interrupt controller ports
 */
#define oICW1_M			0x0020
#define oICW2_M			0x0021
#define oICW3_M			0x0021
#define oICW4_M			0x0021
#define oICW1_S			0x00A0
#define oICW2_S			0x00A1
#define oICW3_S			0x00A1
#define oICW4_S			0x00A1
#define oOCW1_M			0x0021
#define oOCW2_M			0x0020
#define oOCW3_M			0x0020
#define oOCW1_S			0x00A1
#define oOCW2_S			0x00A0
#define oOCW3_S			0x00A0
#define oELCR_M			0x04D0	/* Unit Edge/Level Control */	
#define oELCR_S			0x04D1	/* Unit Edge/Level Control */

/*
 * Misc. defines
 */
#define IRQ_SLAVE_PIC		0x04
#define ELCR(x)			((x>8) ? oELCR_S : oELCR_M)
#define ALI_IRQ_BIT(x)		((x>8) ? (char)(1 << (x-8)) : (char)(1 << x))
#define ALI_USB_MEM_BASE	0x15013000
#define ALI_USB_MEM_SIZE	0x00000100

/*
 * Ali interrupts
 */
#define ALI_IRQ_INTA		15	/* Input from PCI Slot INTA */
#define ALI_IRQ_USB		11	/* Ali USB */ 
#define ALI_IRQ_PARALLEL	7	/* Ali Parallel Port */ 
#define ALI_IRQ_ENET		6	/* BCM4413 HPNA/V.90/ENET */
#define ALI_IRQ_KEYBOARD	1	/* Ali Keyboard */ 

/*
 * Ali PCI device identifiers
 */
#define ALI_ISA_IDSEL		2
#define ALI_ISA_VID		0x153310b9	/* PCI Vendor ID */
#define ALI_IDE_IDSEL		11
#define ALI_IDE_VID		0x522910B9	/* PCI Vendor ID */
#define ALI_PMU_IDSEL		12
#define ALI_PMU_VID		0x710110b9	/* PCI Vendor ID */
#define ALI_USB_IDSEL		15
#define ALI_USB_VID		0x523710b9	/* PCI Vendor ID */

#endif /* __ASM_BRCM_ALI_H */
