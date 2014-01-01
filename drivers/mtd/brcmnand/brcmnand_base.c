/*
 *  drivers/mtd/brcmnand/brcmnand_base.c
 *
    Copyright (c) 2005-2007 Broadcom Corporation                 
    
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
#include <asm/system.h> // For sync on MIPS24K
#include <asm/delay.h>


//#include "bbm.h"

#include "brcmnand_priv.h"


#define PRINTK(...)

#define my_be32_to_cpu(x) be32_to_cpu(x)

#ifdef CONFIG_MIPS_BCM7440B0
	#define PLATFORM_IOFLUSH_WAR()	__sync()
#else
	#define PLATFORM_IOFLUSH_WAR()	
#endif

int gdebug=0;

// Whether we should clear the BBT to fix a previous error.
/* This will eventually be on the command line, to allow a user to 
 * clean the flash
 */
extern int gClearBBT;

/* Number of NAND chips, only applicable to v1.0+ NAND controller */
extern int gNumNand;

/* The Chip Select [0..7] for the NAND chips from gNumNand above, only applicable to v1.0+ NAND controller */
extern int* gNandCS;

#define DRIVER_NAME	"brcmnand"

#define HW_AUTOOOB_LAYOUT_SIZE		32 /* should be enough */

#define BRCMNAND_CORRECTABLE_ECC_ERROR		(1)
#define BRCMNAND_UNCORRECTABLE_ECC_ERROR	(-1)

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
	uint32 timing1, timing2; // Specify a non-zero value to override the default timings.
} brcmnand_chip_Id;

/*
 * List of supported chip
 */
static brcmnand_chip_Id brcmnand_chips[] = {
	{	/* 0 */
		.chipId = SAMSUNG_K9F1G08U0A,
		.mafId = FLASHTYPE_SAMSUNG,
		.chipIdStr = "Samsung K9F1G08U0A",
		.options = NAND_USE_FLASH_BBT, 		/* Use BBT on flash */
				//| NAND_COMPLEX_OOB_WRITE	/* Write data together with OOB for write_oob */
		.timing1 = 0, //00070000,
		.timing2 = 0,
	},
	{	/* 1 */
		.chipId = ST_NAND01GW3B,
		.mafId = FLASHTYPE_ST,
		.chipIdStr = "ST NAND01GW3B2B",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},
	{	/* 2 */
		.chipId = ST_NAND512W3A,
		.mafId = FLASHTYPE_ST,
		.chipIdStr = "ST ST_NAND512W3A",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, //0x6474555f, 
		.timing2 = 0, //0x00000fc7,
	},
	{	/* 3 */
		.chipId = ST_NAND256W3A,
		.mafId = FLASHTYPE_ST,
		.chipIdStr = "ST ST_NAND256W3A",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, //0x6474555f, 
		.timing2 = 0, //0x00000fc7,
	},
#if 0 // EOL
	{	/* 4 */
		.chipId = HYNIX_HY27UF081G2M,
		.mafId = FLASHTYPE_HYNIX,
		.chipIdStr = "HYNIX HY27UF081G2M",
		.options = NAND_USE_FLASH_BBT 
			,
	},
#endif
	/* This is the new version of HYNIX_HY27UF081G2M which is EOL.
	 * Both use the same DevID
	 */
	{	/* 4 */
		.chipId = HYNIX_HY27UF081G2A,
		.mafId = FLASHTYPE_HYNIX,
		.chipIdStr = "Hynix HY27UF081G2A",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},

	{	/* 5 */
		.chipId = MICRON_MT29F2G08AAB,
		.mafId = FLASHTYPE_MICRON,
		.chipIdStr = "MICRON_MT29F2G08AAB",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},
/* This is just the 16 bit version of the above?
	{
		.chipId = MICRON_MT29F2G16AAB,
		.mafId = FLASHTYPE_MICRON,
		.chipIdStr = "MICRON_MT29F2G16AAB",
		.options = NAND_USE_FLASH_BBT 
			,
	}
*/

	{	/* 6 */
		.chipId = SAMSUNG_K9F2G08U0A,
		.mafId = FLASHTYPE_SAMSUNG,
		.chipIdStr = "Samsung K9F2G08U0A",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},


	{	/* 7 */
		.chipId = HYNIX_HY27UF082G2A,
		.mafId = FLASHTYPE_HYNIX,
		.chipIdStr = "Hynix HY27UF082G2A",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},



	{	/* 8 */
		.chipId = HYNIX_HY27UF084G2M,
		.mafId = FLASHTYPE_HYNIX,
		.chipIdStr = "Hynix HY27UF084G2M",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},
#if 0
	{	/* 9 */
		.chipId = SAMSUNG_K9K8G08UOA,
		.mafId = FLASHTYPE_SAMSUNG,
		.chipIdStr = "Samsung K9K8G08UOA",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	},


#endif
	
	{	/* LAST DUMMY ENTRY */
		.chipId = 0,
		.mafId = 0,
		.chipIdStr = "UNSUPPORTED NAND CHIP",
		.options = NAND_USE_FLASH_BBT,
		.timing1 = 0, .timing2 = 0,
	}
};

// Max chip account for the last dummy entry
#define BRCMNAND_MAX_CHIPS (ARRAY_SIZE(brcmnand_chips) - 1)

#include <mtd/brcmnand_oob.h> /* BRCMNAND controller defined OOB */

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

static unsigned char eccmask[64]; // Will be initialized during probe


static uint32_t brcmnand_ctrl_read(uint32_t nandCtrlReg) 
{
	volatile unsigned long* pReg = (volatile unsigned long*) (BRCMNAND_CTRL_REGS 
		+ nandCtrlReg - BCHP_NAND_REVISION);

	if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_BLK_WR_PROTECT ||
		(nandCtrlReg & 0x3) != 0) {
		printk(KERN_ERR "brcmnand_ctrl_read: Invalid register value %08x\n", nandCtrlReg);
	}
if (gdebug > 3) printk("%s: CMDREG=%08x val=%08x\n", __FUNCTION__, (unsigned int) nandCtrlReg, (unsigned int)*pReg);
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
if (gdebug > 3) printk("%s: CMDREG=%08x val=%08x\n", __FUNCTION__, nandCtrlReg, val);
}


/*
 * this: BRCM NAND handle
 * offset: offset from start of mtd
 * cmdEndAddr: 1 for CMD_END_ADDRESS, 0 for CMD_ADDRESS
 */
static void brcmnand_ctrl_writeAddr(struct brcmnand_chip* this, L_OFF_T offset, int cmdEndAddr) 
{
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	uint32_t pAddr = offset + this->pbase;

	this->ctrl_write(cmdEndAddr? BCHP_NAND_CMD_END_ADDRESS: BCHP_NAND_CMD_ADDRESS, pAddr);

#else
	uint32_t udw, ldw, cs;
	DIunion chipOffset;
	
char msg[24];


	// cs is the index into this->CS[]
	cs = (uint32_t) __ll_RightShift(offset, this->chip_shift);
	// chipOffset is offset into the current CS
	chipOffset.ll = __ll_and32(offset, this->chipSize - 1);

	if (cs >= this->numchips) {
		printk(KERN_ERR "%s: Offset=%s outside of chip range cs=%d, pthis->CS[cs]=%d\n", 
			__FUNCTION__, __ll_sprintf(msg, offset), cs, this->CS[cs]);
		BUG();
		return;
	}

if (gdebug) printk("CS=%d, pthis->CS[cs]=%d\n", cs, this->CS[cs]);

	// ldw is lower 32 bit of chipOffset, need to add pbase when on CS0
	if (this->CS[cs] == 0) {
		ldw = chipOffset.s.low + this->pbase;
	}
	else {
		ldw = chipOffset.s.low;
	}
	udw = chipOffset.s.high | (this->CS[cs] << 16);

if (gdebug > 3) printk("%s: offset=%s  cs=%d ldw = %08x, udw = %08x\n", __FUNCTION__, __ll_sprintf(msg, offset), cs,  ldw, udw);
	this->ctrl_write(cmdEndAddr? BCHP_NAND_CMD_END_ADDRESS: BCHP_NAND_CMD_ADDRESS, ldw);
	this->ctrl_write(BCHP_NAND_CMD_EXT_ADDRESS, udw);


#endif
}

static void print_diagnostics(void)
{
	uint32_t nand_acc_control = brcmnand_ctrl_read(BCHP_NAND_ACC_CONTROL);
	uint32_t nand_config = brcmnand_ctrl_read(BCHP_NAND_CONFIG);
	uint32_t flash_id = brcmnand_ctrl_read(BCHP_NAND_FLASH_DEVICE_ID);
	uint32_t pageAddr = brcmnand_ctrl_read(BCHP_NAND_PROGRAM_PAGE_ADDR);
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	uint32_t pageAddrExt = brcmnand_ctrl_read(BCHP_NAND_PROGRAM_PAGE_EXT_ADDR);
#endif
	uint32_t ebiCSBase0 = * ((volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_BASE_0));
	//unsigned long nand_timing1 = brcmnand_ctrl_read(BCHP_NAND_TIMING_1);
	//unsigned long nand_timing2 = brcmnand_ctrl_read(BCHP_NAND_TIMING_2);

	printk("ACC_CONTROL=%08x, \tNAND_CONFIG=%08x, FLASH_ID=%08x\n", nand_acc_control, nand_config, flash_id);
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	printk("PAGE_EXT_ADDR=%08x\n", pageAddrExt);
#endif
	printk("PAGE_ADDR=%08x, \tCS0_BASE=%08x\n", pageAddr, ebiCSBase0);
}		

static void print_config_regs(void)
{
	unsigned long nand_acc_control = brcmnand_ctrl_read(BCHP_NAND_ACC_CONTROL);
	unsigned long nand_config = brcmnand_ctrl_read(BCHP_NAND_CONFIG);
	unsigned long flash_id = brcmnand_ctrl_read(BCHP_NAND_FLASH_DEVICE_ID);
	unsigned long nand_timing1 = brcmnand_ctrl_read(BCHP_NAND_TIMING_1);
	unsigned long nand_timing2 = brcmnand_ctrl_read(BCHP_NAND_TIMING_2);
	
	
	printk("\nFound NAND: ACC=%08lx, cfg=%08lx, flashId=%08lx, tim1=%08lx, tim2=%08lx\n", 
		nand_acc_control, nand_config, flash_id, nand_timing1, nand_timing2);	
}


#if 1
void print_oobbuf(const unsigned char* buf, int len)
{
int i;


if (!buf) {printk("NULL"); return;}
 for (i=0; i<len; i++) {
  if (i % 16 == 0) printk("\n");
  else if (i % 4 == 0) printk(" ");
  printk("%02x", buf[i]);
 }
 printk("\n");
}

void print_databuf(const unsigned char* buf, int len)
{
int i;


 for (i=0; i<len; i++) {
  if (i % 32 == 0) printk("\n%04x: ", i);
  else if (i % 4 == 0) printk(" ");
  printk("%02x", buf[i]);
 }
 printk("\n");
}

#endif

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


static int brcmnand_from_flash_memcpy32(struct brcmnand_chip* this, void* dest, L_OFF_T offset, int len)
{
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	u_char* flash = this->vbase + offset;
#else
	u_char* flash = this->vbase;
#endif
	u_char* pucDest = (u_char*) dest; 
	u_char* pucFlash = (u_char*) flash; 
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
	/*
	 * Take care of the leading odd bytes.  
	 * Can't use memcpy here, because the flash contents are in BE order
	 */
	odd = (((unsigned long) pucFlash) & 0x3);
	if (odd) {
printk("****** WARNING: leading odd bytes ************\n");
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

#if 1
	// THT: Changed to use built-in kernel memcpy() for Prefetch
if (gdebug) printk("%s: pucFlash=%p, len=%d\n", __FUNCTION__, pucFlash, len);
	memcpy(pucDest, pucFlash, len);

#else
	for (i=0; i< (len>>2); i++) {
		pDest[i] = /* THT 8/29/06  cpu_to_be32 */(pSrc[i]);
	}
#endif

	/*
	 * Take care of the trailing odd bytes.  
	 * Can't use memcpy here, because the flash contents are in BE order
	 */
	odd = (len & 0x3);
	if (odd) {
		pucDest = (unsigned char*) pDest;

printk("****** WARNING: trailing odd bytes ************\n");	
		/* pSrc is now aligned on a DW boundary */
		u32.u = /*cpu_to_be32 */ (*pSrc);

		for (i=0; i<odd; i++) {
			pucDest[i] = u32.c[odd+i];
		}
	}

	return 0;
}

static int brcmnand_to_flash_memcpy32(struct brcmnand_chip* this, L_OFF_T offset, const void* src, int len)
{
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	u_char* flash = this->vbase + offset;
#else
	u_char* flash = this->vbase;
#endif
	//int i;
	//uint32_t* pDest = (uint32_t*) flash;
	//uint32_t* pSrc = (uint32_t*) src;
	
	if (unlikely((unsigned long) flash & 0x3)) {
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

#if 1
	// Use memcpy to take advantage of Prefetch
if (gdebug) printk("%s: flash=%p, len=%d\n", __FUNCTION__, flash, len);
	memcpy(flash, src, len);
#else
	for (i=0; i< (len>>2); i++) {
		pDest[i] = /* THT: 8/29/06 cpu_to_be32  */ (pSrc[i]);
	}
#endif

	return 0;
}


/*
 * Returns	 0: No errors
 *			 1: Correctable error
 *			-1: Uncorrectable error
 */
static int brcmnand_verify_ecc(struct brcmnand_chip* this, int state)
{
	int err = 0;
	uint32_t addr;
	uint32_t extAddr = 0;

	/* Only make sense on read */
	if (state != FL_READING) 
		return 0;

	addr = this->ctrl_read(BCHP_NAND_ECC_CORR_ADDR);
	if (addr) {

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
		extAddr = this->ctrl_read(BCHP_NAND_ECC_CORR_EXT_ADDR);
		// Clear it
		this->ctrl_write(BCHP_NAND_ECC_CORR_EXT_ADDR, 0);
#endif

		// Clear it
		this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);
		printk(KERN_WARNING "%s: Correctable ECC error at %08x:%08x\n", __FUNCTION__, extAddr, addr);
		err = BRCMNAND_CORRECTABLE_ECC_ERROR;
	}

	addr = this->ctrl_read(BCHP_NAND_ECC_UNC_ADDR);
	if (addr) {
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
		extAddr = this->ctrl_read(BCHP_NAND_ECC_UNC_EXT_ADDR);
		// Clear it
		this->ctrl_write(BCHP_NAND_ECC_UNC_EXT_ADDR, 0);
#endif
		this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);

		/*
		 * If the block was just erased, and have not yet been written to, this will be flagged,
		 * so this could be a false alarm
		 */

		err = BRCMNAND_UNCORRECTABLE_ECC_ERROR;
	}
	return err;
}

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
	timeout = jiffies + msecs_to_jiffies(3000); // THT: 3secs, for now
	while (time_before(jiffies, timeout)) {
		PLATFORM_IOFLUSH_WAR();
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

#if 0
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
#endif

/* 
 * Returns 	 1: Success, no errors
 * 			 0: Timeout
 *			-1: Errors
 */
static int brcmnand_cache_is_valid(struct mtd_info* mtd,  int state, L_OFF_T offset, int raw) 
{
	struct brcmnand_chip * this = mtd->priv;
	unsigned long timeout;
	uint32_t ready;

	
	/* The 20 msec is enough */
	timeout = jiffies + msecs_to_jiffies(3000); // 3 sec timeout for now
	while (time_before(jiffies, timeout)) {
		PLATFORM_IOFLUSH_WAR();
		ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

		if (ready & (BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK | 0x1)) {
			int ecc;
			
			if (ready & 0x1) {
				printk(KERN_ERR "%s: Flash chip report error %08x\n", __FUNCTION__, ready);
				return -1;
			}

			if (!raw) {
				ecc = brcmnand_verify_ecc(this, state);
				if (ecc < 0) {
// Let caller handle it
//printk("%s: Possible Uncorrectable ECC error at offset %08x\n", __FUNCTION__, (unsigned long) offset);
					return BRCMNAND_UNCORRECTABLE_ECC_ERROR;
				}
			}
			return 1;
		}
		if (state != FL_READING)
			cond_resched();

	}

	return 0; // TimeOut
}

/* 
 * Returns 	 1: Success, no errors
 * 			 0: Timeout
 *			-1: Errors
 */
static int brcmnand_spare_is_valid(struct mtd_info* mtd,  int state, int raw) 
{
	struct brcmnand_chip * this = mtd->priv;
	unsigned long timeout;
	uint32_t ready;

	/* The 20 msec is enough */
	timeout = jiffies + msecs_to_jiffies(3000);  // 3 sec timeout for now
	while (time_before(jiffies, timeout)) {
		PLATFORM_IOFLUSH_WAR();
		ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

		if (ready & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
			int ecc;

			if (!raw) {
				ecc = brcmnand_verify_ecc(this, state);
				if (ecc < 0) {
//printk("%s: Uncorrectable ECC error at offset %08x\n", __FUNCTION__, (unsigned long) offset);
					return -1;
				}
			}
			return 1;
		}
		if (state != FL_READING)
			cond_resched();
	}

	return 0; // Timed out
}

#if 0
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
			flashStatus = status & 0x01;
			return flashStatus; // 0 = write completes with no errors
		}
		else {
			err = -ETIMEDOUT;
		}
	}
	return 1;
}
#endif

