/*
 *  drivers/mtd/brcmnand/brcmnand_base.c
 *
    Copyright (c) 2005-2006 Broadcom Corporation                 
    
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    File: brcmnand_base.c

    Description: 
    NAND driver for Samsung K9F1G08U0A chip with Broadcom NAND controller.
    The main difference between the Broadcom controller and OneNAND is that the Broadcom
    NAND controller has only a 512B cache (bufferram) regardless of the flash chip, 
    whereas OneNAND has multiple bufferram's to match the page size.
    This complicates this driver quite a bit, because, for large page flash (2K page)
    we have to read in all 4 slides before we know for sure whether a page is bad.

when	who what
-----	---	----
051011	tht	codings derived from onenand_base implementation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/byteorder/generic.h>

#include <asm/io.h>
#include <asm/bug.h>

//#include "bbm.h"

#include "brcmnand.h"

#define PRINTK


#define DRIVER_NAME	"brcmnand"

/*
 * MTD structure for Broadcom NAND
 */
//static struct mtd_info *brcmnand_mtd = NULL;

#if 0
#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { "cmdlinepart", NULL,  };
#endif
#endif

typedef struct brcmnand_chip_Id {
    	uint8 mafId, chipId;
	char* chipIdStr;
	uint32 options;
} brcmnand_chip_Id;

/*
 * List of supported chip
 */
static brcmnand_chip_Id brcmnand_chips[] = {
	{
		.chipId = SAMSUNG_K9F1G08U0A,
		.mafId = FLASHTYPE_SAMSUNG,
		.chipIdStr = "Samsung K9F1G08U0A",
		.options = NAND_USE_FLASH_BBT,
	}
};

#define BRCMNAND_MAX_CHIPS ARRAY_SIZE(brcmnand_chips)


/**
 * brcmnand_oob oob info for 2K page
 */
static struct nand_oobinfo brcmnand_oob_64 = {
	.useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 12,
	.eccpos		= {
		6,7,8,
		20,21,22,
		36,37,38,
		52,53,54
		},
	.oobfree	= {	{2, 3}, {9,7}, 
					{18, 3}, {25, 7},
					{34, 3}, {41, 7},
					{50, 3}, {57, 7},
					}
};

/**
 * brcmnand_oob oob info for 512 page
 */
static struct nand_oobinfo brcmnand_oob_16 = {
	.useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 3,
	.eccpos		= {
		6,7,8
		},
	.oobfree	= { {0, 5}, {9,7} }
};

static const unsigned char ffchars[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 16 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 32 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 48 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 64 */
};

static uint32_t brcmnand_ctrl_read(uint32_t nandCtrlReg) 
{
	volatile unsigned long* pReg = (volatile unsigned long*) (BRCMNAND_CTRL_REGS 
		+ nandCtrlReg - BCHP_NAND_REVISION);

	if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_BLK_WR_PROTECT ||
		(nandCtrlReg & 0x3) != 0) {
		printk(KERN_ERR "brcmnand_ctrl_read: Invalid register value %08x\n", nandCtrlReg);
	}
//printk("BRCMNAND_CTRL_REGS=%08x\n", BRCMNAND_CTRL_REGS);
	return (uint32_t) /*be32_to_cpu*/(*pReg);
}


static void brcmnand_ctrl_write(uint32_t nandCtrlReg, uint32_t val) 
{
	volatile unsigned long* pReg = (volatile unsigned long*) (BRCMNAND_CTRL_REGS 
		+ nandCtrlReg - BCHP_NAND_REVISION);

	if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_BLK_WR_PROTECT ||
		(nandCtrlReg & 0x3) != 0) {
		printk(KERN_ERR "brcmnand_ctrl_read: Invalid register value %08x\n", nandCtrlReg);
	}
	*pReg = (volatile unsigned long)/* cpu_to_be32*/(val);
}


/*
 * BRCMNAND controller always copies the data in 4 byte chunk, and in Big Endian mode
 * from and to the flash.
 * This routine assumes that dest and src are 4 byte aligned, and that len is a multiple of 4
 (Restriction removed)

 * TBD: 4/28/06: Remove restriction on count=512B, but do restrict the read from within a 512B section.
 * Change brcmnand_memcpy32 to be 2 functions, one to-flash, and one from-flash,
 * enforcing reading from/writing to flash on a 4B boundary, but relaxing on the buffer being on 4 byte boundary.
 */
 typedef union {
 	uint32_t u;
 	char c[4];
} u32_t;
static int brcmnand_from_flash_memcpy32(void* dest, const void* flash, int len)
{
	unsigned char* pucDest = (unsigned char*) dest; 
	unsigned char* pucFlash = (unsigned char*) flash; 
	uint32_t* pSrc;
	uint32_t* pDest;
	u32_t u32;
	int odd, i;
	
/*	
	if (unlikely(dest & 0x3)) {
		printk(KERN_ERR "brcmnand_memcpy32 dest=%p not DW aligned\n", dest);
		return -EINVAL;
	}
	if (unlikely(src & 0x3)) {
		printk(KERN_ERR "brcmnand_memcpy32 src=%p not DW aligned\n", src);
		return -EINVAL;
	}
	if (unlikely(len & 0x3)) {
		printk(KERN_ERR "brcmnand_memcpy32 len=%d not DW aligned\n", len);
		return -EINVAL;
	}
*/
	/* Take care of the leading odd bytes */
	odd = (((unsigned long) pucFlash) & 0x3);
	if (odd) {
		/* Guaranteed to be valid, since cache is on 512B boundary */
		pSrc = (uint32_t*) (pucFlash - odd);

		/* pSrc is now aligned on a DW boundary, 
		 * no need to call cpu_to_be32 since we write and read from the same endian
		 */
		u32.u = /*cpu_to_be32 */(*pSrc);

		for (i=0; i<odd; i++) {
			pucDest[i] = u32.c[odd+i];
		}
		pucFlash += 4 - odd; 
		len += 4 - odd;
		pucDest += 4 - odd;
	}

	/* Copy the aligned DWs */
	pSrc = (uint32_t*) pucFlash;
	pDest = (uint32_t*) pucDest;
	for (i=0; i< (len>>2); i++) {
		*pDest++ = cpu_to_be32(*pSrc++);
	}

	/* Take care of the trailing odd bytes */
	odd = (len & 0x3);
	if (odd) {
		pucDest = (unsigned char*) pDest;
	
		/* pSrc is now aligned on a DW boundary */
		u32.u = /*cpu_to_be32 */ (*pSrc);

		for (i=0; i<odd; i++) {
			pucDest[i] = u32.c[odd+i];
		}
	}
	return 0;
}

static int brcmnand_to_flash_memcpy32(void* flash, const void* src, int len)
{
	int i;
	unsigned char* pDest = (unsigned char*) flash;
	unsigned char* pSrc = (unsigned char*) src;
	
	if (unlikely((unsigned long) pDest & 0x3)) {
		printk(KERN_ERR "brcmnand_memcpy32 dest=%p not DW aligned\n", flash);
		return -EINVAL;
	}
	if (unlikely((unsigned long) src & 0x3)) {
		printk(KERN_ERR "brcmnand_memcpy32 src=%p not DW aligned\n", src);
		return -EINVAL;
	}
	if (unlikely(len & 0x3)) {
		printk(KERN_ERR "brcmnand_memcpy32 len=%d not DW aligned\n", len);
		return -EINVAL;
	}
	for (i=0; i< (len>>2); i++) {
		pDest[i] = cpu_to_be32(pSrc[i]);
	}
	return 0;
}


#if 0

/**
 * brcmnand_readw - [BrcmNAND Interface] Read BrcmNAND register
 * @param addr		address to read
 *
 * Read BrcmNAND register
 */
static unsigned short brcmnand_readw(void __iomem *addr)
{
	return readw(addr);
}

/**
 * brcmnand_writew - [BrcmNAND Interface] Write BrcmNAND register with value
 * @param value		value to write
 * @param addr		address to write
 *
 * Write BrcmNAND register with value
 */
static void brcmnand_writew(unsigned short value, void __iomem *addr)
{
	writew(value, addr);
}



/**
 * brcmnand_block_address - [DEFAULT] Get block address
 * @param this		onenand chip data structure
 * @param block		the block
 * @return		translated block address if DDP, otherwise same
 *
 * Setup Start Address 1 Register (F100h)
 */
