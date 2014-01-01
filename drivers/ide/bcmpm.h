/**************************************************************************
 *     Copyright (c) 206 Broadcom Corporation
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
 * Module description
 * brcmpm.h	Header file for Port Multiplier Support in Command mode.
 *
 * Revision History:
 *    11-03-06 tht Initial revision. Version 0.1
 *

 ***************************************************************************/


#ifndef __BCMPM_H__
#define __BCMPM_H__

#include <linux/config.h>
#include <linux/version.h>
#include <linux/ioport.h>
#include <linux/ide.h>
#include <linux/bitops.h>

#include <asm/byteorder.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>


/*
	struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
		unsigned head		: 4;
		unsigned unit		: 1;
		unsigned bit5		: 1;
		unsigned lba		: 1;
		unsigned bit7		: 1;
#elif defined(__BIG_ENDIAN_BITFIELD)
		unsigned bit7		: 1;
		unsigned lba		: 1;
		unsigned bit5		: 1;
		unsigned unit		: 1;
		unsigned head		: 4;
#else
#error "Please fix <asm/byteorder.h>"
#endif
	} b;
*/
//#if defined(__LITTLE_ENDIAN_BITFIELD)
//#define SELECT_DRIVE_A_MASK 0xf7
//#elif defined(__BIG_ENDIAN_BITFIELD)
#define SELECT_DRIVE_A_MASK 0xef  // Bit 4 is 0
//#else
//#error "Please fix <asm/byteorder.h>"
//#endif

extern int bcmpm_detect_pm(ide_hwif_t *hwif);


static inline bcmpm_select_drive(ide_drive_t *drive, u8 head)
{
	ide_hwif_t* hwif = HWIF(drive);
	
	if (bcmpm_detect_pm(hwif)) {
		

		/* This call switches the drive at the PM level */
		if (HWIF(drive)->selectproc)
			HWIF(drive)->selectproc(drive);

		if (drive->select.b.unit) {
			/* Port multiplier only/always addresses drive A */
			drive->select.b.unit = 0;
		}
//printk("%s: HWIF(drive)->OUTB(%x,%x);\n", __FUNCTION__, (head & 0x0f)|drive->select.all, IDE_SELECT_REG);	
	}
	hwif->OUTB((head & 0x0f)|drive->select.all, IDE_SELECT_REG);
}

#endif //__BCMPM_H__