static int brcmnand_write_is_complete(struct mtd_info *mtd, int* outp_needBBT)
{
	int err;
	uint32_t status;
	uint32_t flashStatus = 0;

	*outp_needBBT = 1;
	err = brcmnand_wait(mtd, FL_WRITING, &status);
	if (!err) {
		if (status & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
			flashStatus = status & 0x01;
			*outp_needBBT = flashStatus; // 0 = write completes with no errors
			return 1;
		}
		else {
			return 0;
		}
	}
	return 0;
}


#if 0
/* Compare the OOB areas, skipping over the ECC, and BI bytes */

static int ecccmp(struct mtd_info* mtd, L_OFF_T offset, const unsigned char* left, const unsigned char* right, int len)
{
//	struct brcmnand_chip *this = mtd->priv;
	int i, ret=0;
	L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));  // Beginning of slice
	L_OFF_T pageOffset = __ll_and32(offset, ~ (mtd->oobblock - 1)); // Beginning of page
	int sliceNum =  __ll_isub(sliceOffset , pageOffset) / mtd->eccsize; // Slice number 0-3
	int oobOffset = sliceNum * mtd->eccsize;

	/* Walk through the autoplace chunks */
	for (i = 0; i < len; i++) {
		if ((left[i] & eccmask[i+oobOffset]) != (right[i] & eccmask[i+oobOffset])) {
			ret = 1;
			break;
		}
	}
	
	return ret;
}
#endif


/**
 * brcmnand_convert_oobfree_to_fsbuf - [GENERIC] Read 1 page's oob buffer into fs buffer
 * @param mtd		MTD device structure
 * @oob_buf			IN: The raw OOB buffer read from the NAND flash
 * @param fsbuf		OUT: the converted file system oob buffer
 * @oob_buf			raw oob buffer read from flash
 * returns the number of bytes converted.
 */
static int brcmnand_convert_oobfree_to_fsbuf(struct mtd_info* mtd, 
	u_char* raw_oobbuf, u_char* out_fsbuf, int fslen, struct nand_oobinfo* oobsel, int autoplace) 
{
	int i;
	int len = 0;

	/* without autoplace. Legacy mode used by YAFFS1 */
	switch(oobsel->useecc) {
	case MTD_NANDECC_AUTOPLACE:
	case MTD_NANDECC_AUTOPL_USR:
		/* Walk through the autoplace chunks */
		for (i = 0; oobsel->oobfree[i][1] && i < ARRAY_SIZE(oobsel->oobfree); i++) {
			int from = oobsel->oobfree[i][0];
			int num = oobsel->oobfree[i][1];
			int clen;

			if (num == 0) break; // Reach end marker
			clen = min_t(int, num, fslen-len);			
			memcpy(&out_fsbuf[len], &raw_oobbuf[from], clen);
			len += clen;
		}
		break;
	case MTD_NANDECC_PLACE:
		len = mtd->oobsize;
		memcpy(out_fsbuf, raw_oobbuf, len);
		break;
		
	default:
		printk(KERN_ERR "%s: Invalid useecc %d not supported\n",
			__FUNCTION__, oobsel->useecc);
		len = 0;
	}
	return len;

}




/**
 * brcmnand_prepare_oobbuf - [GENERIC] Prepare the out of band buffer
 * @mtd:	MTD device structure
 * @fsbuf:	buffer given by fs driver (input oob_buf)
 * @fslen:	size of buffer
 * @oobsel:	out of band selection structure
 * @oob_buf:	Output OOB buffer
 * @autoplace:	1 = place given buffer into the free oob bytes
 * @numpages:	number of pages to prepare. The Brcm version only handles 1 page at a time.
 * @retlen:	returned len, i.e. number of bytes used up from fsbuf.
 *
 * Return:
 * 1. Filesystem buffer available and autoplacement is off,
 *    return filesystem buffer
 * 2. No filesystem buffer or autoplace is off, return internal
 *    buffer
 * 3. Filesystem buffer is given and autoplace selected
 *    put data from fs buffer into internal buffer and
 *    retrun internal buffer
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
 * 
 *
*/
static u_char * 
brcmnand_prepare_oobbuf (
		struct mtd_info *mtd, const u_char *fsbuf, int fslen, u_char* oob_buf, struct nand_oobinfo *oobsel,
		int autoplace, int* retlen)
{
	struct brcmnand_chip *this = mtd->priv;
	int i, len, ofs;

	*retlen = 0;

	/* Zero copy fs supplied buffer */
	if (fsbuf && !autoplace) {
		*retlen = fslen;
		return (u_char*) fsbuf;
	}

	/* Check, if the buffer must be filled with ff again */
	//if (this->oobdirty) 
	{
		memset (oob_buf, 0xff, mtd->oobsize );
		this->oobdirty = 0;
	}

	/* If we have no autoplacement or no fs buffer use the internal one */
	if (!autoplace || !fsbuf) {
		*retlen = 0;
		return oob_buf;
	}

	/* Walk through the pages and place the data */
	this->oobdirty = 1;
	ofs = 0;
	//while (numpages--) 
	{
		for (i = 0, len = 0; len < mtd->oobavail && len < fslen && i < ARRAY_SIZE(oobsel->oobfree); i++) {
			int to = ofs + oobsel->oobfree[i][0];
			int num = oobsel->oobfree[i][1];

			if (num == 0) break; // End marker reached
#if 1
			if ((len + num) > fslen) {

				memcpy(&oob_buf[to], &fsbuf[*retlen], fslen-len);
				*retlen += fslen-len;
				break;
			}
#endif			

			memcpy (&oob_buf[to], &fsbuf[*retlen], num);
			len += num;
			*retlen += num;
		}
		ofs += mtd->oobavail;
	}

	return oob_buf;
}



/**
 * brcmnand_posted_read_cache - [BrcmNAND Interface] Read the 512B cache area
 * Assuming brcmnand_get_device() has been called to obtain exclusive lock
 * @param mtd		MTD data structure
 * @param oobarea	Spare area, pass NULL if not interested
 * @param buffer	the databuffer to put/get data, pass NULL if only spare area is wanted.
 * @param offset	offset to read from or write to, must be 512B aligned.
 * @param raw: Ignore BBT bytes when raw = 1
 *
 * Caller is responsible to pass a buffer that is
 * (1) large enough for 512B for data and optionally an oobarea large enough for 16B.
 * (2) 4-byte aligned.
 *
 * Read the cache area into buffer.  The size of the cache is mtd-->eccsize and is always 512B.
 */
static int brcmnand_posted_read_cache(struct mtd_info* mtd, 
		void* buffer, u_char* oobarea, L_OFF_T offset, int raw)
{
	struct brcmnand_chip* this = mtd->priv;
	L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
	int i, ret;
	int retries = 2, done = 0;
	uint32_t* p32 = (uint32_t*) oobarea;

if (gdebug > 3) {
printk("%s: offset=%08x, oobarea=%p\n", __FUNCTION__, __ll_low(offset), oobarea);}

	while (retries > 0 && !done) {
		if (unlikely(__ll_isub(offset, sliceOffset))) {
			printk(KERN_ERR "%s: offset %08x is not cache aligned, sliceOffset=%08lx, CacheSize=%d\n", 
				__FUNCTION__, (unsigned int) offset, (unsigned long) sliceOffset, mtd->eccsize);
			return -EINVAL;
		}


		this->ctrl_writeAddr(this, sliceOffset, 0);
		PLATFORM_IOFLUSH_WAR();
		this->ctrl_write(BCHP_NAND_CMD_START, OP_PAGE_READ);

		// Wait until cache is filled up
		switch (brcmnand_cache_is_valid(mtd, FL_READING, offset, raw)) {
		case 1: /* Success, no errors */
			if (buffer) {
				brcmnand_from_flash_memcpy32(this, buffer, offset, mtd->eccsize);
			}

			if (oobarea) {
				PLATFORM_IOFLUSH_WAR();
				for (i = 0; i < 4; i++) {
					p32[i] = /* THT 11-30-06 */ be32_to_cpu (this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + i*4));
				}
if (gdebug) {printk("%s: offset=%08x, oob=\n", __FUNCTION__, __ll_low(sliceOffset)); print_oobbuf(oobarea, 16);}
			}

			ret = 0; // Success
			done = 1;
			break;
			
		case BRCMNAND_UNCORRECTABLE_ECC_ERROR:
			{
				/* Flash chip returns errors 

				|| There is a bug in the controller, where if one reads from an erased block that has NOT been written to,
				|| this error is raised.  
				|| (Writing to OOB area does not have any effect on this bug)
				|| The workaround is to also look into the OOB area, to see if they are all 0xFF
				
				*/
				u_char oobbuf[16];
				int all00, allFF;

				if (!oobarea) 
					oobarea = &oobbuf[0];
				p32 = (uint32_t*) oobarea;

				for (i = 0; i < 4; i++) {
					p32[i] = /* THT 11-30-06 */ be32_to_cpu (this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + i*4));
				}

				all00 = (oobarea[6] == 0xff && oobarea[7] == 0xff && oobarea[8] == 0xff);
				allFF = (oobarea[6] == 0x00 && oobarea[7] == 0x00 && oobarea[8] == 0x00);
				if ( all00 || allFF) {
					/* 
					 * For the first case, the slice is an erased block, and the ECC bytes are all 0xFF,
					 * for the 2nd, all bytes are 0xFF, so the Hamming Codes for it are all zeroes.
					 * The current version of the BrcmNAND controller treats these as un-correctable errors.
					 * For either case, fill data buffer with 0x00 or 0xff and return success.  The error has already
					 * been cleared inside brcmnand_verify_ecc.
					 * Both case will be handled correctly by the BrcmNand controller in later releases.
					 */
					p32 = (uint32_t*) buffer;
					for (i=0; i < this->eccsize/4; i++) {
						p32[i] = 0xFFFFFFFF;
					}
					ret = 0; // Success

				}
				else {
					/* Real error: Disturb read returns uncorrectable errors */
					ret = -EBADMSG; 
printk("<-- %s: ret -EBADMSG\n", __FUNCTION__);
				}
				done = 1;
				
				break;
			}
			
		case 0:
			//Read has timed out 
			printk(KERN_ERR "brcmnand_cache_is_valid returns 0\n");
			ret = -ETIMEDOUT;
			done = 1;
			break;

		default:
			BUG_ON(1);
			/* Should never gets here */
			ret = -EINVAL;
			done = 1;
			break; 
		}
	}

	return ret;
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
		unsigned char* oobarea, L_OFF_T offset, int raw)
{
	struct brcmnand_chip* this = mtd->priv;
	L_OFF_T sliceOffset = __ll_and32(offset, ~(mtd->eccsize - 1));
	int i, ret, done = 0;
	int retries = 5;
	
//char msg[20];
//printk("->%s: offset=%s\n", __FUNCTION__, __ll_sprintf(msg, offset));

	while (retries > 0 && !done) {
		if (unlikely(__ll_isub(sliceOffset, offset))) {
			printk(KERN_ERR "%s: offset %08x is not cache aligned\n", 
				__FUNCTION__, __ll_low(offset));
			return -EINVAL;
		}

		this->ctrl_writeAddr(this, sliceOffset, 0);
		this->ctrl_write(BCHP_NAND_CMD_START, OP_SPARE_AREA_READ);

		// Wait until spare area is filled up
		switch (brcmnand_spare_is_valid(mtd, FL_READING, raw)) {
		case 1:
			if (oobarea) {
				uint32_t* p32 = (uint32_t*) oobarea;
				
				for (i = 0; i < 4; i++) {
					p32[i] = /* THT 11-30-06 */ be32_to_cpu /**/(this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + (i<<2)));
				}
if (gdebug) {printk("%s: offset=%08x, oob=\n", __FUNCTION__, __ll_low(sliceOffset)); print_oobbuf(oobarea, 16);}

			}
			
			ret = 0;
			done = 1;
			break;

		case -1:
			ret = -EBADMSG;
printk("%s: ret = -EBADMSG\n", __FUNCTION__);
			/* brcmnand_spare_is_valid also clears the error bit, so just retry it */
			retries--;
			break;
			
		case 0:
			//Read has timed out or read found bad block. TBD: Find out which is which
			ret = -ETIMEDOUT;
			done = 1;
			break;
			
		default:
			BUG_ON(1);
			/* NOTREACHED */
			ret = -EINVAL;
			done = 1;
			break; /* Should never gets here */
		}

	}	

//printk("<-- %s\n", __FUNCTION__);
	return ret;
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
		const void* buffer, const unsigned char* oobarea, L_OFF_T offset)
{
	struct brcmnand_chip* this = mtd->priv;
	L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
	uint32_t* p32;
	int i, needBBT=0;
	int ret;

	char msg[20];


//printk("10\n");
	if (unlikely(__ll_isub(sliceOffset, offset))) {
		printk(KERN_ERR "%s: offset %s is not cache aligned\n", 
			__FUNCTION__, __ll_sprintf(msg, offset));

		ret =  -EINVAL;
		goto out;
	}
	this->ctrl_writeAddr(this, sliceOffset, 0);

//printk("20\n");
	if (buffer) {
		brcmnand_to_flash_memcpy32(this, offset, buffer, mtd->eccsize);
	}
	/* Must write data when NAND_COMPLEX_OOB_WRITE */
	else if (this->options & NAND_COMPLEX_OOB_WRITE) {
		brcmnand_to_flash_memcpy32(this, offset, ffchars, mtd->eccsize);
	}

//printk("30\n");
	if (oobarea) {
		p32 = (uint32_t*) oobarea;
if (gdebug) {printk("%s: oob=\n", __FUNCTION__); print_oobbuf(oobarea, 16);}
	}
	else {
		// Fill with 0xFF if don't want to change OOB
		p32 = (uint32_t*) &ffchars[0];
	}

//printk("40\n");
	for (i = 0; i < 4; i++) {
		this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, /* THT 11-30-06 */ cpu_to_be32 /* */(p32[i]));
	}

	PLATFORM_IOFLUSH_WAR();
	this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_PAGE);
//printk("50\n");

	// Wait until flash is ready
	if (brcmnand_write_is_complete(mtd, &needBBT)) {
		if (!needBBT) {
			ret = 0;
			goto out;
		}
	
		else { // Need BBT
			printk(KERN_WARNING "%s: Marking bad block @%s\n", __FUNCTION__, __ll_sprintf(msg, offset));
//printk("80 block mark bad\n");
			ret = this->block_markbad(mtd, offset);
			ret = -EINVAL;
			goto out;
		}
	}
	//Write has timed out or read found bad block. TBD: Find out which is which
	printk(KERN_INFO "%s: Timeout\n", __FUNCTION__);
	ret = -ETIMEDOUT;

out:
//printk("99\n");

	return ret;
}

/**
 * brcmnand_posted_write_oob - [BrcmNAND Interface] Write the spare area
 * @param mtd		MTD data structure
 * @param oobarea	Spare area, pass NULL if not interested.  Must be able to 
 *					hold mtd->oobsize (16) bytes.
 * @param offset	offset to write to, and must be 512B aligned
 *
 */