static int brcmnand_block_address(struct brcmnand_chip *this, int block)
{
	if (this->device_id & ONENAND_DEVICE_IS_DDP) {
		/* Device Flash Core select, NAND Flash Block Address */
		int dfs = 0;

		if (block & this->density_mask)
			dfs = 1;

		return (dfs << ONENAND_DDP_SHIFT) |
			(block & (this->density_mask - 1));
	}

	return block;
}

/**
 * brcmnand_bufferram_address - [DEFAULT] Get bufferram address
 * @param this		onenand chip data structure
 * @param block		the block
 * @return		set DBS value if DDP, otherwise 0
 *
 * Setup Start Address 2 Register (F101h) for DDP
 */
static int brcmnand_bufferram_address(struct brcmnand_chip *this, int block)
{
	if (this->device_id & ONENAND_DEVICE_IS_DDP) {
		/* Device BufferRAM Select */
		int dbs = 0;

		if (block & this->density_mask)
			dbs = 1;

		return (dbs << ONENAND_DDP_SHIFT);
	}

	return 0;
}

/**
 * brcmnand_page_address - [DEFAULT] Get page address
 * @param page		the page address
 * @param sector	the sector address
 * @return		combined page and sector address
 *
 * Setup Start Address 8 Register (F107h)
 */
static int brcmnand_page_address(int page, int sector)
{
	/* Flash Page Address, Flash Sector Address */
	int fpa, fsa;

	fpa = page & ONENAND_FPA_MASK;
	fsa = sector & ONENAND_FSA_MASK;

	return ((fpa << ONENAND_FPA_SHIFT) | fsa);
}

/**
 * brcmnand_buffer_address - [DEFAULT] Get buffer address
 * @param dataram1	DataRAM index
 * @param sectors	the sector address
 * @param count		the number of sectors
 * @return		the start buffer value
 *
 * Setup Start Buffer Register (F200h)
 */
static int brcmnand_buffer_address(int dataram1, int sectors, int count)
{
	int bsa, bsc;

	/* BufferRAM Sector Address */
	bsa = sectors & ONENAND_BSA_MASK;

	if (dataram1)
		bsa |= ONENAND_BSA_DATARAM1;	/* DataRAM1 */
	else
		bsa |= ONENAND_BSA_DATARAM0;	/* DataRAM0 */

	/* BufferRAM Sector Count */
	bsc = count & ONENAND_BSC_MASK;

	return ((bsa << ONENAND_BSA_SHIFT) | bsc);
}



/**
 * brcmnand_command - [DEFAULT] Send command to BrcmNAND device
 * @param mtd		MTD device structure
 * @param cmd		the command to be sent
 * @param addr		offset to read from or write to
 * @param len		number of bytes to read or write
 *
 * Send command to BrcmNAND device. This function is used for middle/large page
 * devices (1KB/2KB Bytes per page)
 */
static int brcmnand_command(struct mtd_info *mtd, int cmd, loff_t addr, size_t len)
{
	struct brcmnand_chip *this = mtd->priv;
	int value, readcmd = 0, block_cmd = 0;
	int block, page;
	/* Now we use page size operation */
	int sectors = 4, count = 4;

	/* Address translation */
	switch (cmd) {
	case ONENAND_CMD_UNLOCK:
	case ONENAND_CMD_LOCK:
	case ONENAND_CMD_LOCK_TIGHT:
		block = -1;
		page = -1;
		break;

	case ONENAND_CMD_ERASE:
	case ONENAND_CMD_BUFFERRAM:
	case ONENAND_CMD_OTP_ACCESS:
		block_cmd = 1;
		block = (int) (addr >> this->erase_shift);
		page = -1;
		break;

	default:
		block = (int) (addr >> this->erase_shift);
		page = (int) (addr >> this->page_shift);
		page &= this->page_mask;
		break;
	}

	/* NOTE: The setting order of the registers is very important! */
	if (cmd == ONENAND_CMD_BUFFERRAM) {
		/* Select DataRAM for DDP */
		value = brcmnand_bufferram_address(this, block);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS2);

		/* Switch to the next data buffer */
		ONENAND_SET_NEXT_BUFFERRAM(this);

		return 0;
	}

	if (block != -1) {
		/* Write 'DFS, FBA' of Flash */
		value = brcmnand_block_address(this, block);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS1);

		if (block_cmd) {
			/* Select DataRAM for DDP */
			value = brcmnand_bufferram_address(this, block);
			this->write_word(value, this->base + ONENAND_REG_START_ADDRESS2);
		}
	}

	if (page != -1) {
		int dataram;

		switch (cmd) {
		case ONENAND_CMD_READ:
		case ONENAND_CMD_READOOB:
			dataram = ONENAND_SET_NEXT_BUFFERRAM(this);
			readcmd = 1;
			break;

		default:
			dataram = ONENAND_CURRENT_BUFFERRAM(this);
			break;
		}

		/* Write 'FPA, FSA' of Flash */
		value = brcmnand_page_address(page, sectors);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS8);

		/* Write 'BSA, BSC' of DataRAM */
		value = brcmnand_buffer_address(dataram, sectors, count);
		this->write_word(value, this->base + ONENAND_REG_START_BUFFER);

		if (readcmd) {
			/* Select DataRAM for DDP */
			value = brcmnand_bufferram_address(this, block);
			this->write_word(value, this->base + ONENAND_REG_START_ADDRESS2);
		}
	}

	/* Interrupt clear */
	this->write_word(ONENAND_INT_CLEAR, this->base + ONENAND_REG_INTERRUPT);

	/* Write command */
	this->write_word(cmd, this->base + ONENAND_REG_COMMAND);

	return 0;
}

#endif

/**
 * brcmnand_wait - [DEFAULT] wait until the command is done
 * @param mtd		MTD device structure
 * @param state		state to select the max. timeout value
 *
 * Wait for command done. This applies to all BrcmNAND command
 * Read can take up to 53, erase up to ?s and program up to 30 clk cycle ()
 * according to general BrcmNAND specs
 */
static int brcmnand_wait(struct mtd_info *mtd, int state, uint32_t* pStatus)
{
	struct brcmnand_chip * this = mtd->priv;
	unsigned long timeout;
	uint32_t ready;

	/* The 20 msec is enough */
	timeout = jiffies + msecs_to_jiffies(20);
	while (time_before(jiffies, timeout)) {
		ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

		if (ready & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
			*pStatus = ready;
			return 0;
		}

		if (state != FL_READING)
			cond_resched();
		//touch_softlockup_watchdog();
	}

	/*
	 * Get here on timeout
	 */
	return -ETIMEDOUT;
}

static int brcmnand_ctrl_ready(struct mtd_info *mtd, int state)
{
	uint32_t status;
	int err;

	err = brcmnand_wait(mtd, state, &status);
	if (!err) {
		return (status & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK);
	}
	return 0;
}

static int brcmnand_cache_is_valid(struct mtd_info* mtd,  int state) 
{
	struct brcmnand_chip * this = mtd->priv;
	unsigned long timeout;
	uint32_t ready;
	
	/* The 20 msec is enough */
	timeout = jiffies + msecs_to_jiffies(100);
	while (time_before(jiffies, timeout)) {
		ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

//printk("ctrl_read(BCHP_NAND_INTFC_STATUS) returns %08x\n", ready);

		if (ready & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
			return 1;
		}
		if (state != FL_READING)
			cond_resched();
	}
	return 0; // TimeOut
}

/*
 * Returns 0 = Complete with no error
 * 1 = Write complete, need BBT
 * err < 0: Other errors
 */
static int brcmnand_wait_for_write(struct mtd_info *mtd)
{
	uint32_t status;
	uint32_t flashStatus = 0;
	int err;

	err = brcmnand_wait(mtd, FL_WRITING, &status);
	if (!err) {
		if (status & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
			flashStatus = status & 0xff;
			return !flashStatus; // 0 = write completes with no errors
		}
		else {
			err = -ETIMEDOUT;
		}
	}
	return 1;
}

static int brcmnand_write_is_complete(struct mtd_info *mtd, int* outp_needBBT)
{
	int err;
	uint32_t status;
	uint32_t flashStatus = 0;

	*outp_needBBT = 1;
	err = brcmnand_wait(mtd, FL_WRITING, &status);
	if (!err) {
		if (status & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
			flashStatus = status & 0xff;
			*outp_needBBT = flashStatus; // 0 = write completes with no errors
			return 1;
		}
		else {
			return 0;
		}
	}
}

