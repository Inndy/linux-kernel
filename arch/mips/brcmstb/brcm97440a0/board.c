/*
 * arch/mips/brcmstb/brcm97038/board.c
 *
 * Copyright (C) 2004-2005 Broadcom Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Board Specific routines for Broadcom eval boards
 *
 * when         who    what
 * ----         ---    ----
 * 03-31-2004   THT    Created
 */

#include <linux/config.h>
// For module exports
#include <linux/module.h>

#include <asm/brcmstb/common/brcmstb.h>
#include <asm/brcmstb/brcm97400a0/bchp_pci_cfg.h>

#ifdef CONFIG_MIPS_BRCM_IKOS

#ifdef DRAM_SIZE
#undef DRAM_SIZE
#endif

// For Ikos
#define DRAM_SIZE	(32 << 20)
#endif

#define NUM_DDR 4


#define SUN_TOP_CTRL_STRAP_VALUE 0xb040401c

#define STRAP_PCI_MEMWIN_SIZE_SHIFT 7
#define STRAP_PCI_MEMWIN_SIZE_MASK 0x00000018	/* Bit 7 & 8 */

#define STRAP_DDR2_0_CONFIGURATION_SHIFT 	13
#define STRAP_DDR2_0_CONFIGURATION_MASK  0x00006000  /* Bits 13-14 */

#define STRAP_DDR2_1_CONFIGURATION_SHIFT 	15
#define STRAP_DDR2_1_CONFIGURATION_MASK  0x00018000  /* Bits 15-16 */


static unsigned long
board_init_once(void)
{
	unsigned long regval;
	unsigned long memSize0 = 256 << 20;
	unsigned long memSize1 = 256 << 20;
	unsigned long ddr0, ddr1;
	unsigned long pci_memwin_size;
	
	regval = *((volatile unsigned long *) SUN_TOP_CTRL_STRAP_VALUE) ;

	/* Bit 12-13: 	DDR configuration for DDR0
	 * 			0 =  256MB using 16Mx16 parts
	 *       		1 =  512MB using 32Mx16 parts
	 *			2 = 1024MB using 64Mx16 parts
	 *			3 = 1024MB using 128Mx16 parts
	 */

	ddr0 = (regval & STRAP_DDR2_0_CONFIGURATION_MASK) >> STRAP_DDR2_0_CONFIGURATION_SHIFT;
	ddr1 = (regval & STRAP_DDR2_1_CONFIGURATION_MASK) >> STRAP_DDR2_1_CONFIGURATION_SHIFT;
	pci_memwin_size = (regval & STRAP_PCI_MEMWIN_SIZE_MASK) >> STRAP_PCI_MEMWIN_SIZE_SHIFT;

#if 0
	printk("%s: regval=%08x, ddr0_strap=%x, ddr1_strap=%x, pci_size=%x\n",
		__FUNCTION__,
		(unsigned int)regval,
		(unsigned int)ddr0,
		(unsigned int)ddr1,
		(unsigned int)pci_memwin_size);
#endif

	switch (ddr0 & 3) {
	case 0:
		/* 256 Mb using 16Mx16 parts, 32MB per part, 4 parts = 128MB total */
		memSize0 = 128 << 20; 
		break;
	case 1:
		/* 512 Mb using 32Mx16 parts, 64MB per part, 4 parts = 256 MB total */
		memSize0 = 256 << 20; 
		break;
	case 2:
		/* 1024 Mb using 64Mx16 parts, 128MB per part, 4 parts = 512 MB total */
		memSize0 = 512 << 20; 
		break;
	case 3:
		/* 1024 Mb using 128Mx8 parts, 128MB per part, 4 parts = 512MB total */
		memSize0 = 512 << 20; 
		break;
	}

	switch (ddr1 & 3) {
	case 0:
		/* 256 Mb using 16Mx16 parts, 32MB per part, 4 parts = 128MB total */
		memSize1 = 128 << 20; 
		break;
	case 1:
		/* 512 Mb using 32Mx16 parts, 64MB per part, 4 parts = 256 MB total */
		memSize1 = 256 << 20; 
		break;
	case 2:
		/* 1024 Mb using 64Mx16 parts, 128MB per part, 4 parts = 512 MB total */
		memSize1 = 512 << 20; 
		break;
	case 3:
		/* 1024 Mb using 128Mx8 parts, 128MB per part, 4 parts = 512MB total */
		memSize1 = 512 << 20; 
		break;
	}
	
	printk("Board Strap Settings: DDR_0 %d MB, DDR_1 %d MB\n",
		(unsigned int)(memSize0 >> 20), (unsigned int)(memSize1 >> 20));

	return memSize0;
}


unsigned long
get_RAM_size(void)
{
	static int once;
	static unsigned long dramSize = 0;

	if (!once) {
		once++;
		dramSize = board_init_once();
#ifdef NEVER
		if (dramSize != DRAM_SIZE) {
			printk("%s: Board strapped at %d MB, using %d MB\n",
				__FUNCTION__, (dramSize>>20), (DRAM_SIZE>>20));
			dramSize = DRAM_SIZE;
		}
#endif
	}

	printk("%s: Returning %d MB\n", __FUNCTION__, (dramSize>>20));
	return dramSize;
}


EXPORT_SYMBOL(get_RAM_size);