static int brcmnand_posted_write_oob(struct mtd_info *mtd,
		const unsigned char* oobarea, L_OFF_T offset)
{
	struct brcmnand_chip* this = mtd->priv;
	L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
	uint32_t* p32;
	int i, needBBT=0;


	if (unlikely(__ll_isub(sliceOffset, offset))) {
		printk(KERN_ERR "%s: offset %08x is not cache aligned\n", 
			__FUNCTION__, (unsigned int) offset);
	}
	
	this->ctrl_writeAddr(this, sliceOffset, 0);

	/* Must write data when NAND_COMPLEX_OOB_WRITE option is set */
	if (this->options & NAND_COMPLEX_OOB_WRITE) {
		brcmnand_to_flash_memcpy32(this, offset, ffchars, mtd->eccsize);
	}

	// assert oobarea here
	BUG_ON(!oobarea);	
	p32 = (uint32_t*) oobarea;
		
	for (i = 0; i < 4; i++) {
		this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, /* THT 11-30-06 */ cpu_to_be32 (p32[i]));
	}

	PLATFORM_IOFLUSH_WAR();
	if (this->options & NAND_COMPLEX_OOB_WRITE) {
printk("****** Workaround, using OP_PROGRAM_PAGE instead of OP_PROGRAM_SPARE_AREA\n");
		this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_PAGE);
	}
	else {
		this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_SPARE_AREA);
	}

	// Wait until flash is ready
	if (brcmnand_write_is_complete(mtd, &needBBT)) {
		return 0;
	}
	if (needBBT){
		int ret;
		
		printk(KERN_WARNING "%s: Marking bad block @%08x\n", __FUNCTION__, (unsigned int) offset);
		ret = this->block_markbad(mtd, offset);
		return -EINVAL;
	}

	return -ETIMEDOUT;
	
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

static struct brcmnand_chip* 
brcmnand_get_device_exclusive(void)
{
	struct brcmnand_chip *this = (struct brcmnand_chip*) get_brcmnand_handle();
	struct mtd_info *mtd = (struct mtd_info*) this->priv;
	int ret;

	ret = brcmnand_get_device(mtd, BRCMNAND_FL_XIP);
	if (0 == ret)
		return this;
	else
		return ((struct brcmnand_chip *) 0);
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

/*
 * Determine whether we should use autoplace
 */
static int 
brcmnand_autoplace(struct mtd_info *mtd, struct nand_oobinfo* oobsel)
{
	//struct brcmnand_chip *this = mtd->priv;
	int autoplace;

	/* if oobsel is NULL, use chip defaults */
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;

	switch(oobsel->useecc) {
		case MTD_NANDECC_AUTOPLACE:
		case MTD_NANDECC_AUTOPL_USR:
			autoplace = 1;
			break;
		default:
			autoplace = 0;
			break;
		}

	return autoplace;
}







/**
 * brcmnand_read_page - [GENERIC] Read 1 page data and the page OOB into internal buffer.
 * @param mtd		MTD device structure
 * @param offset		offset from start of flash
 * @param data_buf	Data buffer to be read to, must be at least mtd->oobblock
 * @param len		Length of data buffer, must be equal or less than page size
 * @param oob_buf	OOB buffer, must be at least mtd->oobsize
 * Assume that device lock has been held
 * returns0 on success
 */
static int brcmnand_read_page(struct mtd_info* mtd, 
		L_OFF_T offset, u_char* data_buf, int len, u_char* oob_buf, 
		struct nand_oobinfo* oobsel, int* retlen, int* retooblen)
{
	struct brcmnand_chip *this = mtd->priv;
	int dataRead = 0, oobRead = 0;
	int winslice;
	int oob = 0;
	int ret;
	int autoplace; 
	u_char local_oob[64];
	u_char* pOobBuf = NULL;

char msg[20];

	*retlen = 0;
	*retooblen = 0;


	switch(oobsel->useecc) {
		case MTD_NANDECC_AUTOPLACE:
		case MTD_NANDECC_AUTOPL_USR:
			autoplace = 1;
			/* Must read into temporary oob buffer, in order to convert it */
			pOobBuf = oob_buf ? local_oob : NULL;
			break;
		default:
			autoplace = 0;
			pOobBuf = oob_buf ? oob_buf : NULL;
			break;
		}
if (gdebug) printk("--> %s: offset=%s oob_buf=%p, autoplace=%d\n", 
	__FUNCTION__, __ll_sprintf(msg, offset), pOobBuf, autoplace);

			
	if (len > mtd->oobblock || len < 0) {
		printk(KERN_ERR "%s: Invalid length %d\n", __FUNCTION__, len);
		return -EINVAL;
	}
	for (winslice = 0; winslice < this->eccsteps && dataRead < len; winslice++) {
		int thislen = min_t(int, len-dataRead, mtd->eccsize);
		//int i;
		unsigned char* oobbuf = (pOobBuf ? &pOobBuf[oobRead] : NULL);

		// If partial read, must use internal buffer for data.  
		if (thislen != mtd->eccsize) {
			ret = brcmnand_posted_read_cache(mtd, &this->data_buf[winslice*this->eccsteps], oobbuf, 
					__ll_add32(offset, dataRead), 0);
			memcpy(&data_buf[dataRead], &this->data_buf[winslice*this->eccsteps], thislen);
		} 
		else {
			ret = brcmnand_posted_read_cache(mtd, &data_buf[dataRead], oobbuf, 
					__ll_add32(offset, dataRead), 0);
		}
		if (ret) {
			printk(KERN_ERR "%s: posted read cache failed at offset=%08lx, ret=%d\n", 
				__FUNCTION__, (unsigned long) __ll_add32(offset, dataRead), ret);
			return ret;
		}

		dataRead += thislen;
		oobRead += this->eccOobSize;
		
	}

	// Convert the free bytes into fsbuffer
	if (!oob_buf) {
		oob = 0;
	}
	else if (autoplace && pOobBuf) {
		oob = brcmnand_convert_oobfree_to_fsbuf(mtd, pOobBuf, oob_buf, oobRead, oobsel, autoplace);
	}
	else {
		oob = oobRead;
	}
		
	*retlen = dataRead;
	*retooblen = oob;

if (gdebug) printk("<-- %s, retlen=%d, retooblen=%d\n", __FUNCTION__, *retlen, *retooblen);
	return 0;
}



/**
 * brcmnand_read_pageoob - [GENERIC] write to one page's OOB area
 * @mtd:			MTD device structure
 * @offset: 		offset  from start of the chip, must be called with (offset & this->pagemask)
 * @len: 			size of data buffer, len must be <= mtd->oobblock, and can indicate partial page.
 * @data_buf: 	data buffer.
 * @oob_buf:		out of band data buffer for 1 page (size = oobsize), must be prepared beforehand
 * @retlen: 		Returned length
 * @convert2fs: 	Convert oob_buf to fsbuf
 *
 * Nand_page_program function is used for write and writev !
 * This function will always program a full page of data
 * If you call it with a non page aligned buffer, you're lost :)
 *
 * Cached programming is not supported yet.
 */
static int brcmnand_read_pageoob (struct mtd_info *mtd, L_OFF_T offset,
	u_char* oob_buf, int* retooblen, struct nand_oobinfo *oobsel, int autoplace, int raw)
{
	struct brcmnand_chip *this = mtd->priv;
	int oobRead = 0;
	int winslice;
	int ret;
	u_char this_oobbuf[64];
	u_char* pOobbuf;
char msg[20];
//printk("-->%s, offset=%s\n", __FUNCTION__, __ll_sprintf(msg, offset));

	/* If autoplace, use the scratch buffer */
	if (autoplace) {
		pOobbuf = this_oobbuf;
	}
	else {
		pOobbuf = oob_buf;
	}

	/*
	 * The 4 slice OOB areas may not have the same format, (e.g. for the 2K page, the first slice
	 * lose 2 bytes.)
	 * We have to prepare the OOB buffer for the entire page, then write it out one slice at a time.
	 */
	for (winslice = 0; winslice < this->eccsteps && oobRead < mtd->oobsize; winslice++) {
		L_OFF_T sliceOffset = __ll_add32(offset, this->eccsize*winslice);
		int sliceOobOffset = this->eccOobSize*winslice;

if (gdebug) printk("winslice=%d, slice=%s, sliceOobOffset=%d\n", 
	winslice, __ll_sprintf(msg, sliceOffset), sliceOobOffset);

		ret = brcmnand_posted_read_oob(mtd, &pOobbuf[sliceOobOffset], sliceOffset, raw);
//printk("brcmnand_posted_read_oob returns %d\n", ret);
		if (ret) {
			printk(KERN_ERR "%s: posted read cache failed at offset=%08ux, ret=%d\n", 
				__FUNCTION__, __ll_low(sliceOffset), ret);
			return ret;
		}
		oobRead += this->eccOobSize;
	}

//printk("end for\n");	
	*retooblen = oobRead;

	// Convert the free bytes into fsbuffer
	if (autoplace) {
//printk("Calling brcmnand_convert_oobfree_to_fsbuf\n");
		*retooblen = brcmnand_convert_oobfree_to_fsbuf(mtd, pOobbuf, oob_buf, mtd->oobsize, oobsel, autoplace);
	}
	
//printk("<-- %s\n", __FUNCTION__);
	return 0;
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
	struct brcmnand_chip *this = mtd->priv;
	int dataRead = 0, oobRead = 0;
	int thislen;
	L_OFF_T pageAddr;
	int page;
	int ret = 0;
	//u_char* raw_oob = this->oob_buf;
	int retooblen, retdatalen;
	loff_t startPageOffset;
	u_char data_buf[2048];


	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_read_ecc: from = 0x%08x, len = %i, oob_buf=%p, autoplace=%d\n", 
	(unsigned int) from, (int) len, oob_buf, brcmnand_autoplace(mtd, oobsel));
	*retlen = 0;
	//oobarea = oob_buf;

	/* Do not allow reads past end of device */
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely(from + len) > mtd->size) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(from, len), this->mtdSize))) 
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	/* THT: TBD: Move it inside while loop: It would be less efficient, but
	 * would also give less response time to multiple threads
	 */
	brcmnand_get_device(mtd, FL_READING);

	/* Setup variables and oob buffer */
	// We know that pageSize=128k or pageShift = 17 bits for large page, so page fits inside an int
	page = (int) __ll_RightShift(from, this->page_shift);
	startPageOffset = __ll_and32(from, ~(this->pageSize - 1));
	
	while (dataRead < len) {
		u_char* oobbuf;
		L_OFF_T offset;
		int leadingBytes;
		
		thislen = min_t(int, mtd->oobblock, len - dataRead);
		if (oob_buf) {
			oobbuf = &oob_buf[oobRead];
		}
		else {
			oobbuf = NULL;
		}

		/* page marks the start of the  block encompassing [from, thislen] */
		offset = __ll_add32(from, dataRead);
		pageAddr = __ll_and32(offset,  ~ (mtd->oobblock - 1));

		/* If not at start of page, read up to the next start of page */
		leadingBytes = __ll_isub(offset, pageAddr);
		if (leadingBytes) {
			thislen = min_t(int, len - dataRead, mtd->oobblock - leadingBytes);
			retooblen = retdatalen = 0;
			/* Read the entire page into the default buffer */
			ret = brcmnand_read_page(mtd, pageAddr, data_buf, mtd->oobblock, oobbuf, oobsel, &retdatalen, &retooblen);

			if (ret) {
				DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: read failed = %d\n", ret);
				// Do BTT management here
				goto out;
			}
			/* Copy from default buffer into actual outbuf */
			memcpy(&buf[dataRead], &data_buf[leadingBytes], thislen);

		}
		else {
			/* Check thislen against the page size, if not equal, we will have to use the default buffer, again */
			if (thislen == mtd->oobblock) {
				/* Read straight into buffer */
				ret = brcmnand_read_page(mtd, pageAddr, &buf[dataRead], thislen, oobbuf,  oobsel, &retdatalen, &retooblen);

				if (ret) {
					DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: read failed = %d\n", ret);
					// Do BTT management here
					goto out;
				}
			}
			else {
				/* thislen < mtd->oobblock
				 * This is a partial read: Read into default buffer  
				 * then copy over to avoid writing beyond the end of the input buffer
				 */
				// Here we run the risk of overwriting the OOB input buffer, but the interface does not allow
				// us to know the size of the receiving OOB.
				ret = brcmnand_read_page(mtd, pageAddr, data_buf, thislen, oobbuf, oobsel, &retdatalen, &retooblen);

				if (ret) {
					DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: read failed = %d\n", ret);
					// Do BTT management here
					goto out;
				}
				memcpy(&buf[dataRead], data_buf, thislen);
			}

		}
		dataRead += thislen;
		oobRead += retooblen;

		if (dataRead >= len)
			break;

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: read failed = %d\n", ret);
			goto out;
		}
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	/*
	 * Return success, if no ECC failures, else -EBADMSG
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EBADMSG
	 */
	*retlen = dataRead;
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
	DEBUG(MTD_DEBUG_LEVEL3, "-->%s from=%08x, len=%d\n", __FUNCTION__,
 		(unsigned long) from, (int) len);
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
static int brcmnand_read_oob(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct brcmnand_chip *this = mtd->priv; // Needed by v1.0, warning unused by < v1.0
	int oobread = 0, thislen;
	//unsigned int page;
	L_OFF_T pageAddr;
	int ret = 0;
	//int raw;
	struct nand_oobinfo place_oob;

	// Read_oob always use place_oob.
	
	place_oob = mtd->oobinfo;
	place_oob.useecc = MTD_NANDECC_PLACE;
	
 	DEBUG(MTD_DEBUG_LEVEL3, "%s: from = 0x%08x, len = %i\n", __FUNCTION__,
 		(unsigned long) from, (int) len);

	/* Initialize return length value */
	*retlen = 0;

	/*
	  * Well, we can't do this test any more since mtd->size may be larger than a 32bit integer can hold
	  */
	/* Do not allow reads past end of device */
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely(from + len) > mtd->size) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(from, len), this->mtdSize)))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_oob: Attempt read beyond end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_READING);

	/* page marks the start of the page block encompassing [from, thislen] */
	//page = ((unsigned long) from & ~ (mtd->oobblock - 1)) >> this->page_shift;
	pageAddr = __ll_and32(from, ~ (mtd->oobblock - 1));

	/* Make sure we don't go over bound */
	while (oobread < len) {
		
		ret = brcmnand_read_pageoob(mtd, pageAddr, &buf[oobread], &thislen, &place_oob, 0, 1);

		if (ret) {
			printk(KERN_ERR "brcmnand_read_pageoob returns %d, thislen = %d\n", ret, thislen);
		}
		
		// Just copy up to thislen
		//memcpy(&buf[oobread], &this->oob_buf, thislen);
		oobread += thislen;
		if (oobread >= len) {
			break;
		}

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_oob: read failed = %d\n", ret);
			goto out;
		}

		//buf += thislen;		
		pageAddr = __ll_add32(pageAddr, mtd->oobblock);
		
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	*retlen = oobread;

	return ret;
}




/**
 * brcmnand_read_oobfree - [MTD Interface] NAND read free out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 *
 * NAND read free out-of-band data from the spare area
 */
