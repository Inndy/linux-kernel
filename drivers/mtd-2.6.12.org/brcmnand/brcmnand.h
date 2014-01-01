/*
 * drivers/mtd/brcmnand/brcmnand.h
 *
 *  Copyright (c) 2005,2006 Broadcom Corp.
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
 * Data structures for Broadcom NAND controller
 * 
 * when		who		what
 * 20060729	tht		Original coding
 */


#ifndef _BRCM_NAND_H_
#define _BRCM_NAND_H_

#include <linux/mtd/nand.h>
#include <asm/brcmstb/common/bcmtypes.h>
#include <asm/brcmstb/common/brcmstb.h>

//ST NAND flashes
#ifndef FLASHTYPE_ST
    #define FLASHTYPE_ST            0x20
#endif
#define ST_NAND128W3A           0x73
#define ST_NAND256R3A           0x35
#define ST_NAND256W3A           0x75
#define ST_NAND256R4A           0x45
#define ST_NAND256W4A           0x55
#define ST_NAND512R3A           0x36    //Used on Bcm97400
#define ST_NAND512W3A           0x76
#define ST_NAND512R4A           0x46
#define ST_NAND512W4A           0x56
#define ST_NAND01GR3A           0x39
#define ST_NAND01GW3A           0x79
#define ST_NAND01GR4A           0x49
#define ST_NAND01GW4A           0x59
#define ST_NAND01GR3B           0xA1
#define ST_NAND01GW3B           0xF1
#define ST_NAND01GR4B           0xB1
#define ST_NAND01GW4B           0xC1
#define ST_NAND02GR3B           0xAA
#define ST_NAND02GW3B           0xDA
#define ST_NAND02GR4B           0xBA
#define ST_NAND02GW4B           0xCA
#define ST_NAND04GR3B           0xAC
#define ST_NAND04GW3B           0xDC
#define ST_NAND04GR4B           0xBC
#define ST_NAND04GW4B           0xCC
#define ST_NAND08GR3B           0xA3
#define ST_NAND08GW3B           0xD3
#define ST_NAND08GR4B           0xB3
#define ST_NAND08GW4B           0xC3

//Samsung NAND flash
#define FLASHTYPE_SAMSUNG       0xEC
#define SAMSUNG_K9F1G08R0A      0xA1
#define SAMSUNG_K9F1G08U0A      0xF1
#define SAMSUNG_K9F2G08U1A      0xF1

//Command Opcode
#define OP_PAGE_READ                0x01000000
#define OP_SPARE_AREA_READ          0x02000000
#define OP_STATUS_READ              0x03000000
#define OP_PROGRAM_PAGE             0x04000000
#define OP_PROGRAM_SPARE_AREA       0x05000000
#define OP_COPY_BACK                0x06000000
#define OP_DEVICE_ID_READ           0x07000000
#define OP_BLOCK_ERASE              0x08000000
#define OP_FLASH_RESET              0x09000000
#define OP_BLOCKS_LOCK              0x0A000000
#define OP_BLOCKS_LOCK_DOWN         0x0B000000
#define OP_BLOCKS_UNLOCK            0x0C000000
#define OP_READ_BLOCKS_LOCK_STATUS  0x0D000000

//NAND flash controller 
#define NFC_FLASHCACHE_SIZE     512

#define BRCMNAND_CTRL_REGS		(KSEG1ADDR(0x10000000 + BCHP_NAND_REVISION))
#define BRCMNAND_CTRL_REGS_END	(KSEG1ADDR(0x10000000 + BCHP_NAND_BLK_WR_PROTECT))


/**
 * brcmnand_state_t - chip states
 * Enumeration for BrcmNAND flash chip state
 */
typedef enum {
	BRCMNAND_FL_READY = FL_READY,
	BRCMNAND_FL_READING = FL_READING,
	BRCMNAND_FL_WRITING = FL_WRITING,
	BRCMNAND_FL_ERASING = FL_ERASING,
	BRCMNAND_FL_SYNCING = FL_SYNCING,
	BRCMNAND_FL_CACHEDPRG = FL_CACHEDPRG,
	FL_UNLOCKING,
	FL_LOCKING,
	FL_RESETING,
	FL_OTPING,
	FL_PM_SUSPENDED,
} brcmnand_state_t;

#if 0
/**
 * struct brcmnand_cache - Brcm NAND BufferRAM Data
 * @param block		block # in BufferRAM
 * @param page		page # in BufferRAM
 * @param slide		slide # within page, if page size != NFC_FLASH_CACHE_SIZE
 * @param valid		valid flag
 */
struct brcmnand_bufferram {
	int block;	// Block #
	int page;	// Page # in block
	int slide;	// Slide # (of 512B cache) in page
	int valid;
};
#endif