static int brcmnand_spare_is_valid(struct mtd_info* mtd,  int state) 
{
	struct brcmnand_chip * this = mtd->priv;
	unsigned long timeout;
	uint32_t ready;
	
	/* The 20 msec is enough */
	timeout = jiffies + msecs_to_jiffies(20);
	while (time_before(jiffies, timeout)) {
		ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

		if ((ready & 0x90000000) == 0x90000000) {
			return 1;
		}
		if (state != FL_READING)
			cond_resched();
	}
}



/**
 * brcmnand_posted_read_cache - [BrcmNAND Interface] Read the 512B cache area
 * Assuming brcmnand_get_device() has been called to obtain exclusive lock
 * @param mtd		MTD data structure
 * @param oobarea	Spare area, pass NULL if not interested
 * @param buffer	the databuffer to put/get data, pass NULL if only spare area is wanted.
 * @param offset	offset to read from or write to, must be 512B aligned.
 *
 * Caller is responsible to pass a buffer that is
 * (1) large enough for 512B for data and optionally an oobarea large enough for 16B.
 * (2) 4-byte aligned.
 *
 * Read the cache area into buffer.  The size of the cache is mtd-->oobblock and is always 512B.

 * TBD: 4/28/06: Remove restriction on count=512B, but do restrict the read from within a 512B section.
 * Change brcmnand_memcpy32 to be 2 functions, and to-flash, and one from-flash,
 * enforcing reading from/writing to flash on a 4B boundary, but lenient on the buffer being on 4 byte boundary.
 */
static int brcmnand_posted_read_cache(struct mtd_info* mtd, 
		void* buffer, unsigned char* oobarea, int offset, int len)
{
	struct brcmnand_chip* this = mtd->priv;
	int page = (offset & ~ (mtd->oobblock - 1));
	int nextStartOffset = page + mtd->oobblock;
	int i;

#if 0 /* THT: Now allow it to begin anywhere inside the 512B block */
	if (page != offset) {
		printk(KERN_ERR "brcmnand_posted_read_cache: offset=%08x is not 512B aligned\n", offset);
		return -EINVAL;
	}
#endif
	/* Make sure that len does not cross the 512B block */
	if ((offset + len) > nextStartOffset) {
		printk(KERN_ERR "brcmnand_posted_read_cache: [offset=%08x,end=%08x] cross %d boundary\n", 
			offset, offset + len, mtd->oobblock);
		return -EINVAL;
	}

	this->ctrl_write(BCHP_NAND_CMD_ADDRESS, page);
	this->ctrl_write(BCHP_NAND_CMD_START, OP_PAGE_READ);

	// Wait until cache is filled up
	if (brcmnand_cache_is_valid(mtd, FL_READING)) {
//printk("PR: copy data from %08x, len=%d, vbase=%08x, offset=%d\n", this->vbase+offset, len, this->vbase, offset);
		if (buffer) {
			brcmnand_from_flash_memcpy32(buffer, this->vbase+offset, len);
		}

		if (oobarea) {
			uint32_t* p32 = (uint32_t*) oobarea;
			
			for (i = 0; i < 4; i++) {
				p32[i] = this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + i*4);
//printk("PR: OOB=%08x, %08x, %08x, %08x\n", p32[0], p32[1], p32[2], p32[3]);
			}
		}
		
		return 0;
	}
	else {
		//Read has timed out or read found bad block. TBD: Find out which is which
		printk(KERN_ERR "brcmnand_cache_is_valid returns 0\n");
		return -ETIMEDOUT;
	}
}

/**
 * brcmnand_posted_read_oob - [BrcmNAND Interface] Read the spare area
 * @param mtd		MTD data structure
 * @param oobarea	Spare area, pass NULL if not interested
 * @param offset	offset to read from or write to
 *
 * This is a little bit faster than brcmnand_posted_read, making this command useful for improving
 * the performance of BBT management.
 * The 512B flash cache is invalidated.
 *
 * Read the cache area into buffer.  The size of the cache is mtd->oobblock and is always 512B,
 * for this version of the BrcmNAND controller.
 */
static int brcmnand_posted_read_oob(struct mtd_info* mtd, 
		unsigned char* oobarea, int offset)
{
	struct brcmnand_chip* this = mtd->priv;
	int page = (offset & ~ (mtd->oobblock - 1));
	int i;

#if 0
	if (page != offset) {
		printk(KERN_ERR "brcmnand_posted_read_oob: offset=%08x is not 512B aligned\n", offset);
		return -EINVAL;
	}
#endif

	this->ctrl_write(BCHP_NAND_CMD_ADDRESS, page);
	this->ctrl_write(BCHP_NAND_CMD_START, OP_SPARE_AREA_READ);

	// Wait until cache is filled up
	if (brcmnand_cache_is_valid(mtd, FL_READING)) {

		if (oobarea) {
			uint32_t* p32 = (uint32_t*) oobarea;
			
			for (i = 0; i < 4; i++) {
				p32[i] = this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + (i<<2));
			}
if (p32[0] != 0xffffffff || p32[1] != 0xffffffff || p32[2] != 0xffffffff || p32[3] != 0xffffffff)
printk("OOB-RD: %08x %08x %08x %08x\n", p32[0], p32[1], p32[2], p32[3]);
		}
		
		return 0;
	}
	else {
		//Read has timed out or read found bad block. TBD: Find out which is which
		return -ETIMEDOUT;
	}
}





/**
 * brcmnand_posted_write - [BrcmNAND Interface] Write a buffer to the flash cache
 * Assuming brcmnand_get_device() has been called to obtain exclusive lock
 *
 * @param mtd		MTD data structure
 * @param buffer	the databuffer to put/get data
 * @param oobarea	Spare area, pass NULL if not interested
 * @param offset	offset to write to, and must be 512B aligned
 *
 * Write to the cache area TBD 4/26/06
 */
static int brcmnand_posted_write_cache(struct mtd_info *mtd,
		const void* buffer, const unsigned char* oobarea, int offset, int len)
{
	struct brcmnand_chip* this = mtd->priv;
	int page = (offset & ~ (mtd->oobblock - 1));
	int nextStartOffset = page +  mtd->oobblock;
	uint32_t* p32;
	int i, needBBT=0;
	int ret;

	PRINTK("brcmnand_posted_write_cache buf=%p, len=%d\n", buffer, len);
#if 0
	if (page != offset) {
		printk(KERN_ERR "brcmnand_posted_write_cache: offset=%08x is not 512B aligned\n", offset);
		return -EINVAL;
	}
#endif
	/* Make sure that len does not cross the 512B block */
	if ((offset + len) > nextStartOffset) {
		printk(KERN_ERR "brcmnand_posted_write_cache: [offset=%08x,end=%08x] cross %d boundary\n", 
			offset, offset + len, mtd->oobblock);
		return -EINVAL;
	}

	this->ctrl_write(BCHP_NAND_CMD_ADDRESS, page);

	if (buffer) {
		brcmnand_to_flash_memcpy32(this->vbase+offset, buffer, len);
	}

	if (oobarea) {
		p32 = (uint32_t*) oobarea;
printk("PW: oob=");
for (i = 0; i < 4; i++) {
printk("%04x ", p32[i]);
}
printk("\n");
	}
	else {
		// Fill with 0xFF if don't want to change OOB
		p32 = (uint32_t*) &ffchars[0];
	}

	for (i = 0; i < 4; i++) {
		this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, p32[i]);
	}

	this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_PAGE);

	// Wait until flash is ready
	if (brcmnand_write_is_complete(mtd, &needBBT)) {
		return 0;
	}
	if (needBBT){
		ret = this->isbad_bbt(mtd, offset, 1);
		printk(KERN_INFO "PW: called is_bad_bbt returning %d\n", ret);
		return -EINVAL;
	}
	//Write has timed out or read found bad block. TBD: Find out which is which
	return -ETIMEDOUT;
	
}

/**
 * brcmnand_posted_write_oob - [BrcmNAND Interface] Write the spare area
 * @param mtd		MTD data structure
 * @param oobarea	Spare area, pass NULL if not interested.  Must be able to 
 *					hold mtd->oobsize (16) bytes.
 * @param offset	offset to write to, and must be 512B aligned
 *
 * Write to the cache area TBD 4/26/06
 */