static int brcmnand_read_oobfree (struct mtd_info *mtd, 
			loff_t from, size_t len, size_t * retlen, u_char * out_fsbuf)
{

	struct brcmnand_chip *this = mtd->priv;
	int oobread = 0, thislen;
	//unsigned int page;
	L_OFF_T pageAddr;
	int ret = 0;
	int autoplace;


	DEBUG(MTD_DEBUG_LEVEL3, "-->%s from=%08x, len=%d\n", __FUNCTION__,
 		(unsigned long) from, (int) len);


	*retlen=0;

	autoplace = 1;

	/* Do not allow reads past end of device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely(from + len) > mtd->size) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(from, len), this->mtdSize)))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt read beyond end of device\n", __FUNCTION__);
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_READING);

	/* page marks the start of the page block encompassing [from, thislen] */
	//page = ((unsigned long) from & ~ (mtd->oobblock - 1)) >> this->page_shift;
	pageAddr = __ll_and32(from, ~ (mtd->oobblock - 1));

	/* Make sure we don't go over bound */
	while (oobread < len) {
		
		ret = brcmnand_read_pageoob(mtd, pageAddr,  &out_fsbuf[*retlen], &thislen, this->autooob, autoplace, 1);
	
		if (ret) {
			printk(KERN_ERR "%s: read_oob returns %d\n", __FUNCTION__, ret);
			goto out;
		}

		// Convert to fsbuf, returning # bytes converted (== mtd->oobavail)
		// already done in readpageoob
		// ret = brcmnand_convert_oobfree_to_fsbuf(mtd, this->oob_buf, &fsbuf[oobread], thislen, this->autooob, autoplace);

		// Advance to next page
		pageAddr = __ll_add32(pageAddr, mtd->oobblock);
		oobread += mtd->oobsize;
		*retlen += thislen;
		//from += mtd->oobblock;
	}

	ret = 0;

out:
	brcmnand_release_device(mtd);

if (4 == gdebug) gdebug = 0;
	return ret;
}




#ifdef CONFIG_MTD_BRCMNAND_VERIFY_WRITE
/*
 * Returns 0 on success, 
 */
static int brcmnand_verify_pageoob_priv(struct mtd_info *mtd, loff_t offset, 
	const u_char* fsbuf, int fslen, u_char* oob_buf, int ooblen, struct nand_oobinfo* oobsel, 
	int autoplace, int raw)
{
	//struct brcmnand_chip *this = mtd->priv;
	int ret = 0;
	int complen;

	
	if (autoplace) {

		complen = min_t(int, ooblen, fslen);

		/* We may have read more from the OOB area, so just compare the min of the 2 */
		if (complen == fslen) {
			ret = memcmp(fsbuf, oob_buf, complen);
			if (ret) {
{
printk("Autoplace Comparison failed at %08x, ooblen=%d fslen=%d left=\n", 
	__ll_low(offset), ooblen, fslen);
print_oobbuf(fsbuf, fslen);
printk("\nRight=\n"); print_oobbuf(oob_buf, ooblen);
dump_stack();
}
				goto comparison_failed;
			}
		}
		else {
printk("%s: OOB comparison failed, ooblen=%d is less than fslen=%d\n", 
		__FUNCTION__, ooblen, fslen);
			return  -EBADMSG;
		}
	}
	else { // No autoplace.  Skip over non-freebytes

		/* 
		 * THT:
		 * WIth YAFFS1, the FS codes overwrite an already written chunks quite a lot
		 * (without erasing it first, that is!!!!!)
		 * For those write accesses, it does not make sense to check the write ops
		 * because they are going to fail every time
		 */
		

#if 0
		int i, len; 
		
		for (i = 0; oobsel->oobfree[i][1] && i < ARRAY_SIZE(oobsel->oobfree); i++) {
			int from = oobsel->oobfree[i][0];
			int num = oobsel->oobfree[i][1];
			int len1 = num;

			if (num == 0) break; // End of oobsel
			
			if ((from+num) > fslen) len1 = fslen-from;
			ret = memcmp(&fsbuf[from], &oob_buf[from], len1);
			if (ret) {
				printk(KERN_ERR "%s: comparison at offset=%08x, i=%d from=%d failed., num=%d\n", 
					__FUNCTION__, i, __ll_low(offset), from, num); 
if (gdebug > 3) 
{
printk("No autoplace Comparison failed at %08x, ooblen=%d fslen=%d left=\n", 
	__ll_low(offset), ooblen, fslen);
print_oobbuf(&fsbuf[0], fslen);
printk("\nRight=\n"); print_oobbuf(&oob_buf[0], ooblen);
dump_stack();
}
				goto comparison_failed;
			}
			if ((from+num) >= fslen) break;
			len += num;
		}
#endif
	}
	return ret;


comparison_failed:
	{
		//unsigned long nand_timing1 = brcmnand_ctrl_read(BCHP_NAND_TIMING_1);
		//unsigned long nand_timing2 = brcmnand_ctrl_read(BCHP_NAND_TIMING_2);
		//u_char raw_oob[64];
		//int retlen;
		//struct nand_oobinfo noauto_oobsel;

		printk("Comparison Failed\n");
		print_diagnostics();
		
		//noauto_oobsel = *oobsel;
		//noauto_oobsel.useecc = MTD_NANDECC_PLACEONLY;
		//brcmnand_read_pageoob(mtd, offset, raw_oob, &retlen, &noauto_oobsel, 0, raw);
//if (gdebug) { printk("oob="); print_oobbuf(raw_oob, retlen);}
//printk("<-- %s: comparison failed\n", __FUNCTION__);

	
		return -EBADMSG;
	}
}



/**
 * brcmnand_verify_page - [GENERIC] verify the chip contents after a write
 * @param mtd		MTD device structure
 * @param dbuf		the databuffer to verify
 * @param dlen		the length of the data buffer, and should be less than mtd->oobblock
 * @param fsbuf		the length of the file system OOB data and should be exactly
 *                             mtd-->oobavail (for autoplace) or mtd->oobsize otherise
  *					bytes to verify.
 *
 * Assumes that lock on.  Munges the internal data and OOB buffers.
 */
//#define MYDEBUG
static int brcmnand_verify_page(struct mtd_info *mtd, loff_t addr, const u_char *dbuf, int dlen, 
		u_char* fsbuf, int fslen, struct nand_oobinfo* oobsel, int autoplace
		)
{
	//struct brcmnand_chip *this = mtd->priv;
	u_char data_buf[2048];
	u_char oob_buf [64];
	int ret = 0;
	int ooblen=0, datalen=0;
	//int complen;
	u_char* oobbuf = (fsbuf && fslen > 0) ? oob_buf : NULL;


if (gdebug) printk("%s: oobbuf=%p\n", __FUNCTION__, oobbuf);

	// Must read entire page
	ret = brcmnand_read_page(mtd, addr, data_buf, mtd->oobblock, oobbuf, oobsel, &datalen, &ooblen);
	if (ret) {
		printk(KERN_ERR "%s: brcmnand_read_page at %08x failed ret=%d\n", 
			__FUNCTION__, (unsigned int) addr, ret);
		return ret;
	}
if (gdebug) printk("%s: fsdlen=%d, fslen=%d, datalen=%d, ooblen=%d\n", __FUNCTION__, 
	dlen, fslen, datalen, ooblen);

	/* 
	 * If there are no Input ECC bytes, there is nothing to verify, reading the page and checking the ECC status
	 * flag should be enough
	 */
	if (!fsbuf || fslen <= 0)
		return 0;
	
	// We only verify the ECC bytes.
	ret = brcmnand_verify_pageoob_priv(mtd, addr, fsbuf, fslen, oob_buf, ooblen, oobsel, autoplace, 0);
	if (ret) {
		return ret;
	}

	// but only verify dlen bytes	
	ret = memcmp(data_buf, dbuf, dlen);

	return ret;
}

/**
 * brcmnand_verify_pageoob - [GENERIC] verify the chip contents after a write
 * @param mtd		MTD device structure
 * @param dbuf		the databuffer to verify
 * @param dlen		the length of the data buffer, and should be less than mtd->oobblock
 * @param fsbuf		the file system OOB data 
 * @param fslen		the length of the file system buffer
 * @param oobsel		Specify how to write the OOB data
 * @param autoplace	Specify how to write the OOB data
 * @param raw		Ignore the Bad Block Indicator when true
 *
 * Assumes that lock on.  Munges the OOB internal buffer.
 */
static int brcmnand_verify_pageoob(struct mtd_info *mtd, loff_t addr, const u_char* fsbuf, int fslen,
		struct nand_oobinfo *oobsel, int autoplace, int raw)
{
//	struct brcmnand_chip *this = mtd->priv;
	//u_char* data_buf = this->data_buf;
	u_char oob_buf[64]; // = this->oob_buf;
	int ret = 0;
	//int complen;
	//char tmpfsbuf[64]; // Max oob size we support.
	int ooblen = 0;

if(gdebug) printk("-->%s addr=%08x, fslen=%d, autoplace=%d, raw=%d\n", __FUNCTION__, __ll_low(addr),
	fslen, autoplace, raw);

	// Must read entire page
	ret = brcmnand_read_pageoob(mtd, addr, oob_buf, &ooblen, oobsel, autoplace, raw);

	if (ret) {
		printk(KERN_ERR "%s: brcmnand_read_page at %08x failed ret=%d\n", 
			__FUNCTION__, (unsigned int) addr, ret);
		return ret;
	}

if(gdebug) printk("%s: Calling verify_pageoob_priv(addr=%08x, fslen=%d, ooblen=%d\n", 
	__FUNCTION__, __ll_low(addr), fslen, ooblen);
	ret = brcmnand_verify_pageoob_priv(mtd, addr, fsbuf, fslen, oob_buf, ooblen, oobsel, autoplace, raw);

	return ret;
}



#else
#define brcmnand_verify_page(...)	(0)
#define brcmnand_verify_pageoob(...)		(0)
//#define brcmnand_verify_oob(...)		(0)
#endif

#define NOTALIGNED(x)	((x & (mtd->oobblock - 1)) != 0)



/**
 * brcmnand_read_raw - [PRIVATE] Read raw data including oob into buffer
 * @mtd:	MTD device structure
 * @buf:	temporary buffer
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @ooblen:	number of oob data bytes to read
 *
 * Read raw data including oob into buffer.  Raw Reading ignores any BBT flag.
 */
static int brcmnand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen)
{
	struct brcmnand_chip *this = mtd->priv;
	int slice, ret=0, page;
	int ignoreBBT= 1;

	if (from & (mtd->oobblock - 1)) {
		printk(KERN_ERR "brcmnand_read_raw: from %08lx is not OOB sliceOffset aligned\n", 
			(unsigned long) from);
		return -EINVAL;
	}
	/* len must be multiple of oobblock */
	if (((len / mtd->oobblock) * mtd->oobblock) != len) {
		printk(KERN_ERR "%s: len %08x is not multiple of oobblock %d\n", __FUNCTION__, len, mtd->oobblock);
		return -EINVAL;
	}
	/* ooblen must be multiple of oobsize */
	if (((ooblen / mtd->oobsize) * mtd->oobsize) != ooblen) {
		printk(KERN_ERR "%s: ooblen %08x is not multiple of oobblock %d\n", __FUNCTION__, ooblen, mtd->oobsize);
		return -EINVAL;
	}
	for (page=0; (page * mtd->oobblock) < len; page++) {
		/* For each 2K page, the first 2K are for data (4X512B), followed by 4X64B OOB data */
		L_OFF_T startPage = page*(mtd->oobblock+mtd->oobsize); // Address in buffer corresponding to start of this page
		for (slice = 0; slice < this->eccsteps; slice++) {
			L_OFF_T offset = from + slice*mtd->eccsize; // offset of data w.r.t. flash
			L_OFF_T dataOffset = startPage + slice*mtd->eccsize;
			int oobOffset = startPage + mtd->oobblock + slice*this->eccOobSize; 

			ret = brcmnand_posted_read_cache(mtd, &buf[dataOffset], 
				&buf[oobOffset], offset, ignoreBBT); /* Ignore BBT */
			if (ret) {
				printk(KERN_ERR "%s failed at offset = %08x, dataOffset=%08x, oobOffset=%d\n", 
					__FUNCTION__, __ll_low(offset),  __ll_low(dataOffset),  oobOffset);
				return ret;
			}
		}
		// if partial page, must pad with FF??
	}
	return ret;
	
}


/**
 * brcmnand_write_page - [GENERIC] write one page
 * @mtd:	MTD device structure
 * @offset: 	offset  from start of the chip, must be called with (offset & this->pagemask)
 * @len: size of data buffer, len must be <= mtd->oobblock, and can indicate partial page.
 * @data_buf: data buffer.
 * @oob_buf:	out of band data buffer for 1 page (size = oobsize), must be prepared beforehand
 *
 * Nand_page_program function is used for write and writev !
 * This function will pad the buffer to a Cache boundary before writing it out.
 *
 * Cached programming is not supported yet.
 */
static int brcmnand_write_page (struct mtd_info *mtd, L_OFF_T offset, int len,
	u_char* data_buf, u_char* oob_buf)
{

	struct brcmnand_chip *this = mtd->priv;
	int written = 0, oobWritten = 0;
	int winslice;
	int done = 0;
	int ret;
	L_OFF_T pageOffset, columnOffset;
	int column;
	int padlen = 0, trailer;

	/* Use the passed in buffer */
	this->data_poi = data_buf;
	/* page marks the start of the page encompassing [from, thislen] */
	pageOffset = __ll_and32( offset,  ~ (mtd->oobblock - 1));
	/* column marks the start of the 512B cache slice encompassing [from, thislen] */
	columnOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
	// Make sure that we cast to int as mod op on long long yield unresolved symbols
	column = ((int) __ll_isub(columnOffset , pageOffset)) % mtd->eccsize;
	/* Pad to the front if not starting on cache slice boundary */
	padlen = __ll_isub(offset, columnOffset);
	if (padlen) {
		trailer = min_t(int, len, mtd->oobblock - padlen);

		this->data_poi = this->data_buf;
		memcpy(this->data_buf, ffchars, padlen);
		// Make it simple stupid, just copy the remaining part of a page
		memcpy(&this->data_buf[padlen], data_buf, trailer);
		// Adjust offset accordingly
		// written = - padlen;
	}

char msg[20];
if (gdebug) printk("%s: 30\noffset=%s\n", __FUNCTION__, __ll_sprintf(msg, offset));
	for (winslice = column; winslice < this->eccsteps && written < len && !done; winslice++) {
		int thislen = min_t(int, len-written, mtd->eccsize - padlen);
		//int i;

		// Pad at the end ?
		if (thislen != mtd->eccsize - padlen) {
			int fillupLen = ((padlen+len - written + mtd->eccsize) & ~(mtd->eccsize - 1)) - (padlen+len);
			done = 1; // Writing last slice
			//Pad with 0xff before writing

			memcpy(&this->data_poi[padlen+len], &ffchars[padlen+len], fillupLen);
		}

		if (oob_buf) {
//printk("%s: 35, offset=%s\n", __FUNCTION__, __ll_sprintf(msg, offset));
			ret = brcmnand_posted_write_cache(mtd, &this->data_poi[written], &oob_buf[oobWritten], 
					__ll_add32(offset, written-padlen));
		}
		else {
//printk("%s: 36\n", __FUNCTION__);
			ret = brcmnand_posted_write_cache(mtd, &this->data_poi[written], NULL, 
					__ll_add32(offset, written-padlen));
		}
//printk("%s: 40\n", __FUNCTION__);
		if (ret) {
			printk(/*KERN_ERR */ "%s: posted write cache failed at offset=%08x, ret=%d\n", 
				__FUNCTION__, __ll_low(offset) + written, ret);
			return ret;
		}

		written += thislen;
		if (oob_buf)
			oobWritten += this->eccOobSize;
		padlen = 0;
//printk("%s: 49\n", __FUNCTION__);

	}
if (gdebug) printk("%s: 99\n", __FUNCTION__);
	return 0;
}


/**
 * brcmnand_write_pageoob - [GENERIC] write to one page's OOB area
 * @mtd:	MTD device structure
 * @offset: 	offset  from start of the chip, must be called with (offset & this->pagemask)
 * @len: size of data buffer, len must be <= mtd->oobblock, and can indicate partial page.
 * @data_buf: data buffer.
 * @oob_buf:	out of band data buffer for 1 page (size = oobsize), must be prepared beforehand
 *
 * Nand_page_program function is used for write and writev !
 * This function will always program a full page of data
 * If you call it with a non page aligned buffer, you're lost :)
 *
 * Cached programming is not supported yet.
 */
