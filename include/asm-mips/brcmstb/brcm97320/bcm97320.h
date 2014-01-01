/***************************************************************************
 *     Copyright (c) 1999-2005, Broadcom Corporation
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
 */

#ifndef BCM97320_H
#define BCM97320_H

#ifndef LANGUAGE_ASSEMBLY
#if __cplusplus
extern "C" {
#endif
#endif

#define BOARD_ID_STR					"BCM97320"

/*************************************************************************/
/* MIPS Clock.                                                           */
/*************************************************************************/
#define CPU_CLOCK_RATE         			248400000		/* Hz */
#define XTALFREQ            			27000000
#define XTALFREQ1						27000000


/*****************************************************************************/
/*                    Physical Memory Map                                    */
/*****************************************************************************/
#define PHYS_DRAM_BASE		0x00000000	/* Dynamic RAM Base */
#define PHYS_ROM_BASE		0x1FC00000	/* ROM */

/* if ROM daughter card is present */
#define PHYS_FLASH1_CS0ROM_BASE   0x1f000000      /* CS1 */
#define PHYS_FLASH2_CS0ROM_BASE   0x1e800000      /* CS2 */
#define PHYS_FLASH_CS0ROM_BASE    PHYS_FLASH2_CS0ROM_BASE

#define PHYS_DOC_BASE           0x1C000000
#define DOC_BASE                (PHYS_DOC_BASE | 0xA0000000)

/* no ROM daughter card present */
#define PHYS_FLASH1_CS0FLASH_BASE 0x1f800000      /* CS0 */
#define PHYS_FLASH2_CS0FLASH_BASE 0x1f000000      /* CS2 */
#define PHYS_FLASH_CS0FLASH_BASE  PHYS_FLASH2_CS0FLASH_BASE

#define PHYS_INPUT_BUFFER_BASE  0x1E000000

#define DRAM_SIZE (64*1024*1024)


/*****************************************************************************/
/* Note that the addresses above are physical addresses and that programs    */
/* have to use converted addresses defined below:                            */
/*****************************************************************************/
#define DRAM_BASE_CACHE		(0x80000000 | PHYS_DRAM_BASE)   /* cached DRAM */
#define DRAM_BASE_NOCACHE	(0xA0000000 | PHYS_DRAM_BASE)   /* uncached DRAM */
#define ROM_BASE_CACHE		(0x80000000 | PHYS_ROM_BASE)
#define ROM_BASE_NOCACHE	(0xA0000000 | PHYS_ROM_BASE)

#define INPUT_BUFFER_BASE_NOCACHE (0xA0000000 | PHYS_INPUT_BUFFER_BASE)

/* 7230 register map */
#include "bcm7320.h"

#ifndef LANGUAGE_ASSEMBLY
#if __cplusplus
}
#endif
#endif

#endif