static int brcmnand_posted_write_oob(struct mtd_info *mtd,
		const unsigned char* oobarea, int offset)
{
	struct brcmnand_chip* this = mtd->priv;
	int page = (offset & ~ (mtd->oobblock - 1));
	uint32_t* p32;
	int i, needBBT=0;

#if 0
	if (page != offset) {
		printk(KERN_ERR "brcmnand_posted_write_cache: offset=%08x is not 512B aligned\n", offset);
		return -EINVAL;
	}
#endif

	this->ctrl_write(BCHP_NAND_CMD_ADDRESS, page);

	// assert oobarea here
	BUG_ON(!oobarea);	
	p32 = (uint32_t*) oobarea;
		
	for (i = 0; i < 4; i++) {
		this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, p32[i]);
printk("OOB-WR: %08x %08x %08x %08x\n", p32[0], p32[1], p32[2], p32[3]);
	}
	
	this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_SPARE_AREA);

	// Wait until flash is ready
	if (brcmnand_write_is_complete(mtd, &needBBT)) {
		return 0;
	}
	if (needBBT){
		// TBD: DO BBT here
	}
	
	return -ETIMEDOUT;
	
}


#if 0

/**
 * brcmnand_check_bufferram - [GENERIC] Check BufferRAM information
 * @param mtd		MTD data structure
 * @param addr		address to check
 * @return		1 if there are valid data, otherwise 0
 *
 * Check bufferram if there is data we required
 */
static int brcmnand_check_bufferram(struct mtd_info *mtd, loff_t addr)
{
	struct brcmnand_chip *this = mtd->priv;
	int block, page, slide = 0;
	int i;

	block = (int) (addr >> this->erase_shift);
	page = (int) (addr >> this->page_shift);
	page &= this->page_mask;
	if (this->pageSize != NFC_FLASHCACHE_SIZE)
		slide = (int) (page >> 9);
	}

	/* Is there valid data? */
	if (this->bufferram.block == block &&
	    this->bufferram.page == page &&
	    this->bufferram.slide == slide &&
	    this->bufferram.valid)
		return 1;

	return 0;
}

/**
 * brcmnand_update_bufferram - [GENERIC] Update BufferRAM information
 * @param mtd		MTD data structure
 * @param addr		address to update
 * @param valid		valid flag
 *
 * Update BufferRAM information
 */
static int brcmnand_update_bufferram(struct mtd_info *mtd, loff_t addr,
		int valid)
{
	struct brcmnand_chip *this = mtd->priv;
	int block, page, slide = 0;
	int i;

	block = (int) (addr >> this->erase_shift);
	page = (int) (addr >> this->page_shift);
	page &= this->page_mask;
	if (this->pageSize != NFC_FLASHCACHE_SIZE)
		slide = (int) (page >> 9);
	}

	/* Invalidate BufferRAM */
	for (i = 0; i < MAX_BUFFERRAM; i++) {
		if (this->bufferram.block == block &&
		    this->bufferram.page == page &&
		    this->bufferram.slide == slide)
		    
			this->bufferram.valid = 0;
	}

	
	this->bufferram.block = block;
	this->bufferram.page = page;
	this->bufferram.slide = slide;
	this->bufferram.valid = valid;

	return 0;
}

#endif


/**
 * brcmnand_read_raw - [GENERIC] Read raw data including oob into buffer
 * @mtd:	MTD device structure
 * @buf:	temporary buffer
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @ooblen:	number of oob data bytes to read
 *
 * Read raw data including oob into buffer
 */
int brcmnand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen)
{
	struct brcmnand_chip *this = mtd->priv;
	int slide, ret;
	int offset = (int) from;

PRINTK("brcmnand_read_raw(from=%08x, len=%08x, offset=%d\n", (unsigned long) from, len, offset);
	if (from & (mtd->oobblock - 1)) {
		printk(KERN_ERR "brcmnand_read_raw: from %08ul is not OOB slide aligned\n", (unsigned long) from);
		return -EINVAL;
	}
	/* len must be multiple of oobblock */
	if (((len / mtd->oobblock) * mtd->oobblock) != len) {
		printk(KERN_ERR "brcmnand_read_raw: len %08x is not multiple of oobblock %d\n", len, mtd->oobblock);
		return -EINVAL;
	}
	/* ooblen must be multiple of oobsize */
	if (((ooblen / mtd->oobsize) * mtd->oobsize) != ooblen) {
		printk(KERN_ERR "brcmnand_read_raw: ooblen %08x is not multiple of oobblock %d\n", ooblen, mtd->oobsize);
		return -EINVAL;
	}
	for (slide = 0; (slide*mtd->oobblock) < len; slide++) {
		int bufOffset = slide*(mtd->oobblock+mtd->oobsize); // offset of buffer
		offset = from + slide*mtd->oobblock;	// offset of data
		ret = brcmnand_posted_read_cache(mtd, &buf[bufOffset], 
			&buf[bufOffset + mtd->oobblock], offset, mtd->oobblock);
		if (ret) {
			printk(KERN_ERR "brcmnand_read_raw failed at offset = %08x, bufOffset=%08x\n", offset, bufOffset);
			return ret;
		}
	}
	return ret;
	
}



/**
 * brcmnand_get_device - [GENERIC] Get chip for selected access
 * @param mtd		MTD device structure
 * @param new_state	the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int brcmnand_get_device(struct mtd_info *mtd, int new_state)
{
	struct brcmnand_chip *this = mtd->priv;
	DECLARE_WAITQUEUE(wait, current);

	/*
	 * Grab the lock and see if the device is available
	 */
	while (1) {
		spin_lock(&this->chip_lock);
#if 0
		if (atomic_dec(this->semCount)) == 0) {
			// Write 1 to Semaphore register TBD
			this->ctrl_write(BCHP_NAND_SEMAPHORE, 1);
		}
		else { /* In Use by another thread */
			atomic_inc(this->semCount);
			continue;
		}
#endif
		if (this->state == FL_READY) {
			this->state = new_state;
			spin_unlock(&this->chip_lock);
			break;
		}
		if (new_state == FL_PM_SUSPENDED) {
			spin_unlock(&this->chip_lock);
			return (this->state == FL_PM_SUSPENDED) ? 0 : -EAGAIN;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&this->wq, &wait);
		spin_unlock(&this->chip_lock);
		schedule();
		remove_wait_queue(&this->wq, &wait);
	}

	return 0;
}

/**
 * brcmnand_release_device - [GENERIC] release chip
 * @param mtd		MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void brcmnand_release_device(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;

	/* Release the chip */
	spin_lock(&this->chip_lock);
	this->state = FL_READY;
	wake_up(&this->wq);
	spin_unlock(&this->chip_lock);
}

/**
 * brcmnand_read_ecc - [MTD Interface] Read data with ECC
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 * @param oob_buf	filesystem supplied oob data buffer
 * @param oobsel	oob selection structure
 *
 * BrcmNAND read with ECC
 * returns 0 on success, -errno on error.
 */
static int brcmnand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf,
	u_char *oob_buf, struct nand_oobinfo *oobsel)
{
	//struct brcmnand_chip *this = mtd->priv;
	int bytesread = 0;
	int page; 
	int thislen;
	int ret = 0;
	unsigned char* oobarea;
	//unsigned char bcache[512];

	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_read_ecc: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);
	*retlen = 0;
	oobarea = oob_buf;

	/* Do not allow reads past end of device */
	if (unlikely(from + len) > mtd->size) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	/* THT: TBD: Move it inside while loop: It would be less efficient, but
	 * would also give less response time to multiple threads
	 */
PRINTK("brcmnand_read_ecc: get device\n");
	brcmnand_get_device(mtd, FL_READING);

	/* TODO handling oob */

	while (bytesread < len) {
		thislen = min_t(int, mtd->oobblock, len - bytesread);


		/* page marks the start of the 512B block encompassing [from, thislen] */
		page = ((unsigned long) from & ~ (mtd->oobblock - 1));

		/* If not at start of page, read up to the next start of page */
		if (page != from)
			thislen = mtd->oobblock -(from - page);
PRINTK("brcmnand_read_ecc: thislen=%d, from=%08x, page=%08x, oobblock=%d\n", 
	thislen, (unsigned long) from, page, mtd->oobblock);
#if 0
//THT: This is done in HW in our platform
		if (!brcmnand_check_cache(mtd, from)) {
			this->command(mtd, ONENAND_CMD_READ, from, mtd->oobblock);

			ret = this->wait(mtd, FL_READING);
			/* First copy data and check return value for ECC handling */
			brcmnand_update_cache(mtd, from, 1);
		}

		this->read_cache(mtd, ONENAND_DATARAM, buf, column, thislen);
#endif
//-------------------- Restart here 7/13/06 --------------------------
		/* Posted read till end of column */
		/* THT Check buswidth here */
		ret = brcmnand_posted_read_cache(mtd, &buf[bytesread], oobarea, from, thislen);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: read failed = %d\n", ret);
			// Do BTT management here
			goto out;
		}
		bytesread += thislen;

		if (bytesread >= len)
			break;

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: read failed = %d\n", ret);
			goto out;
		}

		from += thislen;
		buf += thislen;
		if (oobarea)
			oobarea += mtd->oobsize;
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	/*
	 * Return success, if no ECC failures, else -EBADMSG
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EBADMSG
	 */
	*retlen = bytesread;
	return ret;
}