static int brcmnand_write_pageoob (struct mtd_info *mtd, L_OFF_T offset, int len,
	const u_char* oob_buf)
{
	struct brcmnand_chip *this = mtd->priv;
	int oobWritten = 0;
	int winslice;
	int done = 0;
	int ret;

	if (len > mtd->oobsize) {
		printk(KERN_ERR "%s: len of %d is greater than OOB size %d\n", __FUNCTION__, len, mtd->oobavail);
		return -EINVAL;
	}
	/*
	 * The 4 slice OOB areas may not have the same format, (e.g. for the 2K page, the first slice
	 * lose 2 bytes.)
	 * We have to prepare the OOB buffer for the entire page, then write it out one slice at a time.
	 */
	for (winslice = 0; winslice < this->eccsteps && oobWritten < len && !done; winslice++) {
		L_OFF_T sliceOffset = offset+mtd->eccsize*winslice;
		
		ret = brcmnand_posted_write_oob(mtd, &oob_buf[oobWritten], sliceOffset);
		if (ret) {
			printk(KERN_ERR "%s: posted write cache failed at offset=%08x, ret=%d\n", 
				__FUNCTION__, __ll_low(sliceOffset), ret);
			return ret;
		}
		oobWritten += this->eccOobSize;
	}

	return 0;
}



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
	struct brcmnand_chip *this = mtd->priv;
	int page, ret = -EIO, oob = 0, written = 0, eccWritten = 0;
	int autoplace = 0, totalpages;
	u_char *oobbuf;
	//int	ppblock = (1 << (this->phys_erase_shift - this->page_shift));
	loff_t startPageOffset; 
	u_char oob_buf[64];
	int eccRetLen = 0;

	char msg[20];
	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_write_ecc: to = 0x%s, len = %i\n", __ll_sprintf(msg,to), (int) len);
if (gdebug > 3) printk("brcmnand_write_ecc: to = 0x%s, len = %i\n", __ll_sprintf(msg,to), (int) len);
	autoplace = brcmnand_autoplace(mtd, oobsel);
if (gdebug > 3) printk("%s: autoplace=%d\n", __FUNCTION__, autoplace);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow writes past end of device */
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely(to + len) > mtd->size) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(to, len), this->mtdSize)))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_ecc: Attempt write to past end of device\n");
		return -EINVAL;
	}
//printk("%s: 20\n", __FUNCTION__);
	/* Reject writes, which are not page aligned */
        if (unlikely(NOTALIGNED(__ll_low(to))) || unlikely(NOTALIGNED(len))) {
                DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_write_ecc: Attempt to write not page aligned data\n");
                return -EINVAL;
        }

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_WRITING);


	/* Setup variables and oob buffer */
	totalpages = len >> this->page_shift;
	page = (uint32_t) __ll_RightShift(to , this->page_shift);
	startPageOffset = __ll_and32(to, ~(this->pageSize-1)); // (loff_t) (page << this->page_shift);
	
	/* Invalidate the page cache, if we write to the cached page */
	//if (page <= this->pagebuf && this->pagebuf < (page + totalpages))
	//	this->pagebuf = -1;
	eccWritten = 0;

	/*
	 * Write the part before page boundary.
	 */
//printk("%s: 30\n", __FUNCTION__);
	if (__ll_isub(startPageOffset,  to)) {
		// How many bytes will need to be written for partial page write.
		int preface = (int)(to - startPageOffset);
		int thislen = min_t(int, len - written, mtd->oobblock - preface);
		u_char pagebuf[2048];
		
		/* Make a page, fill it up with 0xFF up to where the actual buffer starts */
		memcpy(pagebuf, ffchars, preface);
		memcpy(&pagebuf[preface], buf, thislen);
		this->data_poi = (u_char*) &pagebuf[0];

		/* We dont write ECC to a partial page */
		if (eccbuf) {
			printk("%s: OOB buffer ignored in partial page write at offset %08x\n", __FUNCTION__, (unsigned int) to);
		}
		oobbuf = NULL;
		
		/* Write one page. If this is the last page to write
		 * or the last page in this block, then use the
		 * real pageprogram command, else select cached programming
		 * if supported by the chip.
		 */

		ret = brcmnand_write_page (mtd, startPageOffset, mtd->oobblock, this->data_poi, oobbuf);
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: write_page failed %d\n", ret);
			goto out;
		}

		/* We can't verify a partial page write.
		 * ret = brcmnand_verify_page (mtd, (loff_t) (page << this->page_shift), 
				&buf[written], thislen, oobbuf, eccRetLen, oobsel, autoplace

		 *		);
		 */

		
		/* Update written bytes count */
		written = thislen;
		*retlen = written;
		page++;
		startPageOffset = __ll_add32(startPageOffset, mtd->oobblock);
	}

//printk("%s: 50\n", __FUNCTION__);
	/* Loop until all data are written */
	while (written < len) {
		int thislen = min_t(int, len - written, mtd->oobblock);

		eccRetLen = 0;

		this->data_poi = (u_char*) &buf[written];

		if (eccbuf) {
			oobbuf = brcmnand_prepare_oobbuf (mtd, 
						&eccbuf[eccWritten], mtd->oobsize, oob_buf, oobsel, autoplace, &eccRetLen);

			oob = 0;
		}
		else {
			oobbuf = NULL;
		}

if (gdebug) printk("%s: 60, startPage=%s\n", __FUNCTION__, __ll_sprintf(msg, startPageOffset));
		ret = brcmnand_write_page (mtd, startPageOffset, thislen, this->data_poi, oobbuf);
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: write_page failed %d\n", ret);
			goto out;
		}

if (gdebug) printk("%s: Verify page, oobarea=%p, autoplace=%d\n", __FUNCTION__, oobbuf, autoplace);
		ret = brcmnand_verify_page (mtd, startPageOffset, 
				&buf[written], thislen, &eccbuf[eccWritten], eccRetLen, oobsel, autoplace
				);

		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: verify_pages failed %d @%08x, written=%d\n", __FUNCTION__, ret, (unsigned long) to, written);
			goto out;
		}
//printk("%s: 70\n", __FUNCTION__);
		/* Next oob page */
		oob += mtd->oobsize;
		/* Update written bytes count */
		written += mtd->oobblock;
		// if autoplace, count only the free bytes, otherwise, must count the entire OOB size
		eccWritten += autoplace? eccRetLen :mtd->oobsize;

		*retlen = written;

		/* Increment page address */
		page++;
		startPageOffset = __ll_add32(startPageOffset, mtd->oobblock);
	}
	/* Verify the remaining pages */

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);
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
	DEBUG(MTD_DEBUG_LEVEL3, "-->%s to=%08x, len=%d\n", __FUNCTION__,
 		(unsigned long) to, (int) len);
	return brcmnand_write_ecc(mtd, to, len, retlen, buf, NULL, NULL);
}


/*
 * Allow nested calls inside this module in order to avoid having to release-and-reacquire lock.  
 * Take same arguments as the public interface.
 * Used mostly by BBT routines.
 */
static int brcmnand_write_oob_int(struct mtd_info *mtd, loff_t to, size_t ooblen, size_t* retlen,
	const u_char *oob_buf)
{
	struct brcmnand_chip *this = mtd->priv;
	int column; 
	L_OFF_T pageAddr;
	int ret = -EIO, oob = 0, eccWritten = 0;
	int autoplace = 0;
	//int chipnr;
	//u_char *oobbuf, *bufstart;
	struct nand_oobinfo oobsel;

	oobsel = mtd->oobinfo;
	oobsel.useecc = MTD_NANDECC_PLACE;

	//DEBUG(MTD_DEBUG_LEVEL3, "%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) ooblen);

	/* Shift to get page */
	//page = (int) __ll_RightShift(to, this->page_shift);
	pageAddr = __ll_and32(to, ~(mtd->oobblock - 1));
	
	//chipnr = (int) (to >> this->chip_shift);

	/* Mask to get column */
	column = __ll_low(__ll_and32(to, (mtd->oobsize - 1)));

	/* Initialize return length value */
	*retlen = 0;

	if ((ooblen % mtd->oobsize) != 0) {
		printk(KERN_WARNING "%s called at offset %08x with len(%d) not multiple of oobsize(%d)\n", 
			__FUNCTION__, __ll_low(to), ooblen, mtd->oobsize);
	}

	/* Do not allow writes past end of device */


#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely((to + ooblen) > (mtd->size+mtd->oobsize))) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(to, ooblen), __ll_add32(this->mtdSize, mtd->oobsize))))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt write to past end of device\n", __FUNCTION__);
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	// brcmnand_get_device(mtd, FL_WRITING);

	eccWritten = 0; // Dest offset
	oob = 0; // SOurce offset
	
	/* Loop until all data are written */
	while (eccWritten < ooblen) {
		int thislen = min_t(int, ooblen - eccWritten, mtd->oobsize);
		
		/* Write one page. If this is the last page to write
		 * or the last page in this block, then use the
		 * real pageprogram command, else select cached programming
		 * if supported by the chip.
		 */
if (gdebug) printk("Calling brcmnand_write_pageoob oob=%d addr=%08x thislen=%d\n", 
	oob, __ll_low(pageAddr), thislen);
		// No autoplace when this routine is called.
		ret = brcmnand_write_pageoob (mtd, pageAddr, thislen, &oob_buf[oob]);
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: write_pageoob failed %d\n", __FUNCTION__, ret);
			goto out;
		}

		ret = brcmnand_verify_pageoob (mtd, pageAddr, &oob_buf[oob], thislen, &oobsel, autoplace, 1);
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: verify_pages failed %d\n", __FUNCTION__, ret);
			goto out;
		}
		/* Next oob page */
		oob += mtd->oobsize;
		/* Update written bytes count */
		eccWritten += mtd->oobsize;

		*retlen += mtd->oobsize;

		/* Increment page address */
		pageAddr = __ll_add32(pageAddr, mtd->oobblock);
	}
	/* Verify the remaining pages */

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	return ret;
}


/**
 * brcmnand_write_oob - [MTD Interface] BrcmNAND write out-of-band
 * @param mtd		MTD device structure
 * @param to			offset to write to (of the page, not the oob).
 * @param ooblen		number of OOB bytes to write
 * @param oob_buf	the data to write must be formatted beforehand
 *
 * BrcmNAND write out-of-band
 */
static int brcmnand_write_oob(struct mtd_info *mtd, loff_t to, size_t ooblen, size_t* retlen,
	const u_char *oob_buf)
{
	int ret;

	DEBUG(MTD_DEBUG_LEVEL3, "%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) ooblen);

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_WRITING);

	ret = brcmnand_write_oob_int(mtd, to, ooblen, retlen, oob_buf);
	
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	return ret;
}

/**
 * brcmnand_write_oobfree - [MTD Interface] NAND write free out-of-band
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 *
 * NAND write free out-of-band area
 */
static int brcmnand_write_oobfree (struct mtd_info *mtd, 
		loff_t to, size_t len, size_t * retlen, const u_char * oob_buf)
{
	struct brcmnand_chip *this = mtd->priv;
	int column; 
	int ret = -EIO, oob = 0, written = 0, eccWritten = 0;
	L_OFF_T pageAddr;
	int autoplace = 1;
	u_char *oobbuf;
	struct nand_oobinfo* oobsel = this->autooob;

	DEBUG(MTD_DEBUG_LEVEL3, "\n%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) len);

	/* Shift to get page */
	//page = (int) (to >> this->page_shift);
	pageAddr = __ll_and32(to, ~(mtd->oobblock-1));
	
	//chipnr = (int) (to >> this->chip_shift);

	/* Mask to get column */
	column = to & (mtd->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow writes past end of device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely((to + len) > (mtd->size+mtd->oobsize))) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(to, len), __ll_add32(this->mtdSize, mtd->oobsize))))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "\n%s: Attempt write to past end of device, to=%08x, len=%d, size=%08x\n", 
			__FUNCTION__, (unsigned int) to, (unsigned int) len, mtd->size);
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_WRITING);

	/* Invalidate the page cache, if we write to the cached page */
	//if (page == this->pagebuf)
	//	this->pagebuf = -1;

	eccWritten = 0;
	
	/* Loop until all data are written */
	while (eccWritten < len) {
		int eccRetLen = 0;
		int thislen = min_t(int, len - eccWritten, mtd->oobavail);
		u_char tmp_oob[64];


		oobbuf = brcmnand_prepare_oobbuf (mtd, &oob_buf[eccWritten], thislen, tmp_oob, oobsel, 1, &eccRetLen);
		oob = 0;


		/* Write one page. If this is the last page to write
		 * or the last page in this block, then use the
		 * real pageprogram command, else select cached programming
		 * if supported by the chip.
		 */

		/* We write out the entire OOB buffer, regardless of thislen */
		ret = brcmnand_write_pageoob (mtd, pageAddr, mtd->oobsize, &oobbuf[oob]);

		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: write_pageoob failed %d\n", __FUNCTION__, ret);
			goto out;
		}


		ret = brcmnand_verify_pageoob (mtd, pageAddr, &oob_buf[eccWritten], eccRetLen, 
			oobsel, autoplace, 0);

		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: verify_pages failed %d\n", __FUNCTION__, ret);
			goto out;
		}
		eccWritten += eccRetLen;
		/* Next oob page */
		oob += mtd->oobavail;
		/* Update written bytes count */
		eccWritten += mtd->oobavail;
		
		*retlen = written;

		/* Increment page address */
		pageAddr = __ll_add32(pageAddr, mtd->oobblock);
	}
	/* Verify the remaining pages TBD */
	ret = 0;

out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	return ret;
}

/**
 * brcmnand_writev_ecc - [MTD Interface] write with iovec with ecc
 * @param mtd		MTD device structure
 * @param vecs		the iovectors to write
 * @param count		number of vectors
 * @param to		offset to write to
 * @param retlen	pointer to variable to store the number of written bytes
 * @param eccbuf	filesystem supplied oob data buffer 
 * @param oobsel	oob selection structure  
 *
 * BrcmNAND write with iovec with ecc
 */
