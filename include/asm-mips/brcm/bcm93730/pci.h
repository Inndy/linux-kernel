/*
 * include/asm/brcm/pci.h
 *
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * PCI defines for Broadcom eval boards
 *
 * 10-19-2001   SJH    Created 
 */

#ifndef __ASM_BRCM_PCI_H
#define __ASM_BRCM_PCI_H

/*
 * Standard PCI Configuration Registers
 * CF8 Configuration Register format:
 * 	Bit[31]		ENABLE
 * 	Bit[30-11]	PCI Device Number
 * 	Bit[10-2]	Device Register Offset
 */
#define PCI_CF8_VENDOR		0x00
#define PCI_CF8_DEVICE		0x02	
#define PCI_CF8_CMD		0x04
#define PCI_CF8_STAT		0x06
#define PCI_CF8_CC_REV		0x08
#define PCI_CF8_HDR_CFG		0x0C
#define PCI_CF8_IO_BASE		0x10	/* BAR0 */
#define PCI_CF8_MEM_BASE	0x14	/* BAR1 */
#define PCI_CF8_REG_BASE	0x18	/* BAR2 */
#define PCI_CF8_PCU_BASE	0x1C	/* BAR3 */
#define PCI_CF8_BAR4		0x20	/* BAR4 */

/*
 * PCI macros
 */
#define PCI_IDSEL(x)		(((x)&0x1f)<<11)
#define PCI_FNSEL(x)		(((x)&0x7)<<8)
#define PCI_FNCSEL(f)		(f<<8)

/*
 * Standard PCI CFC Configuration Register Values
 */
#define PCI_ENABLE		0x80000000
#define PCI_IO_ENABLE		0x00000001
#define PCI_MEM_ENABLE		0x00000002
#define PCI_BUS_MASTER		0x00000004

/*
 * BCM93725 PCI Bus Device Connections
 *
 * PCI Device Number	PCI Address	PCI Device
 *	0		AD16		Bcm4413
 *	1		AD17		BCM3250 
 *	2		AD18		ALI ISA 153310b9
 *	3		AD19
 *	4		AD20
 *	5		AD21
 *	6		AD22
 *	7		AD23
 *	8		AD24
 *	9		AD25
 *	10		AD26
 *	11		AD27		ALI IDE 522910B9
 *	12		AD28		ALI PMU 710110b9
 *	13		AD29		SLOT A
 *	14		AD30
 *	15		AD31		ALI USB 523710b9
 */
#define BCM4410_IDSEL		0
#define BCM4410_VID		0x441014e4
#define BCM4411_IDSEL		0
#define BCM4411_VID		0x441114e4
#define BCM4412_IDSEL		0
#define BCM4412_VID		0x441214e4
#define BCM4212_IDSEL		0
#define BCM4212_VID		0x421214e4
#define BCM3250_IDSEL		1
#define BCM3250_VID		0x325014e4	/* PCI Vendor ID */
#define SLOTA_IDSEL		13
#define NE2000_IDSEL		SLOTA_IDSEL
#define SIIG_IDE_VID		0x00051191	/* SIIG IDE Controller card */
#define LINKSYS_ETHERNET_VID	0x09401050	/* Linksys NE2000 Ethernet */
#define SMC_ETHERNET_VID	0x802910ec	/* SMC NE2000 Ethernet */

/*
 * BCM4413 iLine 20
 */
#define BCM4410_PCI_MEM_BASE	0x15010000
#define BCM4410_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

/*
 * BCM4413 codec
 */
#define BCM4411_PCI_MEM_BASE	0x15011000
#define BCM4411_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

/* 
 * BCM4413 ENET
 */
#define BCM4412_PCI_MEM_BASE	0x15012000
#define BCM4412_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

/* 
 * BCM4212 V.90 Softmodem
 */
#define BCM4212_PCI_MEM_BASE	0x15014000
#define BCM4212_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

/* 
 * BCM7040 MPEG2 Encoder
 */
#define BCM7040_VID		0x9000141f
#define BCM7040_MEM_BASE        0x15200000
#define BCM7040_MEM_SIZE	0x00080000	/* requires 512K bytes */

/* 
 * BCM7041 Dual MPEG2 Encoder
 */
#define BCM7041_VID		0xa000141f
#define BCM7041_MEM_BASE        0x15280000
#define BCM7041_MEM_SIZE	0x000c0000	/* requires 768K bytes */

/*
 * Misc. defines
 */
#define BCM_7020_PCI_EXT_CF8_NOCACHE	0xb3e00cf8
#define BCM_7020_PCI_EXT_CFC_NOCACHE	0xb3e00cfc
#define NE2000_PCI_IO_BASE		0x00006000

#endif /* __ASM_BRCM_PCI_H */