/**
 * brcmnand_read - [MTD Interface] MTD compability function for brcmnand_read_ecc
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 *
 * This function simply calls brcmnand_read_ecc with oob buffer and oobsel = NULL
*/
static int brcmnand_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	return brcmnand_read_ecc(mtd, from, len, retlen, buf, NULL, NULL);
}

/**
 * brcmnand_read_oob - [MTD Interface] BrcmNAND read out-of-band
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 *
 * BrcmNAND read out-of-band data from the spare area
 */

//----------------------- THT 7/14/06
static int brcmnand_read_oob(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct brcmnand_chip *this = mtd->priv;
	int oobread = 0, thislen;
	unsigned int page;
	int ret = 0;

//	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_read_oob: from = 0x%08ul, len = %i\n", 
//		(unsigned long) from, (int) len);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow reads past end of device */
	if (unlikely((from + len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_oob: Attempt read beyond end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_READING);

	/* page marks the start of the 512B block encompassing [from, thislen] */
	page = ((unsigned long) from & ~ (mtd->oobblock - 1));

	/* Make sure we don't go over bound */
	while (oobread < len) {
		thislen = min_t(int, mtd->oobsize, len - oobread);
		ret = brcmnand_posted_read_oob(mtd, &this->page_buf[mtd->oobblock], page);
		if (ret) {
			printk(KERN_ERR "brcmnand_posted_read_oob returns %d, thislen = %d\n", ret, thislen);
		}
		
		// Just copy up to thislen
		memcpy(&buf[oobread], &this->page_buf[mtd->oobblock], thislen);
		oobread += thislen;
		if (oobread >= len) {
			break;
		}

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_oob: read failed = %d\n", ret);
			goto out;
		}

		buf += thislen;

		/* Read more? */
		if (oobread < len) {
			/* Page size */
			page += mtd->oobblock; // THT: SHould this be the 512B sliding window instead?  Yes it is.
		}
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	*retlen = oobread;
	return ret;
}

#ifdef CONFIG_MTD_BRCMNAND_VERIFY_WRITE
/**
 * brcmnand_verify_oob - [GENERIC] verify the oob contents after a write
 * @param mtd		MTD device structure
 * @param buf		the databuffer to verify
 * @param to			offset to read from
 * @param len		number of bytes to read and compare
 *
 */
static int brcmnand_verify_oob(struct mtd_info *mtd, const u_char *buf, loff_t to, int len)
{
	struct brcmnand_chip *this = mtd->priv;
	char *readp = this->page_buf;
	int status, i;

	brcmnand_posted_read_oob(mtd, &readp[mtd->oobblock], to);
	
	for(i = 0; i < len; i++)
		if (buf[i] != 0xFF && buf[i] != readp[i])
			return -EBADMSG;

	return 0;
}

/**
 * brcmnand_verify_page - [GENERIC] verify the chip contents after a write
 * @param mtd		MTD device structure
 * @param buf		the databuffer to verify
 *
 * Check DataRAM area directly
 */
static int brcmnand_verify_page(struct mtd_info *mtd, u_char *buf, loff_t addr)
{
	struct brcmnand_chip *this = mtd->priv;
	char *readp = this->page_buf;
	void __iomem *dataram0, *dataram1;
	int ret = 0;

	ret = brcmnand_posted_read_cache(mtd, readp, addr);
	if (ret)
		return ret;
	
	if (memcmp(readp, buf, mtd->oobblock))
		return -EBADMSG;

	return 0;
}
#else
#define brcmnand_verify_page(...)	(0)
#define brcmnand_verify_oob(...)		(0)
#endif

#define NOTALIGNED(x)	((x & (mtd->oobblock - 1)) != 0)

/**
 * brcmnand_write_ecc - [MTD Interface] BrcmNAND write with ECC
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 * @param eccbuf	filesystem supplied oob data buffer
 * @param oobsel	oob selection structure
 *
 * BrcmNAND write with ECC
 */
static int brcmnand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf,
	u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	//struct brcmnand_chip *this = mtd->priv;
	int written = 0;
	int ret = 0;

	//DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_write_ecc: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow writes past end of device */
	if (unlikely((to + len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_ecc: Attempt write to past end of device\n");
		return -EINVAL;
	}

	/* Reject writes, which are not page aligned */
        if (unlikely(NOTALIGNED(to)) || unlikely(NOTALIGNED(len))) {
                DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_ecc: Attempt to write not page aligned data\n");
                return -EINVAL;
        }

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_WRITING);

	/* Loop until all data are written */
	while (written < len) {
		int thislen = min_t(int, mtd->oobblock, len - written);

		ret = brcmnand_posted_write_cache(mtd, buf, eccbuf, to, thislen);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_ecc: brcmnand_posted_write_cache failed %d\n", ret);
			goto out;
		}

		written += thislen;

		/* Only check verify write turn on */
		ret = brcmnand_verify_page(mtd, (u_char *) buf, to);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_ecc: verify failed %d\n", ret);
			goto out;
		}

		if (written >= len)
			break;

		to += thislen;
		buf += thislen;
		eccbuf += mtd->oobsize;
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	*retlen = written;

	//DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_write_ecc returns %d\n", ret);

	return ret;
}

/**
 * brcmnand_write - [MTD Interface] compability function for brcmnand_write_ecc
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 *
 * This function simply calls brcmnand_write_ecc
 * with oob buffer and oobsel = NULL
 */
static int brcmnand_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	return brcmnand_write_ecc(mtd, to, len, retlen, buf, NULL, NULL);
}

/**
 * brcmnand_write_oob - [MTD Interface] BrcmNAND write out-of-band
 * @param mtd		MTD device structure
 * @param to			offset to write to (of the page, not the oob).
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 *
 * BrcmNAND write out-of-band
 */