static int brcmnand_writev_ecc(struct mtd_info *mtd, const struct kvec *vecs,
	unsigned long count, loff_t to, size_t *retlen,
	u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	int i, len, total_len, ret = -EIO, written = 0, oobretlen, buflen;
	L_OFF_T pageAddr;
	int numpages, autoplace = 0, oobWritten = 0;
	struct brcmnand_chip *this = mtd->priv;
	//int	ppblock = (1 << (this->phys_erase_shift - this->page_shift));
	u_char *oobbuf, *bufstart = NULL;
	u_char tmp_oob[64];
	u_char data_buf[2048];


	/* Preset written len for early exit */
	*retlen = 0;

	/* Calculate total length of data */
	total_len = 0;
	for (i = 0; i < count; i++)
		total_len += vecs[i].iov_len;

	DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_writev_ecc: to = 0x%08x, len = %i, count = %ld, eccbuf=%p, total_len=%d\n", 
	(unsigned int) to, (unsigned int) total_len, count, eccbuf, total_len);

	/* Do not allow write past end of the device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
	if (unlikely(to + total_len) > mtd->size) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(to, total_len), this->mtdSize)))
#endif
	{
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

	autoplace = brcmnand_autoplace(mtd, oobsel);

	/* Setup start page, we know that to is aligned on page boundary */
	pageAddr = to;

	/* Loop until all keve's data has been written */
	len = 0; 		// How many data from current iovec has been written
	written = 0;	// How many bytes have been written so far in all
	oobWritten = 0; // How many oob bytes written in all
	buflen = 0;	// How many bytes from the buffer has been copied to.
	while (count) {
		/* If the given tuple is >= pagesize then
		 * write it out from the iov
		 */
		// THT: We must also account for the partial buffer left over from previous iovec
		if ((buflen + vecs->iov_len - len) >= mtd->oobblock) {
			/* Calc number of pages we can write
			 * out of this iov in one go */
			numpages = (buflen + vecs->iov_len - len) >> this->page_shift;


			//oob = 0;
			for (i = 0; i < numpages; i++) {
				if (0 == buflen) { // If we start a new page
					bufstart = &((u_char *)vecs->iov_base)[len];
				}
				else { // Reuse existing partial buffer, partial refill to end of page
					memcpy(&bufstart[buflen], &((u_char *)vecs->iov_base)[len], mtd->oobblock - buflen);
				}

				/* Write one page. If this is the last page to write
				 * then use the real pageprogram command, else select
				 * cached programming if supported by the chip.
				 */
				oobbuf = brcmnand_prepare_oobbuf (mtd, &eccbuf[oobWritten], 
					autoplace? mtd->oobsize: mtd->oobavail, 
					tmp_oob, oobsel, autoplace, &oobretlen);

				ret = brcmnand_write_page (mtd, pageAddr, mtd->oobblock, bufstart, oobbuf);
				bufstart = NULL;

				if (ret) {
					printk("%s: brcmnand_write_page failed, ret=%d\n", __FUNCTION__, ret);
					goto out;
				}
				len += mtd->oobblock - buflen;
				buflen = 0;
				//oob += oobretlen;
				pageAddr = __ll_add32(pageAddr, mtd->oobblock);
				written += mtd->oobblock;
				oobWritten += oobretlen;
			}
			/* Check, if we have to switch to the next tuple */
			if (len >= (int) vecs->iov_len) {
				vecs++;
				len = 0;
				count--;
			}
		} else { // (vecs->iov_len - len) <  mtd->oobblock)
			/*
			 * We must use the internal buffer, read data out of each
			 * tuple until we have a full page to write
			 */
			


#if 0
			int cnt = 0;
			while (count && cnt < mtd->oobblock) {
				if (vecs->iov_base != NULL && vecs->iov_len)
					data_buf[cnt++] = ((u_char *) vecs->iov_base)[len++];
				/* Check, if we have to switch to the next tuple */
				if (len >= (int) vecs->iov_len) {
					vecs++;
					len = 0;
					count--;
				}
#else
			/*
			 * THT: Changed to use memcpy which is more efficient than byte copying, does not work yet
			 *  Here we know that 0 < vecs->iov_len - len < mtd->oobblock, and len is not necessarily 0
			 */
			// While we have iovec to write and a partial buffer to fill
			while (count && (buflen < mtd->oobblock)) {
				
				// Start new buffer?
				if (0 == buflen) {
					bufstart = data_buf;
				}
				if (vecs->iov_base != NULL && (vecs->iov_len - len) > 0) {
					// We fill up to the page
					int fillLen = min_t(int, vecs->iov_len - len, mtd->oobblock - buflen);
					
					memcpy(&data_buf[buflen], &((u_char*) vecs->iov_base)[len], fillLen);
					buflen += fillLen;
					len += fillLen;
				}
				/* Check, if we have to switch to the next tuple */
				if (len >= (int) vecs->iov_len) {
					vecs++;
					len = 0;
					count--;
				}
#endif
			}
			// Write out a full page if we have enough, otherwise loop back to the top
			if (buflen == mtd->oobblock) {
				numpages = 1;
				oobbuf = brcmnand_prepare_oobbuf (mtd, eccbuf, 
					autoplace? mtd->oobsize: mtd->oobavail, 
					tmp_oob, oobsel, autoplace, &oobretlen);

				ret = brcmnand_write_page (mtd, pageAddr, mtd->oobblock, bufstart, oobbuf);
				if (ret) {
					printk("%s: brcmnand_write_page failed, ret=%d\n", __FUNCTION__, ret);
					goto out;
				}
				pageAddr = __ll_add32(pageAddr, mtd->oobblock);
				written += mtd->oobblock;
				oobWritten += oobretlen;
				bufstart = NULL;
				buflen = 0;
			}
		}

		/* All done ? */
		if (!count) {
			if (buflen) { // Partial page left un-written.  Imposible, as we check for totalLen being multiple of pageSize above.
				printk("%s: %d bytes left unwritten with writev_ecc at offset %08x\n", 
					__FUNCTION__, buflen, __ll_low(pageAddr));
				BUG();
			}
			break;
		}

#if 0
		/* Check, if we cross a chip boundary */
		if (!startpage) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
#endif
	}
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	brcmnand_release_device(mtd);

	*retlen = written;
//if (*retlen <= 0) printk("%s returns retlen=%d, ret=%d, startAddr=%08x\n", __FUNCTION__, *retlen, ret, startAddr);
//printk("<-- %s: retlen=%d\n", __FUNCTION__, *retlen);
	return ret;
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
	DEBUG(MTD_DEBUG_LEVEL3, "-->%s to=%08x, count=%d\n", __FUNCTION__,
 		(unsigned long) to, (int) count);
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
	int res;

	if (getchip) {
		brcmnand_get_device(mtd, FL_READING);
	}
	
	/* Return info from the table */
	res = this->isbad_bbt(mtd, ofs, allowbbt);

	if (getchip) {
		brcmnand_release_device(mtd);
	}

	return res;
}

/**
 * brcmnand_erase_bbt - [Private] erase block(s)
 * @param mtd		MTD device structure
 * @param instr		erase instruction
 * @allowBBT			allow erase of BBT
 *
 * Erase one ore more blocks
 * ** FIXME ** This code does not work for multiple chips that span an address space > 4GB
 */
static int brcmnand_erase_bbt(struct mtd_info *mtd, struct erase_info *instr, int allowbbt, int doNotUseBBT)
{
	struct brcmnand_chip *this = mtd->priv;
	unsigned int block_size;
	loff_t addr;
	int len;
	int ret = 0;
	int needBBT;
	
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	// For now ignore the high DW.
	L_OFF_T instr_addr = __ll_constructor(0, instr->addr);
	instr->addr_hi = 0;
#endif
	

	DEBUG(MTD_DEBUG_LEVEL3, "%s: start = 0x%08x, len = %08x\n", __FUNCTION__, (unsigned int) instr->addr, (unsigned int) instr->len);
//printk( "%s: start = 0x%08x, len = %08x\n", __FUNCTION__, (unsigned int) instr->addr, (unsigned int) instr->len);

	block_size = (1 << this->erase_shift);


	/* Start address must align on block boundary */
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
	if (unlikely(instr->addr & (block_size - 1))) 
#else
	if (unlikely(!__ll_is_zero(__ll_and32(instr_addr, (block_size - 1)))))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Unaligned address\n", __FUNCTION__);
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (unlikely(instr->len & (block_size - 1))) 
	{
		DEBUG(MTD_DEBUG_LEVEL0, 
"%s: Length not block aligned, len=%08x, blocksize=%08x, chip->blkSize=%08x, chip->erase=%d\n",
__FUNCTION__, instr->len, block_size, this->blockSize, this->erase_shift);
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if (unlikely((instr->len + instr->addr) > mtd->size)) 
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
	if (unlikely((instr->len + instr->addr) > mtd->size)) 
#else
	if (unlikely(__ll_is_greater(__ll_add32(instr_addr, instr->len), this->mtdSize)))
#endif
	{
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Erase past end of device\n", __FUNCTION__);
		return -EINVAL;
	}

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
	instr->fail_addr = 0xffffffff;
#else
	instr->fail_addr = instr->fail_addr_hi = 0xffffffff;
#endif

	/* Grab the lock and see if the device is available */
	brcmnand_get_device(mtd, FL_ERASING);

	/*
	 * Clear ECC registers 
	 */
	this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);
	this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);
	
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	this->ctrl_write(BCHP_NAND_ECC_CORR_EXT_ADDR, 0);
	this->ctrl_write(BCHP_NAND_ECC_UNC_EXT_ADDR, 0);
#endif

	/* Loop throught the pages */
	len = instr->len;

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
	addr = instr->addr;
#else
	addr = instr_addr;
#endif

	instr->state = MTD_ERASING;

	while (len) {


/* THT: We cannot call brcmnand_block_checkbad which just look at the BBT,
// since this code is also called when we create the BBT.
// We must look at the actual bits, or have a flag to tell the driver
// to read the BI directly from the OOB, bypassing the BBT
 */
		/* Check if we have a bad block, we do not erase bad blocks */
		if (brcmnand_block_checkbad(mtd, addr, 0, allowbbt)) {
			printk (KERN_ERR "%s: attempt to erase a bad block at addr 0x%08x\n", __FUNCTION__, (unsigned int) addr);
			instr->state = MTD_ERASE_FAILED;
			goto erase_one_block;
		}

		//this->command(mtd, ONENAND_CMD_ERASE, addr, block_size);

		this->ctrl_writeAddr(this, addr, 0);
	
		this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCK_ERASE);

		// Wait until flash is ready
		ret = brcmnand_write_is_complete(mtd, &needBBT);
		
		/* Check, if it is write protected: TBD */
		if (needBBT ) {
			if ( !allowbbt) {
				printk(KERN_ERR "brcmnand_erase: Failed erase, block %d, flash status=%08x\n", 
					__ll_low(__ll_RightShift(addr, this->erase_shift)), needBBT);
				instr->state = MTD_ERASE_FAILED;

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
				instr->fail_addr = addr;
#else
				instr->fail_addr = __ll_low(addr);
				instr->fail_addr_hi = __ll_high(addr);
#endif

				printk(KERN_WARNING "%s: Marking bad block @%08x\n", __FUNCTION__, (unsigned int) addr);
				(void) this->block_markbad(mtd, addr);
				goto erase_one_block;
			}
		}
erase_one_block:

		len -= block_size;
		addr = __ll_add32(addr, block_size);
	}

	instr->state = MTD_ERASE_DONE;

//erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
	/* Do call back function */
	if (!ret) {
		mtd_erase_callback(instr);
	}


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
	DEBUG(MTD_DEBUG_LEVEL3, "-->%s addr=%08x, len=%d\n", __FUNCTION__,
 		(unsigned long) instr->addr, instr->len);
	return brcmnand_erase_bbt(mtd, instr, 0, 0); // Do not allow erase of BBT, and use BBT
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

	PLATFORM_IOFLUSH_WAR();

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
	struct brcmnand_chip *this = mtd->priv;
	
	DEBUG(MTD_DEBUG_LEVEL3, "-->%s ofs=%08x\n", __FUNCTION__, __ll_low(ofs));
	/* Check for invalid offset */
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
	if (ofs > mtd->size)
#else
	if (__ll_is_greater(ofs, this->mtdSize))
#endif
	{
		return -EINVAL;
	}

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
	// THT: 3/20/07: We restrict ourselves to only support x8
	u_char bbmarker[1] = {0x0};  // CFE and BBS uses 0x0F, Linux by default uses 0
								//so we can use this to mark the difference
	u_char buf[64];
	size_t retlen;
	int block;
	UL_OFF_T ulofs;

	ulofs = (UL_OFF_T)__ll_and32(ofs, ~ this->page_mask);
	

	/* Get block number.  Block is guaranteed to be < 2*32 */
	block = __ll_low(__ll_RightShift(ulofs, this->erase_shift));
      if (this->bbt) {
                this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1); 
      	}

	memcpy(buf, ffchars, sizeof(buf));
	memcpy(&buf[this->badblockpos], bbmarker, sizeof(bbmarker));

	// Lock already held by caller, so cant call mtd->write_oob directly
       return brcmnand_write_oob_int(mtd, ulofs, sizeof(buf), &retlen, buf);
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

	DEBUG(MTD_DEBUG_LEVEL3, "-->%s ofs=%08x\n", __FUNCTION__, (unsigned int) ofs);
	
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
	UL_OFF_T blkAddr, ofs = (UL_OFF_T) llofs;

	DEBUG(MTD_DEBUG_LEVEL3, "-->%s llofs=%08x, len=%d\n", __FUNCTION__,
 		(unsigned long) llofs, (int) len);



	/* Block lock scheme */
	for (blkAddr = ofs; __ll_is_less(blkAddr, __ll_add32(ofs, len)); blkAddr = __ll_add32(blkAddr, this->blockSize)) {

		/* The following 2 commands share the same CMD_EXT_ADDR, as the block never cross a CS boundary */
		this->ctrl_writeAddr(this, blkAddr, 0); 
		/* Set end block address */
		this->ctrl_writeAddr(this, __ll_add32(blkAddr, this->blockSize - 1), 1);
		/* Write unlock command */
		this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCKS_UNLOCK);


		/* There's no return value */
		this->wait(mtd, FL_UNLOCKING, &status);

		if (status & 0x0f)  
			printk(KERN_ERR "block = %08x, wp status = 0x%x\n", __ll_low(blkAddr), status);

		/* Sanity check */
#if 0
		while (this->read_word(this->base + ONENAND_REG_CTRL_STATUS)
		    & ONENAND_CTRL_ONGO)
			continue;
#endif

		/* Check lock status */
		this->ctrl_writeAddr(this, blkAddr, 0); 
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

	print_config_regs();

}



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
		case 1:	
			chip->blockSize = 128 << 10;
			break;
		case 0:
			chip->blockSize = 16 << 10;
			break;
	}
	chip->erase_shift = ffs(chip->blockSize) - 1;

	switch((nand_config & 0x40000000) >> 30) {
		case 0:
			chip->pageSize= 512;
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

/*
 * Adjust timing pattern if specified in chip ID list
 * Also take dummy entries, but no adjustments will be made.
 */
static void brcmnand_adjust_timings(struct brcmnand_chip *this, brcmnand_chip_Id* chip)
{
		unsigned long nand_timing1 = this->ctrl_read(BCHP_NAND_TIMING_1);
		unsigned long nand_timing1_b4;
		unsigned long nand_timing2 = this->ctrl_read(BCHP_NAND_TIMING_2);
		unsigned long nand_timing2_b4;
		
	// Adjust NAND timings:
	if (chip->timing1) {
		nand_timing1_b4 = nand_timing1;

		if (chip->timing1 & BCHP_NAND_TIMING_1_tWP_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tWP_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tWP_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tWH_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tWH_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tWH_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tRP_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tRP_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tRP_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tREH_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tREH_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tREH_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tCS_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tCS_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tCS_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tCLH_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tCLH_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tCLH_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tALH_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tALH_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tALH_MASK);  
		}
		if (chip->timing1 & BCHP_NAND_TIMING_1_tADL_MASK) {
			nand_timing1 &= ~BCHP_NAND_TIMING_1_tADL_MASK;
			nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tADL_MASK);  
		}

		this->ctrl_write(BCHP_NAND_TIMING_1, nand_timing1);
printk("Adjust timing1: Was %08lx, changed to %08lx\n", nand_timing1_b4, nand_timing1);
	}
	else {
printk("timing1 not adjusted: %08lx\n", nand_timing1);
	}

	// Adjust NAND timings:
	if (chip->timing2) {
		nand_timing2_b4 = nand_timing2;

		if (chip->timing2 & BCHP_NAND_TIMING_2_tWB_MASK) {
			nand_timing2 &= ~BCHP_NAND_TIMING_2_tWB_MASK;
			nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_tWB_MASK);  
		}
		if (chip->timing2 & BCHP_NAND_TIMING_2_tWHR_MASK) {
			nand_timing2 &= ~BCHP_NAND_TIMING_2_tWHR_MASK;
			nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_tWHR_MASK);  
		}
		if (chip->timing2 & BCHP_NAND_TIMING_2_tREAD_MASK) {
			nand_timing2 &= ~BCHP_NAND_TIMING_2_tREAD_MASK;
			nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_tREAD_MASK);  
		}

		this->ctrl_write(BCHP_NAND_TIMING_2, nand_timing2);
printk("Adjust timing2: Was %08lx, changed to %08lx\n", nand_timing2_b4, nand_timing2);
	}
	else {
printk("timing2 not adjusted: %08lx\n", nand_timing2);
	}
}

static void 
brcmnand_read_id(struct mtd_info *mtd, unsigned int chipSelect, unsigned long* dev_id)
{
	struct brcmnand_chip *this = mtd->priv;
	
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	/* Set correct chip Select */
	this->ctrl_write(BCHP_NAND_CMD_ADDRESS, BCHP_NAND_CMD_START_OPCODE_DEVICE_ID_READ);
	this->ctrl_write(BCHP_NAND_CMD_EXT_ADDRESS, chipSelect << 16);
#endif

PRINTK("-->%s: this=%p, this->ctrl_read=%p\n", __FUNCTION__, this, this->ctrl_read);

	/* Send the command for reading device ID from controller */
	*dev_id = this->ctrl_read(BCHP_NAND_FLASH_DEVICE_ID);

	printk(KERN_INFO "brcmnand_probe: CS%1d: dev_id=%08x\n", chipSelect, (unsigned int) *dev_id);

}


/**
 * brcmnand_probe - [BrcmNAND Interface] Probe the BrcmNAND device
 * @param mtd		MTD device structure
 *
 * BrcmNAND detection method:
 *   Compare the the values from command with ones from register
 */