/**
 * struct brcmnand_chip - BrcmNAND Private Flash Chip Data
 * @param base		[BOARDSPECIFIC] address to access Broadcom NAND controller
 * @param chipsize	[INTERN] the size of one chip for multichip arrays
 * @param device_id	[INTERN] device ID
 * @param verstion_id	[INTERN] version ID
 * @param options	[BOARDSPECIFIC] various chip options. They can partly be set to inform brcmnand_scan about
 * @param erase_shift	[INTERN] number of address bits in a block
 * @param page_shift	[INTERN] number of address bits in a page
 * @param ppb_shift	[INTERN] number of address bits in a pages per block
 * @param page_mask	[INTERN] a page per block mask
 * @param bufferam_index	[INTERN] BufferRAM index
 * @param bufferam	[INTERN] BufferRAM info
 * @param readw		[REPLACEABLE] hardware specific function for read short
 * @param writew	[REPLACEABLE] hardware specific function for write short
 * @param command	[REPLACEABLE] hardware specific function for writing commands to the chip
 * @param wait		[REPLACEABLE] hardware specific function for wait on ready
 * @param read_bufferram	[REPLACEABLE] hardware specific function for BufferRAM Area
 * @param write_bufferram	[REPLACEABLE] hardware specific function for BufferRAM Area
 * @param read_word	[REPLACEABLE] hardware specific function for read register of BrcmNAND
 * @param write_word	[REPLACEABLE] hardware specific function for write register of BrcmNAND
 * @param scan_bbt	[REPLACEALBE] hardware specific function for scaning Bad block Table
 * @param chip_lock	[INTERN] spinlock used to protect access to this structure and the chip
 * @param wq		[INTERN] wait queue to sleep on if a BrcmNAND operation is in progress
 * @param state		[INTERN] the current state of the BrcmNAND device
 * @param autooob	[REPLACEABLE] the default (auto)placement scheme
 * @param bbm		[REPLACEABLE] pointer to Bad Block Management
 * @param priv		[OPTIONAL] pointer to private chip date
 */
struct brcmnand_chip {
	unsigned long		regs;	/* Register page */
	unsigned char __iomem		*vbase; /* Virtual address of start of flash */
	unsigned int		chipSize;
	unsigned int		numchips; // Always 1
	unsigned int		chip_shift; // Always 0

	/* THT Added */
	unsigned int 		busWidth, pageSize, blockSize;
	
	unsigned int		device_id;
	//unsigned int		density_mask;
	unsigned int		options;

	unsigned int		erase_shift;
	unsigned int		page_shift;
	//unsigned int		ppb_shift;	/* Pages per block shift */
	unsigned int		page_mask;

	//THT: In BrcmNAND, the NAND controller  keeps track of the 512B Cache
	// so there is no need to manage the buffer ram.
	//unsigned int		bufferram_index;
	//struct brcmnand_bufferram	bufferram;

	int (*command)(struct mtd_info *mtd, int cmd, loff_t address, size_t len);
	int (*wait)(struct mtd_info *mtd, int state, uint32_t* pStatus);
	/*
	int (*read_bufferram)(struct mtd_info *mtd,
			unsigned char *buffer, int offset, size_t count);
	int (*write_bufferram)(struct mtd_info *mtd,
			const unsigned char *buffer, int offset, size_t count);
	*/
	
	unsigned short (*read_word)(void __iomem *addr);
	void (*write_word)(unsigned short value, void __iomem *addr);

	// THT: Sync Burst Read, not supported.
	//void (*mmcontrol)(struct mtd_info *mtd, int sync_read);
	
	int (*block_markbad)(struct mtd_info *mtd, loff_t ofs);
	int (*scan_bbt)(struct mtd_info *mtd);
	int (*erase_bbt)(struct mtd_info *mtd, struct erase_info *instr, int allowbbt);

	uint32_t (*ctrl_read) (uint32_t command);
	void (*ctrl_write) (uint32_t command, uint32_t val);

	spinlock_t			chip_lock;
	//atomic_t			semCount;
	wait_queue_head_t	wq;
	brcmnand_state_t	state;
	unsigned char		*page_buf;

	struct nand_oobinfo	*autooob;

	int bbt_erase_shift;
	int			badblockpos;
	uint8_t		*bbt;
	int (*isbad_bbt)(struct mtd_info *mtd, loff_t ofs, int allowbbt);
	struct nand_bbt_descr	*bbt_td;
	struct nand_bbt_descr	*bbt_md;
	struct nand_bbt_descr	*badblock_pattern;

	void				*priv;
};


/*
 * Options bits
 */
#define BRCMNAND_CONT_LOCK		(0x0001)
#define BRCMNAND_PAGEBUF_ALLOC	(0x1000)



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
extern int brcmnand_scan(struct mtd_info *mtd , int maxchips );

/**
 * brcmnand_release - [BrcmNAND Interface] Free resources held by the BrcmNAND device
 * @param mtd		MTD device structure
 */
extern void brcmnand_release(struct mtd_info *mtd);

/* BrcmNAND BBT interface */
extern int brcmnand_scan_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd);
extern int brcmnand_default_bbt(struct mtd_info *mtd);

extern int brcmnand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen);


#endif