static int brcmnand_write_oob(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	//struct brcmnand_chip *this = mtd->priv;
	int ret = 0;
	int written = 0;
	unsigned long page;
	//unsigned char oobbuf[16];

	DEBUG(MTD_DEBUG_LEVEL1, "brcmnand_write_oob: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow writes past end of device */
	if (unlikely(to >= mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_oob: Attempt write to past end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_WRITING);

	/* Loop until all data write */
	while (written < len) {
		int thislen = min_t(int, mtd->oobsize, len - written);

		page = (unsigned long) to & (~ (mtd->oobblock - 1));

		/* Pad with FF
		if (thislen < mtd->oobsize) {
			}
			*/

		ret = brcmnand_posted_write_oob(mtd, buf, page);

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_oob: write failed = %d\n", ret);
			goto out;
		}

		ret = brcmnand_verify_oob(mtd, buf, to, thislen);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_oob: verify failed %d\n", ret);
			goto out;
		}

		written += mtd->oobsize;

		if (written >= len)
			break;

		to = page + mtd->oobblock; // Advance offset by a page (512 bytes
		buf += mtd->oobsize;		 // Advance oob buffer by 16 Bytes.
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	*retlen = written;

	return ret;
}

/**
 * brcmnand_writev_ecc - [MTD Interface] write with iovec with ecc
 * @param mtd		MTD device structure
 * @param vecs		the iovectors to write
 * @param count		number of vectors
 * @param to		offset to write to
 * @param retlen	pointer to variable to store the number of written bytes
 * @param eccbuf	filesystem supplied oob data buffer ** THT: Not used **
 * @param oobsel	oob selection structure  ** THT: Not used **
 *
 * BrcmNAND write with iovec with ecc
 */
static int brcmnand_writev_ecc(struct mtd_info *mtd, const struct kvec *vecs,
	unsigned long count, loff_t to, size_t *retlen,
	u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	struct brcmnand_chip *this = mtd->priv;
	unsigned char *pbuf;
	size_t total_len, len;
	int i, written = 0;
	int ret = 0;

	/* Preset written len for early exit */
	*retlen = 0;

	/* Calculate total length of data */
	total_len = 0;
	for (i = 0; i < count; i++)
		total_len += vecs[i].iov_len;

	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_writev_ecc: to = 0x%08x, len = %i, count = %ld\n", (unsigned int) to, (unsigned int) total_len, count);

	/* Do not allow write past end of the device */
	if (unlikely((to + total_len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_writev_ecc: Attempted write past end of device\n");
		return -EINVAL;
	}

	/* Reject writes, which are not page aligned */
        if (unlikely(NOTALIGNED(to)) || unlikely(NOTALIGNED(total_len))) {
                DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_writev_ecc: Attempt to write not page aligned data\n");
                return -EINVAL;
        }

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_WRITING);

	/* TODO handling oob */

	/* Loop until all keve's data has been written */
	len = 0;
	while (count) {
		pbuf = this->page_buf;
		/*
		 * If the given tuple is >= pagesize then
		 * write it out from the iov
		 */
		if ((vecs->iov_len - len) >= mtd->oobblock) {
			pbuf = vecs->iov_base + len;

			len += mtd->oobblock;

			/* Check, if we have to switch to the next tuple */
			if (len >= (int) vecs->iov_len) {
				vecs++;
				len = 0;
				count--;
			}
		} else {
			int cnt = 0, thislen;
			while (cnt < mtd->oobblock) {
				thislen = min_t(int, mtd->oobblock - cnt, vecs->iov_len - len);
				memcpy(this->page_buf + cnt, vecs->iov_base + len, thislen);
				cnt += thislen;
				len += thislen;

				/* Check, if we have to switch to the next tuple */
				if (len >= (int) vecs->iov_len) {
					vecs++;
					len = 0;
					count--;
				}
			}
		}
/*
		this->command(mtd, ONENAND_CMD_BUFFERRAM, to, mtd->oobblock);

		this->write_bufferram(mtd, ONENAND_DATARAM, pbuf, 0, mtd->oobblock);
		this->write_bufferram(mtd, ONENAND_SPARERAM, ffchars, 0, mtd->oobsize);

		this->command(mtd, ONENAND_CMD_PROG, to, mtd->oobblock);

		brcmnand_update_bufferram(mtd, to, 1);

		ret = this->wait(mtd, FL_WRITING);
*/
		ret = brcmnand_posted_write_cache(mtd, pbuf, ffchars, to, mtd->oobblock);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_writev_ecc: write failed %d\n", ret);
			goto out;
		}


		/* Only check verify write turn on */
		ret = brcmnand_verify_page(mtd, (u_char *) pbuf, to);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_writev_ecc: verify failed %d\n", ret);
			goto out;
		}

		written += mtd->oobblock;

		to += mtd->oobblock;
	}

out:
	/* Deselect and wakt up anyone waiting on the device */
	brcmnand_release_device(mtd);

	*retlen = written;

	return 0;
}

/**
 * brcmnand_writev - [MTD Interface] compabilty function for brcmnand_writev_ecc
 * @param mtd		MTD device structure
 * @param vecs		the iovectors to write
 * @param count		number of vectors
 * @param to		offset to write to
 * @param retlen	pointer to variable to store the number of written bytes
 *
 * BrcmNAND write with kvec. This just calls the ecc function
 */
static int brcmnand_writev(struct mtd_info *mtd, const struct kvec *vecs,
	unsigned long count, loff_t to, size_t *retlen)
{
	return brcmnand_writev_ecc(mtd, vecs, count, to, retlen, NULL, NULL);
}

/**
 * brcmnand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @param mtd		MTD device structure
 * @param ofs		offset from device start
 * @param getchip	0, if the chip is already selected
 * @param allowbbt	1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int brcmnand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int getchip, int allowbbt)
{
	struct brcmnand_chip *this = mtd->priv;
	//struct bbm_info *bbm = this->bbm;

	/* Return info from the table */
	return this->isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * brcmnand_erase_bbt - [Private] erase block(s)
 * @param mtd		MTD device structure
 * @param instr		erase instruction
 * @allowBBT			allow erase of BBT
 *
 * Erase one ore more blocks
 */
static int brcmnand_erase_bbt(struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
	struct brcmnand_chip *this = mtd->priv;
	unsigned int block_size;
	loff_t addr;
	int len;
	int ret = 0;
	int needBBT;

	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_erase: start = 0x%08x, len = %08x\n", (unsigned int) instr->addr, (unsigned int) instr->len);

	block_size = (1 << this->erase_shift);


	/* Start address must align on block boundary */
	if (unlikely(instr->addr & (block_size - 1))) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_erase: Unaligned address\n");
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (unlikely(instr->len & (block_size - 1))) {
		DEBUG(MTD_DEBUG_LEVEL0, 
"brcmnand_erase: Length not block aligned, len=%08x, blocksize=%08x, chip->blkSize=%08x, chip->erase=%d\n",
instr->len, block_size, this->blockSize, this->erase_shift);
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if (unlikely((instr->len + instr->addr) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_erase: Erase past end of device\n");
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_ERASING);

	/* Loop throught the pages */
	len = instr->len;
	addr = instr->addr;

	instr->state = MTD_ERASING;

	while (len) {

		/* Check if we have a bad block, we do not erase bad blocks */
		if (brcmnand_block_checkbad(mtd, addr, 0, allowbbt)) {
			printk (KERN_WARNING "brcmnand_erase: attempt to erase a bad block at addr 0x%08x\n", (unsigned int) addr);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}

		//this->command(mtd, ONENAND_CMD_ERASE, addr, block_size);

		this->ctrl_write(BCHP_NAND_CMD_ADDRESS, addr);
	
		this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCK_ERASE);

		// Wait until flash is ready
		ret = brcmnand_write_is_complete(mtd, &needBBT);
		
		/* Check, if it is write protected: TBD */
		if (needBBT) {
			if (ret == -EPERM)
				DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_erase: Device is write protected!!!\n");
			else
				DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_erase: Failed erase, block %d\n", (unsigned) (addr >> this->erase_shift));
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = addr;
			goto erase_exit;
		}

		len -= block_size;
		addr += block_size;
	}

	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	return ret;
}


/**
 * brcmnand_erase - [MTD Interface] erase block(s)
 * @param mtd		MTD device structure
 * @param instr		erase instruction
 *
 * Erase one ore more blocks
 */
static int brcmnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return brcmnand_erase_bbt(mtd, instr, 0); // Do not allow erase of BBT.
}

/**
 * brcmnand_sync - [MTD Interface] sync
 * @param mtd		MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void brcmnand_sync(struct mtd_info *mtd)
{
	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_sync: called\n");

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_SYNCING);

	/* Release it and go back */
	brcmnand_release_device(mtd);
}


/**
 * brcmnand_block_isbad - [MTD Interface] Check whether the block at the given offset is bad
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 *
 * Check whether the block is bad
 */
static int brcmnand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	/* Check for invalid offset */
	if (ofs > mtd->size)
		return -EINVAL;

	return brcmnand_block_checkbad(mtd, ofs, 1, 0);
}

/**
 * brcmnand_default_block_markbad - [DEFAULT] mark a block bad
 * @param mtd		MTD device structure
 * @param ofs		offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
 */
static int brcmnand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct brcmnand_chip *this = mtd->priv;
	//struct bbm_info *bbm = this->bbm;
	u_char buf[2] = {0, 0};
	size_t retlen;
	int block;

	/* Get block number */
	block = ((int) ofs) >> this->erase_shift;
      if (this->bbt)
                this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

        /* We write two bytes, so we dont have to mess with 16 bit access */
        ofs += mtd->oobsize + (this->badblockpos & ~0x01);
        return mtd->write_oob(mtd, ofs , 2, &retlen, buf);
}

/**
 * brcmnand_block_markbad - [MTD Interface] Mark the block at the given offset as bad
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 *
 * Mark the block as bad
 */
static int brcmnand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct brcmnand_chip *this = mtd->priv;
	int ret;

	ret = brcmnand_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing */
		if (ret > 0)
			return 0;
		return ret;
	}

	return this->block_markbad(mtd, ofs);
}

/**
 * brcmnand_unlock - [MTD Interface] Unlock block(s)
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 * @param len		number of bytes to unlock
 *
 * Unlock one or more blocks
 */