static int brcmnand_probe(struct mtd_info *mtd, unsigned int chipSelect)
{
	struct brcmnand_chip *this = mtd->priv;
	unsigned char brcmnand_maf_id, brcmnand_dev_id;
	uint32_t nand_config;
	int version_id;
	//int density;
	int i;

	/* Read manufacturer and device IDs from Controller */
	brcmnand_read_id(mtd, chipSelect, &this->device_id);

	brcmnand_maf_id = (this->device_id >> 24) & 0xff;
	brcmnand_dev_id = (this->device_id >> 16) & 0xff;

	/* Look up in our table for infos on device */
	for (i=0; i < BRCMNAND_MAX_CHIPS; i++) {
		if (brcmnand_dev_id == brcmnand_chips[i].chipId 
			&& brcmnand_maf_id == brcmnand_chips[i].mafId)
			break;
	}

	if (i >= BRCMNAND_MAX_CHIPS) {
#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_0
		printk(KERN_ERR "DevId %08x may not be supported\n", (unsigned int) this->device_id);
		/* Because of the bug in the controller in the first version,
		 * if we can't identify the chip, we punt
		 */
		return (-EINVAL);
#else
		printk(KERN_WARNING"DevId %08x may not be supported.  Will use config info\n", (unsigned int) this->device_id);
#endif
	}

	nand_config = this->ctrl_read(BCHP_NAND_CONFIG);

#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_0
	// Workaround for bug in 7400A0 returning invalid config
	switch(i) { 
	case 0: /* SamSung NAND 1Gbit */
	case 1: /* ST NAND 1Gbit */
	case 4:
	case 5:
		/* Large page, 128K erase block
		PAGE_SIZE = 0x1 = 1b = PG_SIZE_2KB
		BLOCK_SIZE = 0x1 = 01b = BK_SIZE_128KB
		DEVICE_SIZE = 0x5 = 101b = DVC_SIZE_128MB
		DEVICE_WIDTH = 0x0 = 0b = DVC_WIDTH_8
		FUL_ADR_BYTES = 5 = 101b
		COL_ADR_BYTES = 2 = 010b
		BLK_ADR_BYTES = 3 = 011b
		*/
		nand_config &= ~0x30000000;
		nand_config |= 0x10000000; // bit 29:28 = 1 ===> 128K erase block
		//nand_config = 0x55042200; //128MB, 0x55052300  for 256MB
		this->ctrl_write(BCHP_NAND_CONFIG, nand_config);

		break;

	case 2:
	case 3:
		/* Small page, 16K erase block
		PAGE_SIZE = 0x0 = 0b = PG_SIZE_512B
		BLOCK_SIZE = 0x0 = 0b = BK_SIZE_16KB
		DEVICE_SIZE = 0x5 = 101b = DVC_SIZE_128MB
		DEVICE_WIDTH = 0x0 = 0b = DVC_WIDTH_8
		FUL_ADR_BYTES = 5 = 101b
		COL_ADR_BYTES = 2 = 010b
		BLK_ADR_BYTES = 3 = 011b
		*/
		nand_config &= ~0x70000000;
		this->ctrl_write(BCHP_NAND_CONFIG, nand_config);

		break;
	
	default:
		printk(KERN_ERR "%s: DevId %08x not supported\n", __FUNCTION__, (unsigned int) this->device_id);
		BUG();
		break;
	}
#endif
	brcmnand_decode_config(this, nand_config);

	// Also works for dummy entries, but no adjustments possible
	brcmnand_adjust_timings(this, &brcmnand_chips[i]);

	/* Flash device information */
	brcmnand_print_device_info(&brcmnand_chips[i], this->chipSize);
	this->options = brcmnand_chips[i].options;
		
	/* BrcmNAND page size & block size */	
	mtd->oobblock = this->pageSize; 		
	mtd->oobsize = mtd->oobblock >> 5; // tht - 16 byte OOB for 512B page, 64B for 2K page
	mtd->erasesize = this->blockSize;

	/* Fix me: When we have both a NOR and NAND flash on board */
	/* For now, we will adjust the mtd->size for version 0.0 and 0.1 later in scan routine */
	this->mtdSize = __ll_mult32(this->chipSize, this->numchips);
	mtd->size = __ll_low(this->mtdSize);
	mtd->size_hi = __ll_high(this->mtdSize);

	/* Version ID */
	version_id = this->ctrl_read(BCHP_NAND_REVISION);
	printk(KERN_INFO "BrcmNAND version = 0x%04x %dMB @%p\n", 
		version_id, this->chipSize>>20, this->vbase);

	return 0;
}

/**
 * brcmnand_suspend - [MTD Interface] Suspend the BrcmNAND flash
 * @param mtd		MTD device structure
 */
static int brcmnand_suspend(struct mtd_info *mtd)
{
	DEBUG(MTD_DEBUG_LEVEL3, "-->%s  \n", __FUNCTION__);
	return brcmnand_get_device(mtd, FL_PM_SUSPENDED);
}

/**
 * brcmnand_resume - [MTD Interface] Resume the BrcmNAND flash
 * @param mtd		MTD device structure
 */
static void brcmnand_resume(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;

	DEBUG(MTD_DEBUG_LEVEL3, "-->%s  \n", __FUNCTION__);
	if (this->state == FL_PM_SUSPENDED)
		brcmnand_release_device(mtd);
	else
		printk(KERN_ERR "resume() called for the chip which is not"
				"in suspended state\n");
}


/**
 * fill_autooob_layout - [NAND Interface] build the layout for hardware ECC case
 * @mtd:	MTD device structure
 * @eccbytes:	Number of ECC bytes per page
 *
 * Build the page_layout array for NAND page handling for hardware ECC
 * handling basing on the nand_oobinfo structure supplied for the chip
 */
static int fill_autooob_layout(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;
	struct nand_oobinfo *oob = this->autooob;
	int oobfreesize = 0;
	int i, res = 0;
	int eccpos = 0, eccbytes = 0, cur = 0, oobcur = 0;

	this->layout = kmalloc(HW_AUTOOOB_LAYOUT_SIZE * sizeof (struct page_layout_item), GFP_KERNEL);

	if (this->layout == NULL) {
		printk(KERN_ERR "%s: kmalloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}
	else
		this->layout_allocated = 1;

	memset(this->layout, 0, HW_AUTOOOB_LAYOUT_SIZE * sizeof (struct page_layout_item));


	this->layout[0].type = ITEM_TYPE_DATA;
	this->layout[0].length = mtd->oobblock;
	DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: data type, length %d\n", this->layout[0].length);

	i = 1;
	// THT: Our layout is not uniform across eccsteps, so we must scan the entire layout,
	// and we cannot replicate it.
	while (i < HW_AUTOOOB_LAYOUT_SIZE && cur < mtd->oobsize) {
		if (oob->oobfree[oobcur][0] == cur) {
			int len = oob->oobfree[oobcur][1];
			oobfreesize += this->layout[i].length;
			oobcur++;
			if (i > 0 && this->layout[i-1].type == ITEM_TYPE_OOBFREE) {
				i--;
				cur -= this->layout[i].length;
				this->layout[i].length += len;
				DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oobfree concatenated, aggregate length %d\n", this->layout[i].length);
			} else {
				this->layout[i].type = ITEM_TYPE_OOBFREE;
				this->layout[i].length = len;
				DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oobfree type, length %d\n", this->layout[i].length);
			}
		} else if (oob->eccpos[eccpos] == cur) {
			int eccpos_cur = eccpos;
			do  {
				eccpos++;
				eccbytes++;
			} while (eccbytes < oob->eccbytes && oob->eccpos[eccpos] == oob->eccpos[eccpos+1] - 1);
			eccpos++;
			eccbytes++;
			this->layout[i].type = ITEM_TYPE_ECC;
			this->layout[i].length = eccpos - eccpos_cur;
			DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: ecc type, length %d\n", this->layout[i].length);
		} else {
			int len = min_t(int, oob->eccpos[eccpos], oob->oobfree[oobcur][0]);
			if (i > 0 && this->layout[i-1].type == ITEM_TYPE_OOB) {
				i--;
				cur -= this->layout[i].length;
				this->layout[i].length += len;
				DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oob concatenated, aggregate length %d\n", this->layout[i].length);
			} else {
				this->layout[i].type = ITEM_TYPE_OOB;
				this->layout[i].length = len;
				DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oob type, length %d\n", this->layout[i].length);
			}
		}
		cur += this->layout[i].length;
		i++;
	}

	return res;
}


static void fill_ecccmp_mask(struct mtd_info *mtd)
{
	struct brcmnand_chip *this = mtd->priv;
	int i, len;
	struct nand_oobinfo* oobsel = this->autooob;
	unsigned char* myEccMask = (unsigned char*) eccmask; // Defeat const

	/* 
	 * Should we rely on eccmask being zeroed out
	 */
	for (i=0; i < ARRAY_SIZE(eccmask); i++) {
		myEccMask[i] = 0;
	}
	/* Write 0xFF where there is a free byte */
	for (i = 0, len = 0; len < mtd->oobavail && len < mtd->oobsize && i < ARRAY_SIZE(oobsel->oobfree); i++) {
		int to = oobsel->oobfree[i][0];
		int num = oobsel->oobfree[i][1];

		if (num == 0) break; // End marker reached
		memcpy (&myEccMask[to], ffchars, num);
		len += num;
	}
}

/*
 * Sort Chip Select array into ascending sequence, and validate chip ID
 * We have to sort the CS in order not to use a wrong order when the user specify
 * a wrong order in the command line.
 */
static int
brcmnand_sort_chipSelects(struct mtd_info *mtd , int maxchips, int* argNandCS, int* chipSelect)
{
	int i;
	int cs[MAX_NAND_CS];
	struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
	

	this->numchips = 0;
	for (i=0; i<MAX_NAND_CS; i++) {
		cs[i] = -1;
	}
	for (i=0; i<maxchips; i++) {
		cs[argNandCS[i]] = argNandCS[i];
	}
	for (i=0; i<MAX_NAND_CS;i++) {
		if (cs[i] != -1) {
			this->CS[this->numchips++] = cs[i];
			printk("CS[%d] = %d\n", i, cs[i]);
		}
	}

	return 0;
}

/*
 * Make sure that all NAND chips have same ID
 */
static int
brcmnand_validate_cs(struct mtd_info *mtd )
{
	struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
	int i;
	unsigned long dev_id;
	
	// Now verify that a NAND chip is at the CS
	for (i=0; i<this->numchips; i++) {
		brcmnand_read_id(mtd, this->CS[i], &dev_id);

		if (dev_id != this->device_id) {
			printk(KERN_ERR "Device ID for CS[%1d] = %08lx, Device ID for CS[%1d] = %08lx\n",
				this->CS[0], this->device_id, this->CS[i], dev_id);
			return 1;
		}
	}
	return 0;
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
 * THT: For now, maxchips should always be 1.
 */
int brcmnand_scan(struct mtd_info *mtd , int maxchips )
{
	struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
	unsigned char brcmnand_maf_id;
	int err, i;
	volatile unsigned long nand_select, cs;
	unsigned int version_id;
	unsigned int version_major;
	unsigned int version_minor;

	if (!this->ctrl_read)
		this->ctrl_read = brcmnand_ctrl_read;
	if (!this->ctrl_write)
		this->ctrl_write = brcmnand_ctrl_write;
	if (!this->ctrl_writeAddr)
		this->ctrl_writeAddr = brcmnand_ctrl_writeAddr;

	if (!this->read_raw)
		this->read_raw = brcmnand_read_raw;
	if (!this->read_pageoob)
		this->read_pageoob = brcmnand_read_pageoob;
	if (!this->write_is_complete)
		this->write_is_complete = brcmnand_write_is_complete;
	
	if (!this->wait)
		this->wait = brcmnand_wait;

	if (!this->block_markbad)
		this->block_markbad = brcmnand_default_block_markbad;
	if (!this->scan_bbt)
		this->scan_bbt = brcmnand_default_bbt;
	if (!this->erase_bbt)
		this->erase_bbt = brcmnand_erase_bbt;

#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_0
	cs = 0;
	this->CS[0] = 0;
	this->numchips = 1;

#elif CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_1
	/*
	 * Read NAND strap option to see if this is on CS0 or CSn 
	 */
	{
		int i;
		
		nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
		cs = 0;
		for (i=0; i<6; i++) {
			if (nand_select & (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK<<i)) {
				this->CS[0] = cs = i;
				break;  // Only 1 chip select is allowed
			}
		}
	}
	this->numchips = 1;

#elif CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	/*
	  * For now, we can only support up to 1 chip using BCHP_NAND_CS_NAND_SELECT.  
	  * We have to use kernel command line parameter 
	  * to support more than one chip selects.  
	  * May be future HW will allow us to read BCHP_NAND_CS_NAND_SELECT again.
	  */
	/*
	 * Read NAND strap option to see if this is on CS0 or CSn 
	 */
	if (gNumNand == 0) {
		int i;
		
		nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
		cs = 0;
		// We start at 1 since 0 is used for Direct Addressing.
		// These bits are functional in v1.0 for backward compatibility, but we can only select 1 at a time.
		for (i=1; i<6; i++) {
			if (nand_select & (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK<<i)) {
				this->CS[0] = cs = i;
				break;  // Only 1 chip select is allowed
			}
		}
		this->numchips = 1;
	}
	else { // Chip Select via Kernel parameters, currently the only way to allow more than one CS to be set
		cs = this->numchips = gNumNand;
		if (brcmnand_sort_chipSelects(mtd, maxchips, gNandCS, this->CS))
			return (-EINVAL);
	}
	
#endif


PRINTK("brcmnand_scan: Calling brcmnand_probe\n");
	if (brcmnand_probe(mtd, cs))
		return -ENXIO;

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	if (this->numchips > 0) {
		if (brcmnand_validate_cs(mtd))
			return (-EINVAL);
	}
#endif

PRINTK("brcmnand_scan: Done brcmnand_probe\n");

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1	
	if (cs) {
		volatile unsigned long wr_protect;
		volatile unsigned long acc_control;

		
		if (this->chipSize >= (128 << 20)) {
			this->pbase = 0x11000000; /* Skip 16MB EBI Registers */
			mtd->size = this->chipSize - (16<<20); // Maximum size on a 128MB/256MB flash
			mtd->size_hi = 0;
			this->mtdSize = mtd->size;
		}
/*
		else if (this->chipSize == (256 << 20)) {
			this->pbase = 0x11000000; // Skip 16MB EBI Registers 
			mtd->size = 240<<20; // Maximum size on a 256MB flash, provided CS0/NOR is disabled
		}
 */
		else {
			this->pbase = 0x18000000 - this->chipSize;
			mtd->size = this->chipSize;
			mtd->size_hi = 0;
			this->mtdSize = mtd->size;
		}
		printk("Found NAND chip on Chip Select %d, chipSize=%dMB, usable size=%dMB, base=%08x\n", 
			cs, this->chipSize>>20, mtd->size>>20, this->pbase);

		/*
		 * When NAND is on CS0, it reads the strap values and set up accordingly.
		 * WHen on CS1, some configurations must be done by SW
		 */

		// Set Write-Unprotect.  This register is sticky, so if someone already set it, we are out of luck
		wr_protect = brcmnand_ctrl_read(BCHP_NAND_BLK_WR_PROTECT);
		if (wr_protect) {
			printk("Unprotect Register B4: %08x.  Please do a hard power recycle to reset\n", wr_protect);
			// THT: Actually we should punt here, as we cannot zero the register.
		} 
		brcmnand_ctrl_write(BCHP_NAND_BLK_WR_PROTECT, 0); // This will not work.
		if (wr_protect) {
			printk("Unprotect Register after: %08x\n", brcmnand_ctrl_read(BCHP_NAND_BLK_WR_PROTECT));
		}

		// Enable HW ECC.  This is another sticky register.
		acc_control = brcmnand_ctrl_read(BCHP_NAND_ACC_CONTROL);
		printk("ACC_CONTROL B4: %08x\n", acc_control);
		 
		brcmnand_ctrl_write(BCHP_NAND_ACC_CONTROL, acc_control | BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK);
		if (!(acc_control & BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK)) {
			printk("ACC_CONTROL after: %08x\n", brcmnand_ctrl_read(BCHP_NAND_ACC_CONTROL));
		}
	}
	else {
		/* NAND chip on Chip Select 0 */
		this->CS[0] = 0;
	
		/* Set up base, based on flash size */
		if (this->chipSize >= (256 << 20)) {
			this->pbase = 0x12000000;
			mtd->size = 0x20000000 - this->pbase; // THT: This is different than this->chipSize
		} else {
			/* We know that flash endAddr is 0x2000_0000 */
			this->pbase = 0x20000000 - this->chipSize;
			mtd->size = this->chipSize;
		}
		mtd->size_hi = 0;
		this->mtdSize = mtd->size;

		printk("Found NAND chip on Chip Select 0, size=%dMB, base=%08x\n", mtd->size>>20, this->pbase);
	}
	this->vbase = (void*) KSEG1ADDR(this->pbase);
	
#else
	/*
	 * v1.0 controller and after
	 */
	// This table is in the Architecture Doc
	// pbase is the physical address of the "logical" start of flash.  Logical means how Linux sees it,
	// and is given by the partition table defined in bcm7xxx-nand.c
	// The "physical" start of the flash is always at 1FC0_0000
	if (this->chipSize <= (256<<20)) 
		this->pbase = 0x20000000 - this->chipSize;
	else // 512MB and up
		this->pbase = 0; 
	// vbase is the address of the flash cache array
	this->vbase = (void*) (0xb0000000+BCHP_NAND_FLASH_CACHEi_ARRAY_BASE);  // Start of Buffer Cache
	// Already set in probe mtd->size = this->chipSize * this->numchips;
	// Make sure we use Buffer Array access, not direct access, Clear CS0
	nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
	printk("%s: B4 nand_select = %08x\n", __FUNCTION__, (uint32_t) nand_select);
	//this->directAccess = !(nand_select & BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK);
	// THT: Clear Direct Access bit 
	nand_select &= ~(BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK);
	brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, nand_select);

	nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
	printk("%s: After nand_select = %08x\n", __FUNCTION__, (uint32_t)  nand_select);
	this->directAccess = !(nand_select & BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK);
#endif

	this->bbt_erase_shift =  ffs(mtd->erasesize) - 1;

	/* Calculate the address shift from the page size */	
	this->page_shift = ffs(mtd->oobblock) - 1;
	this->bbt_erase_shift = this->phys_erase_shift = ffs(mtd->erasesize) - 1;
	this->chip_shift = ffs(this->chipSize) - 1;

	printk(KERN_INFO "page_shift=%d, bbt_erase_shift=%d, chip_shift=%d, phys_erase_shift=%d\n",
		this->page_shift, this->bbt_erase_shift , this->chip_shift, this->phys_erase_shift);

	/* Set the bad block position */
	this->badblockpos = mtd->oobblock > 512 ? 
		NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;


	/* Allocate buffers, if neccecary */
	if (!this->oob_buf) {
		size_t len;
		len = mtd->oobsize << (this->phys_erase_shift - this->page_shift);
		this->oob_buf = kmalloc (len, GFP_KERNEL);
		if (!this->oob_buf) {
			printk (KERN_ERR "brcmnand_scan(): Cannot allocate oob_buf %d bytes\n", len);
			return -ENOMEM;
		}
		this->options |= NAND_OOBBUF_ALLOC;
	}

	if (!this->data_buf) {
		size_t len;
		len = mtd->oobblock + mtd->oobsize;
		this->data_buf = kmalloc (len, GFP_KERNEL);
		if (!this->data_buf) {
			if (this->options & NAND_OOBBUF_ALLOC)
				kfree (this->oob_buf);
			printk (KERN_ERR "brcmnand_scan(): Cannot allocate data_buf %d bytes\n", len);
			return -ENOMEM;
		}
		this->options |= NAND_DATABUF_ALLOC;
	}
	
	this->state = FL_READY;
	init_waitqueue_head(&this->wq);
	spin_lock_init(&this->chip_lock);

	/* The number of bytes available for the filesystem to place fs dependend
	 * oob data */
PRINTK( "Deterimining mtd->oobavail, this->autooob=%p \n", this->autooob);

	switch (mtd->oobsize) {
	case 64:
		this->autooob = &brcmnand_oob_64;
		/*
		 * Adjust oobfree for ST chips, which also uses the 6th byte,
		 * in addition to the first byte to mark a bad block
		 */
		brcmnand_maf_id = (this->device_id >> 24) & 0xff;
		if (brcmnand_maf_id == FLASHTYPE_ST) {
			BUG_ON(this->autooob->oobfree[0][1] != 4);
			this->autooob->oobfree[0][1] = 3; // Exclude 6th byte from OOB free
		}
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
	
PRINTK("brcmnand_scan: this->autooob=%p, &brcmnand_oob_64=%p\n", this->autooob, &brcmnand_oob_64);

	mtd->oobavail = 0;
	for (i = 0; this->autooob->oobfree[i][1] && i < 8; i++) {
PRINTK("i=%d, oobfree.length=%d\n", i, this->autooob->oobfree[i][1]);
		mtd->oobavail += this->autooob->oobfree[i][1];
	}

PRINTK( "mtd->oobavail  %d\n", mtd->oobavail);

	switch (this->pageSize) {
	case 512:
		this->eccmode = NAND_ECC_HW3_512;
		this->eccbytes = 3;
		this->eccsize = 512;	
		break;
		
	case 2048:
		/* Brcm NAND controller uses NAND_ECC_HW3_512 regardless of page size
		 * may be until the next revision which uses HW6_512
		 */
		this->eccmode = NAND_ECC_HW3_512;
		this->eccbytes = 3;
		this->eccsize = 512;	
		break;
	default:
		printk(KERN_ERR "Page size %d not supported\n", this->pageSize);
	}
	mtd->eccsize = this->eccsize;
	this->eccOobSize = 16;

PRINTK( "mtd->eccsize=%d, eccbytes=%d\n", mtd->eccsize, this->eccbytes);

	/* For 2K page, eccsteps is 4 for the 4 slides that make up a page */
	this->eccsteps = mtd->oobblock / this->eccsize;
	/* We consider only layout allocation performed in nand_base */
	this->layout_allocated = 0;
	if (!this->layout && this->autooob)
		if (fill_autooob_layout(mtd) < 0)
			BUG();

	/* Version ID */
	version_id = this->ctrl_read(BCHP_NAND_REVISION);
	version_major = (version_id & 0xff00) >> 8;
	version_minor = (version_id & 0xff);
	printk(KERN_INFO "Brcm NAND controller version = %x.%x NAND flash size %dMB @%08x\n", 
		version_major, version_minor, this->chipSize>>20, (uint32_t) this->pbase);


	/*
	 * Initialize the eccmask array for ease of verifying OOB area.
	 */
	fill_ecccmp_mask(mtd);
	

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
	mtd->read_oobfree = brcmnand_read_oobfree;
	mtd->write_oobfree = brcmnand_write_oobfree;
	
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
PRINTK("Calling mtd->unlock(ofs=0, MTD Size=%08x\n", mtd->size);
	mtd->unlock(mtd, 0x0, mtd->size);

	err =  this->scan_bbt(mtd);

	return err;

}

void brcmnand_prepare_reboot(void)
{
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
	/* 
	 * Must set NAND back to Direct Access mode for reboot, but only if NAND is on CS0
	 */


	struct brcmnand_chip* this;
	
	/*
	 * Prevent further access to the NAND flash, we are rebooting 
	 */
	this = brcmnand_get_device_exclusive();

	if (this->CS[0] == 0) { // Only if on CS0
		volatile unsigned long nand_select;

		

		nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
		//printk("%s: B4 nand_select = %08x\n", __FUNCTION__, (uint32_t) nand_select);
		
		// THT: Set Direct Access bit 
		nand_select |= BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK;
		brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, nand_select);

		//nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
		//printk("%s: After nand_select = %08x\n", __FUNCTION__, (uint32_t)  nand_select);
	}
	
#endif

	return;
}
EXPORT_SYMBOL(brcmnand_prepare_reboot);


#if defined( CONFIG_MIPS_BCM7401C0 ) || defined( CONFIG_MIPS_BCM7118A0 )  || defined( CONFIG_MIPS_BCM7403A0 ) 
extern int bcm7401Cx_rev;
extern int bcm7118Ax_rev;
extern int bcm7403Ax_rev; 
static DEFINE_SPINLOCK(bcm9XXXX_lock);
static unsigned long misb_war_flags;

static inline void
HANDLE_MISB_WAR_BEGIN(void)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 
                                 || bcm7403Ax_rev == 0x20)
	{	
		spin_lock_irqsave(&bcm9XXXX_lock, misb_war_flags);
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
	}
}