static int brcmnand_unlock(struct mtd_info *mtd, loff_t llofs, size_t len)
{
	struct brcmnand_chip *this = mtd->priv;
	int status;
	unsigned long blkAddr, ofs = (unsigned long) llofs;

PRINTK("-->brcmnand_unlock: ofs=%ul, len=%08x, blocksize=%dK\n", (unsigned long) ofs, len, this->blockSize>>10);

#if 0
	start = ofs >> this->erase_shift;
	end = len >> this->erase_shift;

	/* Continuous lock scheme */
	//if (this->options & ONENAND_CONT_LOCK) 
	{
		/* Set start block address */
		this->ctrl_write(BCHP_NAND_CMD_ADDRESS, start);
		/* Set end block address */
		this->ctrl_write(BCHP_NAND_CMD_END_ADDRESS, end - 1);
		/* Write unlock command */
		this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCKS_UNLOCK);

		/* There's no return value */
		this->wait(mtd, FL_UNLOCKING);

		/* Sanity check */
#if 0
		while (this->read_word(this->base + ONENAND_REG_CTRL_STATUS)
		    & ONENAND_CTRL_ONGO)
			continue;

		/* Check lock status */
		status = this->read_word(this->base + ONENAND_REG_WP_STATUS);
		if (!(status & ONENAND_WP_US))
			printk(KERN_ERR "wp status = 0x%x\n", status);
#endif
		return 0;
	}

#endif /* Continous lock */

	/* Block lock scheme */
	for (blkAddr = ofs; blkAddr < ofs + len; blkAddr += this->blockSize) {
		/* Set block address */
		//value = brcmnand_block_address(this, block); // THT: We don't have DDP support
		this->ctrl_write(BCHP_NAND_CMD_ADDRESS, blkAddr); 
		/* Set end block address */
		this->ctrl_write(BCHP_NAND_CMD_END_ADDRESS, blkAddr + this->blockSize - 1);
		/* Write unlock command */
		this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCKS_UNLOCK);


		/* There's no return value */
		this->wait(mtd, FL_UNLOCKING, &status);

		if (status & 0xff)  
			printk(KERN_ERR "block = %ul, wp status = 0x%x\n", blkAddr, status);

		/* Sanity check */
#if 0
		while (this->read_word(this->base + ONENAND_REG_CTRL_STATUS)
		    & ONENAND_CTRL_ONGO)
			continue;
#endif

		/* Check lock status */
		this->ctrl_write(BCHP_NAND_CMD_ADDRESS, blkAddr); 
		this->ctrl_write(BCHP_NAND_CMD_START, OP_READ_BLOCKS_LOCK_STATUS);
		status = this->ctrl_read(BCHP_NAND_BLOCK_LOCK_STATUS);
		//status = this->read_word(this->base + ONENAND_REG_WP_STATUS);

	}

	return 0;
}


/**
 * brcmnand_print_device_info - Print device ID
 * @param device        device ID
 *
 * Print device ID
 */
static void brcmnand_print_device_info(brcmnand_chip_Id* chipId, unsigned long flashSize)
{
	printk(KERN_INFO "BrcmNAND mfg %x %x %s %dMB\n",
                chipId->mafId, chipId->chipId, chipId->chipIdStr, (int) (flashSize >>20));

}

#if 0

static const struct brcmnand_manufacturers brcmnand_manuf_ids[] = {
        {ONENAND_MFR_SAMSUNG, "Samsung"},
};

/**
 * brcmnand_check_maf - Check manufacturer ID
 * @param manuf         manufacturer ID
 *
 * Check manufacturer ID
 */
static int brcmnand_check_maf(int manuf)
{
	int size = ARRAY_SIZE(brcmnand_manuf_ids);
	char *name;
        int i;

        for (i = 0; i < size; i++)
                if (manuf == brcmnand_manuf_ids[i].id)
                        break;

	if (i < size)
		name = brcmnand_manuf_ids[i].name;
	else
		name = "Unknown";

        printk(KERN_DEBUG "BrcmNAND Manufacturer: %s (0x%0x)\n", name, manuf);

        return (i == size);
}

#endif

/*
 * bit 31: 	1 = OTP read-only
 * 30: 		Page Size: 0 = PG_SIZE_512, 1 = PG_SIZE_2KB
 * 28-29: 	Block size: 3=512K, 1 = 128K, 0 = 16K, 2 = 8K
 * 27:		Reserved
 * 24-26:	Device_Size
 *			0:	4MB
 *			1:	8MB
 *			2: 	16MB
 *			3:	32MB
 *			4:	64MB
 *			5:	128MB
 *			6: 	256MB
 * 23:		Dev_Width 0 = Byte8, 1 = Word16
 * 22-19: 	Reserved
 * 18:16:	Full Address Bytes
 * 15		Reserved
 * 14:12	Col_Adr_Bytes
 * 11:		Reserved
 * 10-08	Blk_Adr_Bytes
 * 07-00	Reserved
 */
 
void
brcmnand_decode_config(struct brcmnand_chip* chip, uint32_t nand_config)
{
	unsigned int chipSizeShift;
	
	//chip->chipSize = (nand_config & 0x07000000) >> (24 - 20);

	switch ((nand_config & 0x30000000) >> 28) {
		case 3:
			chip->blockSize = 512 << 10;
			break;
		case 2:
			chip->blockSize = 8 << 10;
			break;
		/* FIXME: ******   This is different from RDB */
		case 0:	
			chip->blockSize = 128 << 10;
			break;
		case 1:
			chip->blockSize = 16 << 10;
			break;
	}
	chip->erase_shift = ffs(chip->blockSize) - 1;

	switch((nand_config & 0x40000000) >> 30) {
		case 0:
			chip->pageSize = 512;
			break;
		case 1:
			chip->pageSize = 2048;
			break;
	}
	chip->page_shift = ffs(chip->pageSize) - 1;
	chip->page_mask = (1 << chip->page_shift) - 1;

	chipSizeShift = (nand_config & 0x07000000) >> 24;
	chip->chipSize = (4 << 20) << chipSizeShift;

	chip->busWidth = 1 + ((nand_config & 0x00400000) >> 23);

	printk(KERN_INFO "NAND Config: Reg=%08x, chipSize=%dMB, blockSize=%dK, erase_shift=%x\n",
		nand_config, chip->chipSize >> 20, chip->blockSize >> 10, chip->erase_shift);
	printk(KERN_INFO "busWidth=%d, pageSize=%dB, page_shift=%d, page_mask=%08x\n", 
		chip->busWidth, chip->pageSize, chip->page_shift , chip->page_mask);
}


/**
 * brcmnand_probe - [BrcmNAND Interface] Probe the BrcmNAND device
 * @param mtd		MTD device structure
 *
 * BrcmNAND detection method:
 *   Compare the the values from command with ones from register
 */
static int brcmnand_probe(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;
	unsigned char brcmnand_maf_id, brcmnand_dev_id;
	uint32 dev_id;
	uint32_t nand_config;
	int version_id;
	//int density;
	int i;

PRINTK("-->brcmnand_probe: this=%p, this->ctrl_read=%p\n", this, this->ctrl_read);

	/* Send the command for reading device ID from controller */
	dev_id = this->ctrl_read(BCHP_NAND_FLASH_DEVICE_ID);

PRINTK("brcmnand_probe: dev_id=%x\n", dev_id);
#if 1
	/* Read manufacturer and device IDs from Controller */
	brcmnand_maf_id = (dev_id >> 24) & 0xff;
	brcmnand_dev_id = (dev_id >> 16) & 0xff;

	/* Check manufacturer ID */
	//if (brcmnand_check_maf(brcmnand_maf_id))
	//	return -ENXIO;
#endif

	/* Look up in our table for infos on device */
	for (i=0; i < BRCMNAND_MAX_CHIPS; i++) {
		if (brcmnand_dev_id == brcmnand_chips[i].chipId 
			&& brcmnand_maf_id == brcmnand_chips[i].mafId)
			break;
	}

	if (i >= BRCMNAND_MAX_CHIPS) {
		printk(KERN_ERR "DevId %08x not supported\n", (unsigned int) dev_id);
		return (-EINVAL);
	}

	nand_config = this->ctrl_read(BCHP_NAND_CONFIG);

	brcmnand_decode_config(this, nand_config);


	/* Flash device information */
	brcmnand_print_device_info(&brcmnand_chips[i], this->chipSize);
	this->device_id = dev_id;
	this->options = brcmnand_chips[i].options;

#if 0
	density = this->chipSize >> 20;
	this->chipsize = (16 << density) << 20;
	/* Set density mask. it is used for DDP */
	this->density_mask = (1 << (density + 6));
#endif

	/* BrcmNAND page size & block size */
	/* The data buffer size is equal to page size */
	mtd->oobblock = 512; 					// tht -512 cache block
	mtd->oobsize = mtd->oobblock >> 5;	// tht - 16 byte OOB
	mtd->erasesize = this->blockSize;

	/* Fix me: When we have both a NOR and NAND flash on board */

	mtd->size = this->chipSize;

	/* Set up base, based on flash size, for now hardcoded to the 1Gbit flash chip */
	if (this->chipSize == (256 << 20)) {
		this->vbase = (void*) KSEG1ADDR(0x12000000);
	} else {
		/* We know that flash endAddr is 0x2000_0000 */
		this->vbase = (void*) KSEG1ADDR(0x20000000 - this->chipSize);
	}

	/* Version ID */
	version_id = this->ctrl_read(BCHP_NAND_REVISION);
	printk(KERN_INFO "BrcmNAND version = 0x%04x %dMB @%p\n", 
		version_id, this->chipSize>>20, this->vbase);

PRINTK("<-- brcmnand_probe\n");
	return 0;
}