static inline void
HANDLE_MISB_WAR_END(void)
{
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 
                                 || bcm7403Ax_rev == 0x20)
	{	
		spin_unlock_irqrestore(&bcm9XXXX_lock, misb_war_flags);
	}
}

#else
#define HANDLE_MISB_WAR_BEGIN()
#define HANDLE_MISB_WAR_END()
#endif

/*
 * CS0 reset values are gone by now, since the bootloader disabled CS0 before booting Linux
 * in order to give the EBI address space to NAND.
 * We will need to read strap_ebi_rom_size in order to reconstruct the CS0 values
 * This will not be a problem, since in order to boot with NAND on CSn (n != 0), the board
 * must be strapped for NOR.
 */
static unsigned int 
get_rom_size(unsigned long* outp_cs0Base)
{
	volatile unsigned long strap_ebi_rom_size, sun_top_ctrl_strap_value;
	uint32_t romSize = 0;

	sun_top_ctrl_strap_value = *(volatile unsigned long*) (0xb0000000|BCHP_SUN_TOP_CTRL_STRAP_VALUE);
	strap_ebi_rom_size = sun_top_ctrl_strap_value & BCHP_SUN_TOP_CTRL_STRAP_VALUE_strap_ebi_rom_size_MASK;
	strap_ebi_rom_size >>= BCHP_SUN_TOP_CTRL_STRAP_VALUE_strap_ebi_rom_size_SHIFT;

	// Here we expect these values to remain the same across platforms.
	// Some customers want to have a 2MB NOR flash, but I don't see how that is possible.
	switch(strap_ebi_rom_size) {
	case 0:
		romSize = 64<<20;
		*outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_64MB;
		break;
	case 1:
		romSize = 16<<20;
		*outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_16MB;
		break;
	case 2:
		romSize = 8<<20;
		*outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_8MB;
		break;
	case 3:
		romSize = 4<<20;
		*outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_4MB;
		break;
	default:
		printk("%s: Impossible Strap Value %08x for BCHP_SUN_TOP_CTRL_STRAP_VALUE\n", 
			__FUNCTION__, sun_top_ctrl_strap_value);
		BUG();
	}
	return romSize;
}

/*
 * @ buff		Buffer to hold the data read from the NOR flash, must be able to hold len bytes, and aligned on
 *			word boundary.
 * @ offset	Offset of the data from CS0 (on NOR flash), must be on word boundary.
 * @ len		Number of bytes to be read, must be even number.
 *
 * returns 0 on success, negative error codes on failure.
 *
 * The caller thread may block until access to the NOR flash can be granted.
 * Further accesses to the NAND flash (from other threads) will be blocked until this routine returns.
 * The routine performs the required swapping of CS0/CS1 under the hood.
 */
int brcmnand_readNorFlash(struct mtd_info *mtd, void* buff, unsigned int offset, int len)
{
	struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
	int ret = -EFAULT;
	int i; 
	int csNand; // Which CS is NAND
	volatile unsigned long cs0Base, cs0Cnfg, cs0BaseAddr, csNandSelect;
	volatile unsigned long csNandBase[MAX_NAND_CS], csNandCnfg[MAX_NAND_CS];
	unsigned int romSize;
	volatile uint16_t* pui16 = (volatile uint16_t*) buff;
	volatile uint16_t* fp;

	if (!this) { // When booting from CRAMFS/SQUASHFS using romblock dev
		this = brcmnand_get_device_exclusive();
		mtd = (struct mtd_info*) this->priv;
	}
	else if (brcmnand_get_device(mtd, BRCMNAND_FL_EXCLUSIVE))
		return ret;

	romSize = get_rom_size(&cs0Base);
	
	cs0BaseAddr = cs0Base & BCHP_EBI_CS_BASE_0_base_addr_MASK;

	if ((len + offset) > romSize) {
		printk("%s; Attempt to read past end of CS0, (len+offset)=%08x, romSize=%dMB\n",
			__FUNCTION__, len + offset, romSize>>20);
		ret = (-EINVAL);
		goto release_device_and_out;
	}

	cs0Cnfg = *(volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_CONFIG_0);

	// Turn off NAND CS
	for (i=0; i < this->numchips; i++) {
		csNand = this->CS[i];

		if (csNand == 0) {
			printk("%s: Call this routine only if NAND is not on CS0\n", __FUNCTION__);
			ret = (-EINVAL);
			goto release_device_and_out;
		}

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
		BUG_ON(csNand > 5);
#else
		BUG_ON(csNand > 8);
#endif
		csNandBase[i] = *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_BASE_0 + 8*csNand);
		csNandCnfg[i] = *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_CONFIG_0 + 8*csNand);

		// Turn off NAND, must turn off both NAND_CS_NAND_SELECT and CONFIG.
		// We turn off the CS_CONFIG here, and will turn off NAND_CS_NAND_SELECT for all CS at once,
		// outside the loop.
		*(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_CONFIG_0 + 8*csNand) = 
			csNandCnfg[i] & (~BCHP_EBI_CS_CONFIG_0_enable_MASK);

	}
	
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_0_1
	csNandSelect = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);

	brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, csNandSelect & 
		~(
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
			BCHP_NAND_CS_NAND_SELECT_EBI_CS_5_SEL_MASK
			| BCHP_NAND_CS_NAND_SELECT_EBI_CS_4_SEL_MASK
			| BCHP_NAND_CS_NAND_SELECT_EBI_CS_3_SEL_MASK
			| BCHP_NAND_CS_NAND_SELECT_EBI_CS_2_SEL_MASK
			| BCHP_NAND_CS_NAND_SELECT_EBI_CS_1_SEL_MASK
			| BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK
#else
			0x0000003E	/* Not documented on V1.0+ */
#endif
		));
#endif

	// Turn on NOR on CS0
	*(volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_CONFIG_0) = 
		cs0Cnfg | BCHP_EBI_CS_CONFIG_0_enable_MASK;

	// Take care of MISB Bridge bug on 7401c0/7403a0/7118a0
	HANDLE_MISB_WAR_BEGIN();

	// Read NOR, 16 bits at a time
	fp = (volatile uint16_t*) (KSEG1ADDR(cs0BaseAddr + offset));
	for (i=0; i < (len>>1); i++) {
		pui16[i] = fp[i];
	}

	HANDLE_MISB_WAR_END();

	// Turn Off NOR
	*(volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_CONFIG_0) = 
		cs0Cnfg & (~BCHP_EBI_CS_CONFIG_0_enable_MASK);

	// Turn NAND back on
	for (i=0; i < this->numchips; i++) {
		csNand = this->CS[i];
		if (csNand == 0) {
			printk("%s: Call this routine only if NAND is not on CS0\n", __FUNCTION__);
			ret = (-EINVAL);
			goto release_device_and_out;
		}
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
		BUG_ON(csNand > 5);
#else
		BUG_ON(csNand > 8);
#endif
		*(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_BASE_0 + 8*csNand) = csNandBase[i] ;
		*(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_CONFIG_0 + 8*csNand) = csNandCnfg[i];
	}

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_0_1
	// Restore NAND_CS_SELECT
	brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, csNandSelect);
#endif
	udelay(10000); // Wait for ID Configuration to stabilize
	
release_device_and_out:
	brcmnand_release_device(mtd);
//printk("<-- %s\n", __FUNCTION__);

	return ret;
}
EXPORT_SYMBOL(brcmnand_readNorFlash);

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

	/* Buffer allocated by brcmnand_scan */
	if (this->options & NAND_DATABUF_ALLOC)
		kfree(this->data_buf);

	/* Buffer allocated by brcmnand_scan */
	if (this->options & NAND_OOBBUF_ALLOC)
		kfree(this->oob_buf);

}