/**
 * brcmnand_suspend - [MTD Interface] Suspend the BrcmNAND flash
 * @param mtd		MTD device structure
 */
static int brcmnand_suspend(struct mtd_info *mtd)
{
	return brcmnand_get_device(mtd, FL_PM_SUSPENDED);
}

/**
 * brcmnand_resume - [MTD Interface] Resume the BrcmNAND flash
 * @param mtd		MTD device structure
 */
static void brcmnand_resume(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;

	if (this->state == FL_PM_SUSPENDED)
		brcmnand_release_device(mtd);
	else
		printk(KERN_ERR "resume() called for the chip which is not"
				"in suspended state\n");
}

/**
 * brcmnand_scan - [BrcmNAND Interface] Scan for the BrcmNAND device
 * @param mtd		MTD device structure
 * @param maxchips	Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 *
 * THT: For now, machips should always be 1.
 */
int brcmnand_scan(struct mtd_info *mtd , int maxchips )
{
	struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
	int err;

	if (!this->ctrl_read)
		this->ctrl_read = brcmnand_ctrl_read;
	if (!this->ctrl_write)
		this->ctrl_write = brcmnand_ctrl_write;

/*
	if (!this->command)
		this->command = brcmnand_command;
*/
	if (!this->wait)
		this->wait = brcmnand_wait;

#if 0
	if (!this->read_bufferram)
		this->read_bufferram = brcmnand_read_bufferram;
	if (!this->write_bufferram)
		this->write_bufferram = brcmnand_write_bufferram;
#endif


	if (!this->block_markbad)
		this->block_markbad = brcmnand_default_block_markbad;
	if (!this->scan_bbt)
		this->scan_bbt = brcmnand_default_bbt;
	if (!this->erase_bbt)
		this->erase_bbt = brcmnand_erase_bbt;

PRINTK("brcmnand_scan: Calling brcmnand_probe\n");
	if (brcmnand_probe(mtd))
		return -ENXIO;

	this->bbt_erase_shift =  ffs(mtd->erasesize) - 1;

		/* Calculate the address shift from the page size */	
		this->page_shift = ffs(mtd->oobblock) - 1;
		this->bbt_erase_shift = ffs(mtd->erasesize) - 1;
		this->chip_shift = ffs(this->chipSize) - 1;

		/* Set the bad block position */
		this->badblockpos = mtd->oobblock > 512 ? 
			NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

#if 0
/* THT: FOr now */
		/* Get chip options, preserve non chip based options */
		this->options &= ~NAND_CHIPOPTIONS_MSK;
		this->options |= nand_flash_ids[i].options & NAND_CHIPOPTIONS_MSK;
		/* Set this as a default. Board drivers can override it, if neccecary */
		this->options |= NAND_NO_AUTOINCR;
		/* Check if this is a not a samsung device. Do not clear the options
		 * for chips which are not having an extended id.
		 */	
		if (nand_maf_id != NAND_MFR_SAMSUNG && !nand_flash_ids[i].pagesize)
			this->options &= ~NAND_SAMSUNG_LP_OPTIONS;
#endif
#if 0
		/* Check for AND chips with 4 page planes */
		if (this->options & NAND_4PAGE_ARRAY)
			this->erase_cmd = multi_erase_cmd;
		else
			this->erase_cmd = single_erase_cmd;
#endif
#if 0
		/* Do not replace user supplied command function ! */
		if (mtd->oobblock > 512 && this->cmdfunc == nand_command)
			this->cmdfunc = nand_command_lp;
#endif

PRINTK("brcmnand_scan: Done brcmnand_probe\n");
#if 0
//THT: We don't have this feature
	/* Set Sync. Burst Read after probing */
	if (this->mmcontrol) {
		printk(KERN_INFO "BrcmNAND Sync. Burst Read support\n");
		this->read_bufferram = brcmnand_sync_read_bufferram;
	}
#endif

	/* Allocate buffers, if necessary */
	if (!this->page_buf) {
		size_t len;
		len = mtd->oobblock  + mtd->oobsize;  /* FIX ME: THT: May be we don't need oobsize */
		this->page_buf = kmalloc(len, GFP_KERNEL);
		if (!this->page_buf) {
			printk(KERN_ERR "brcmnand_scan(): Can't allocate page_buf\n");
			return -ENOMEM;
		}
		this->options |= BRCMNAND_PAGEBUF_ALLOC;
	}

	this->state = FL_READY;
	init_waitqueue_head(&this->wq);
	spin_lock_init(&this->chip_lock);

	switch (mtd->oobsize) {
	case 64:
		this->autooob = &brcmnand_oob_64;
		break;

	case 16:
		this->autooob = &brcmnand_oob_16;
		break;

	default:
		printk(KERN_WARNING "No OOB scheme defined for oobsize %d\n",
			mtd->oobsize);
		/* To prevent kernel oops */
		this->autooob = &brcmnand_oob_16;
		break;
	}

	memcpy(&mtd->oobinfo, this->autooob, sizeof(struct nand_oobinfo));
	
printk("brcmnand_scan: OOB info pos[0][0]=%d len[0][1]=%d, pos[1][0]=%d, len[1][1]=%d\n", 
mtd->oobinfo.oobfree[0][0], mtd->oobinfo.oobfree[0][1], mtd->oobinfo.oobfree[1][0], mtd->oobinfo.oobfree[1][1);

	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH | MTD_ECC;
	mtd->ecctype = MTD_ECC_SW;
	mtd->erase = brcmnand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = brcmnand_read;
	mtd->write = brcmnand_write;
	mtd->read_ecc = brcmnand_read_ecc;
	mtd->write_ecc = brcmnand_write_ecc;
	mtd->read_oob = brcmnand_read_oob;
	mtd->write_oob = brcmnand_write_oob;

	mtd->readv = NULL;
	mtd->readv_ecc = NULL;
	mtd->writev = brcmnand_writev;
	mtd->writev_ecc = brcmnand_writev_ecc;
	mtd->sync = brcmnand_sync;
	mtd->lock = NULL;
	mtd->unlock = brcmnand_unlock;
	mtd->suspend = brcmnand_suspend;
	mtd->resume = brcmnand_resume;
	mtd->block_isbad = brcmnand_block_isbad;
	mtd->block_markbad = brcmnand_block_markbad;
	mtd->owner = THIS_MODULE;

	/* Unlock whole block */
PRINTK("Calling mtd->unlock(ofs=0, chipSize=%08x\n", this->chipSize);
	mtd->unlock(mtd, 0x0, this->chipSize);

PRINTK("Scanning BBT\n");
	err =  this->scan_bbt(mtd);
PRINTK("<-- brcmnand_scan, err=%d\n", err);
	return err;

}

/**
 * brcmnand_release - [BrcmNAND Interface] Free resources held by the BrcmNAND device
 * @param mtd		MTD device structure
 */
void brcmnand_release(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;

#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions (mtd);
#endif
	/* Deregister the device */
	del_mtd_device (mtd);

#if 1
	/* Buffer allocated by brcmnand_scan */
	if (this->options & BRCMNAND_PAGEBUF_ALLOC)
		kfree(this->page_buf);
#endif
}



