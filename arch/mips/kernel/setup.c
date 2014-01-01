/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 Linus Torvalds
 * Copyright (C) 1995 Waldorf Electronics
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2000, 01, 02, 03  Ralf Baechle
 * Copyright (C) 1996 Stoned Elipot
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2000 2001, 2002  Maciej W. Rozycki
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/utsname.h>
#include <linux/a.out.h>
#include <linux/tty.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <linux/root_dev.h>
#include <linux/highmem.h>
#include <linux/console.h>
#include <linux/cpu.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/system.h>

#ifdef CONFIG_MIPS_BRCM97XXX
#include <asm/brcmstb/common/brcmstb.h>
#endif
#if defined (CONFIG_MIPS_BCM7440)
#include <asm/page.h>
#endif

struct cpuinfo_mips cpu_data[NR_CPUS];

EXPORT_SYMBOL(cpu_data);

#ifdef CONFIG_VT
struct screen_info screen_info;
#endif

/*
 * Despite it's name this variable is even if we don't have PCI
 */
unsigned int PCI_DMA_BUS_IS_PHYS;

EXPORT_SYMBOL(PCI_DMA_BUS_IS_PHYS);

/*
 * Setup information
 *
 * These are initialized so they are in the .data section
 */
unsigned long mips_machtype = MACH_UNKNOWN;
unsigned long mips_machgroup = MACH_GROUP_UNKNOWN;

EXPORT_SYMBOL(mips_machtype);
EXPORT_SYMBOL(mips_machgroup);

struct boot_mem_map boot_mem_map;
phys_t upper_memory;
EXPORT_SYMBOL(upper_memory);

static char command_line[CL_SIZE];
       char arcs_cmdline[CL_SIZE]=CONFIG_CMDLINE;

/*
 * mips_io_port_base is the begin of the address space to which x86 style
 * I/O ports are mapped.
 */
const unsigned long mips_io_port_base = -1;
EXPORT_SYMBOL(mips_io_port_base);

/*
 * isa_slot_offset is the address where E(ISA) busaddress 0 is mapped
 * for the processor.
 */
unsigned long isa_slot_offset;
EXPORT_SYMBOL(isa_slot_offset);

static struct resource code_resource = { .name = "Kernel code", };
static struct resource data_resource = { .name = "Kernel data", };

extern unsigned long g_board_RAM_size;

#define PFN_UP(x)       (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)     ((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)     ((x) << PAGE_SHIFT)

#define MAXMEM          HIGHMEM_START
#define MAXMEM_PFN      PFN_DOWN(MAXMEM)


void __init add_memory_region(phys_t start, phys_t size, long type)
{
	int x = boot_mem_map.nr_map;

/* Disable merging for BRCM boards */
#ifndef CONFIG_MIPS_BRCM

	struct boot_mem_map_entry *prev = boot_mem_map.map + x - 1;
	/*
	 * Try to merge with previous entry if any.  This is far less than
	 * perfect but is sufficient for most real world cases.
	 */
	if (x && prev->addr + prev->size == start && prev->type == type) {
		prev->size += size;
		return;
	}
#endif

	if (x == BOOT_MEM_MAP_MAX) {
                printk("%s: TOO MANY ENTRIES IN THE MEMORY MAP! ABORT!!\n",
		           __FUNCTION__);
		return;
	}

	boot_mem_map.map[x].addr = start;
	boot_mem_map.map[x].size = size;
	boot_mem_map.map[x].type = type;
	boot_mem_map.nr_map++;
}

static void __init print_memory_map(void)
{
	int i;
	const int field = 2 * sizeof(unsigned long);

	for (i = 0; i < boot_mem_map.nr_map; i++) {
		printk(" memory: %0*Lx @ %0*Lx ",
		       field, (unsigned long long) boot_mem_map.map[i].size,
		       field, (unsigned long long) boot_mem_map.map[i].addr);

		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
			printk("(usable)\n");
			break;
		case BOOT_MEM_ROM_DATA:
			printk("(ROM data)\n");
			break;
		case BOOT_MEM_RESERVED:
			printk("(reserved)\n");
			break;
		default:
			printk("type %lu\n", boot_mem_map.map[i].type);
			break;
		}
	}
}

#ifdef CONFIG_MIPS_BRCM

/*
 * THT: Unlike the kernel boot_mem_map which is an array, this is a essentially 
 * a linked list for ease of inserting/deleting nodes. (Double linked list may be an overkill)
 * Since we cannot do kmalloc at this time, we just have to use array to handle it.
 * We don't currently need deletion, so this is just a simple sorted linked list implementation 
 * with support for insertion.
 *
 * Note: We try to put all Brcm extensions under #ifdef CONFIG_MIPS_BRCM but never tested the
 * else case. God forbid, one day we may be required to merge these codes into the MIPS tree.
 */

struct brcm_boot_mem_map {
	int nr_map;
	int head, tail;
	int avail;			// Currently we don't support deletion, so this is just the next free node
	struct {
		phys_t addr;	/* start of memory segment */
		phys_t size;	/* size of memory segment */
		long type;		/* type of memory segment */
		int next, prev;
	} map[BOOT_MEM_MAP_MAX];
};
struct brcm_boot_mem_map brcm_mmap;
struct si_bm_rsvd brcm_bm;

/*
 * Standard Linked List Insert After op
 */
static int brcm_insert_after(int after, phys_t addr, phys_t size, long type)
{
	int newNode = brcm_mmap.avail;

	if (++brcm_mmap.avail >= BOOT_MEM_MAP_MAX) {
		printk("Max Nodes exceeded: Node at [%08lx:%08lx] ignored\n", 
		            addr, size);
		return newNode;
	}
	brcm_mmap.nr_map++;
	brcm_mmap.map[newNode].addr = addr;
	brcm_mmap.map[newNode].size = size;
	brcm_mmap.map[newNode].type = type;
	brcm_mmap.map[newNode].next = brcm_mmap.map[newNode].prev = NILL;
	if (after == NILL) {
		brcm_mmap.map[newNode].next = brcm_mmap.head;
		brcm_mmap.head = newNode;
	}
	else {
		brcm_mmap.map[newNode].next = brcm_mmap.map[after].next;
		brcm_mmap.map[newNode].prev = after;
		brcm_mmap.map[after].next = newNode;
	}
	if (brcm_mmap.tail == after ) { /* insert after last node, or to an empty list */
		brcm_mmap.tail = newNode;
	}
	
	return newNode;
}

/*
 * Load the bootmem_map into our brcm_mmap, for ease of sorting.
 */
static void brcm_init_memory_map(void)
{
	int i;
	
	brcm_mmap.nr_map = 0;
	brcm_mmap.head=brcm_mmap.tail = NILL;
	brcm_mmap.avail = 0;

	for (i=0; i<BOOT_MEM_MAP_MAX; i++) {
		brcm_mmap.map[i].next = brcm_mmap.map[i].prev = NILL;
		brcm_mmap.map[i].addr = brcm_mmap.map[i].size = brcm_mmap.map[i].type = 0;
	}
	
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		brcm_insert_after(brcm_mmap.tail, 
			          boot_mem_map.map[i].addr, 
		                  boot_mem_map.map[i].size, 
		                  boot_mem_map.map[i].type);
	}

        brcm_bm.addr = 0;
        brcm_bm.size = 0;
        brcm_bm.type = 0;

}

static int brcm_insert_ram_node(phys_t start, 
                                phys_t size, 
                                long type, 
                                struct si_bm_rsvd* bm)

{
	int n;
	int found = 0;
	phys_t oldstart, oldsize;
        long oldtype, fliptype;
        unsigned long node_end;

        /*
        ** Initialize return results from this work to caller via bm structure.
        */
        bm->addr = 0;
        bm->size = 0;
        bm->type = 0;



	for (n = brcm_mmap.head; n != NILL && !found; n = brcm_mmap.map[n].next)
	{
		unsigned long end_node = brcm_mmap.map[n].addr + brcm_mmap.map[n].size;

                node_end = brcm_mmap.map[n].addr + brcm_mmap.map[n].size;

                if (start >= brcm_mmap.map[n].addr && start < node_end ) {
                        if ((start+size) <= node_end) {

				found = 1;
				break;
			}
			/* Some validation */
                        else {
                                printk("%s: Start 0x%08lx  Size 0x%08lx overlaps existing node(0x%08lx:0x%08lx) - IGNORE!\n",
                                    __FUNCTION__, start, size, brcm_mmap.map[n].addr, brcm_mmap.map[n].size);
                            return -1;
			}

		}
	}
	if (!found) {
                printk("%s: Start 0x%08lx Size 0x%08lx CANNOT BE INSERTED!!\n",
                        __FUNCTION__, start, size);
		return -2;
	}
	
	
	/* Save the old info */
	oldstart = brcm_mmap.map[n].addr;
	oldtype  = brcm_mmap.map[n].type;
	oldsize  = brcm_mmap.map[n].size;

        switch (type) 
	{
                case BOOT_MEM_RAM:
                        fliptype = BOOT_MEM_RESERVED;
                        break;
                case BOOT_MEM_RESERVED:
                        fliptype = BOOT_MEM_RAM;
                        break;
                default:
                        fliptype = BOOT_MEM_ROM_DATA;
        }

	
	if (start >= brcm_mmap.map[n].addr) {
                /*
                 * We know that start+size <= (brcm_mmap.map[n].addr + 
                 * brcm_mmap.map[n].size) from above
                 * This splits the old node into 2 or 3 nodes. The middle node
                 * will bear the type of the new node (to be inserted)
                 * The other 2 nodes will be "flipped".
                 * This is to cover cases such as:
                 *
                 *  'mem=80M' : Then the remainder of the node is 
                 *              reserved for BMEM.
                 *  'mem=80M@80M" : Then the first 80M and remainder of 
                 *                  the node are reserved for BMEM.
                 */
 
		if (start > brcm_mmap.map[n].addr) {
                        /* Shrink original node accordingly */
			brcm_mmap.map[n].size = start - brcm_mmap.map[n].addr;
			brcm_mmap.map[n].type = fliptype;
		
                        /* Insert new node with specified type. */
			n = brcm_insert_after(n, start, size, type);
		}
                else { // current_start == old_start, just set the type and size
			brcm_mmap.map[n].size = size;
			brcm_mmap.map[n].type = type;			
		}

                /* Insert the last node with the flip type if required */
		if ((start+size) < (oldstart+oldsize)) {
			n = brcm_insert_after(n, 
			                      start+size, 
					      oldstart+oldsize-start-size, 
			                      fliptype);
		}
	}

        /*
        ** Return results from this work to caller via bm structure.
        */
        if ((brcm_mmap.map[n].next != NILL) && 
	    (brcm_mmap.map[n].next != brcm_mmap.head)) 
	{
                /* Pass back values from next node */
                bm->addr = brcm_mmap.map[brcm_mmap.map[n].next].addr;
                bm->size = brcm_mmap.map[brcm_mmap.map[n].next].size;
                bm->type = (int)brcm_mmap.map[brcm_mmap.map[n].next].type;
        }


	return n;
}

/*
** Check to see if the node has already been provided.
*/
static int brcm_test_ram_node(phys_t start, phys_t size, long type)
{
        int n;
        int found = 0;
        unsigned long node_end;
        unsigned long request_end;

        for (n = brcm_mmap.head; n != NILL && !found; n = brcm_mmap.map[n].next)
	{

                node_end    = brcm_mmap.map[n].addr + brcm_mmap.map[n].size;
                request_end = start + size;

		if (((start >= brcm_mmap.map[n].addr && start < node_end) ||
		     (request_end >  brcm_mmap.map[n].addr && 
		      request_end < node_end)) && brcm_mmap.map[n].type == type)
		{
                        printk("Physical Address 0x%08x Size 0x%08x Type %s: OVERLAPS EXISTING NODE - IGNORE!!\n",
                                        start, size,
                                        (type == BOOT_MEM_RESERVED ? "RESERVED" : "RAM"));
                        found = 1;
                        break;
                }
        }
        return(found);
}



#if 0
/*
 * Copy the our private bootmem map back to the kernel bootmem, marking all reserved type as RAM.
 */
static void brcm_copy_bootmem_map(void)
{
	int node;
	
	boot_mem_map.nr_map = 0;
	for (node = brcm_mmap.head; node != NILL; node = brcm_mmap.map[node].next) {
		add_memory_region(brcm_mmap.map[node].addr, brcm_mmap.map[node].size, BOOT_MEM_RAM);
	}
}
#endif

int brcm_walk_boot_mem_map(int in_idx, const int type, struct si_bm_rsvd *si_data)
{
        int n;
        int out_idx = NILL;

        si_data->addr = 0;
        si_data->size = 0;
        si_data->type = 0;

        if (in_idx != NILL) {
                for (n=((in_idx == 0) ? brcm_mmap.head : in_idx); 
                        n != NILL; n = brcm_mmap.map[n].next) 
		{
                        if ((type == -1) || (brcm_mmap.map[n].type == type)) 
			{
                                si_data->addr = brcm_mmap.map[n].addr;
                                si_data->size = brcm_mmap.map[n].size;
                                si_data->type = brcm_mmap.map[n].type;
                                out_idx       = brcm_mmap.map[n].next;
                                break;
                        }
                }
        }
        return (out_idx);
}

EXPORT_SYMBOL(brcm_walk_boot_mem_map);

/*
 * We want to print the memory map NOW so we use uart_puts.  
 * printk's won't be shown  until the kernel has started
 */

static void brcm_print_memory_map(void)
{
	char msg[80];
	int idx = 0;
	struct si_bm_rsvd data;

	while (idx != -1) 
	{
		idx = brcm_walk_boot_mem_map(idx, -1, &data);
		sprintf(msg, "node [%08lx, %08lx: %s]\n", 
		          data.addr, data.size, 
                          ((data.type == BOOT_MEM_RAM) ? "RAM" : "RSVD"));
		uart_puts(msg);
	}

}

/*
 * By default, set the kernel to use 32MB if the user does not specify mem=nnM on the command line
 */
unsigned long get_RAM_size(void);

static inline void brcm_default_boot_mem(void)
{
	int ramSizeMB = get_RAM_size() >> 20;
	int size;
	char msg[40];
	
	if (ramSizeMB <= 32) 
	{
#ifdef CONFIG_BLK_DEV_INITRD
		uart_puts("Using all RAM for STBs\n");
		size = ramSizeMB;

#else
		size = 20;

#endif
	}
	else 
	{
		/* 
 		 * Kernels on STBs with larger than 32MB, we only use 
		 * 32MB RAM for the kernel
		 */

#ifdef CONFIG_MTD_BRCMNAND
		size = 48; // FOr BBT tables
#else
		size = 32;
#endif
	}
	sprintf(msg, "Using %dMB for memory, overwrite by passing mem=xx\n", 
	                                size);
	uart_puts(msg);
        brcm_insert_ram_node(0, size<<20, BOOT_MEM_RAM, &brcm_bm);

}


/*
 * Here we only check for the kernel starting memory.
 */
static void inline brcm_reserve_bootmem_node(unsigned long firstUsableAddr)
{
	int i;
	
	/*
	 * The kernel boot_mem_map does not contain the reserved info, but ours does
	 */
	for (i = brcm_mmap.head; i != NILL; i = brcm_mmap.map[i].next) {
		if (brcm_mmap.map[i].type != BOOT_MEM_RESERVED)
			continue;

		/* Kernel must use the first 1MB for exception handlers, so request to reserve
		 * memory starting at 0 will be ignored
		 */
		if (brcm_mmap.map[i].addr < firstUsableAddr) {
			printk("Error: Cannot reserve memory currently used by the kernel, requested=%08x, firstUsable=%08x\n",
				brcm_mmap.map[i].addr, firstUsableAddr);
			continue;
		}
		if (brcm_mmap.map[i].addr < (256<<20)) 
		{
                        if (upper_memory < brcm_mmap.map[i].addr) {
                                upper_memory = brcm_mmap.map[i].addr;
                        }

#ifndef CONFIG_DISCONTIGMEM
			reserve_bootmem(brcm_mmap.map[i].addr, 
				brcm_mmap.map[i].size);
#else
			reserve_bootmem_node(NODE_DATA(0), 
			                     brcm_mmap.map[i].addr, 
			                     brcm_mmap.map[i].size);		
#endif
		}
		else {
#ifndef CONFIG_DISCONTIGMEM
			uart_puts("Error: Cannot reserve memory above 256MB for UMA platforms\n");
#else
#if defined ( CONFIG_MIPS_BCM97438 )
                        reserve_bootmem_node(NODE_DATA(1),
                                 brcm_mmap.map[i].addr,
                                 brcm_mmap.map[i].size);
#elif defined ( CONFIG_MIPS_BCM7440 )
                        if ((brcm_mmap.map[i].addr >= (512  << 20)) &&
                                (brcm_mmap.map[i].addr <  (1024 << 20)) )
                        {
                                reserve_bootmem_node(NODE_DATA(1),
                                         brcm_mmap.map[i].addr,
                                         brcm_mmap.map[i].size);
                        }
#ifdef NEVER
                        else
                        {
                                reserve_bootmem_node(NODE_DATA(2),
                                         brcm_mmap.map[i].addr,
                                         brcm_mmap.map[i].size);
                        }
#endif

#endif

#endif

		}

	}
}

unsigned long
get_RSVD_size(void)
{
	int node;
	unsigned long res = 0;

	for (node = brcm_mmap.head; node != NILL; node = brcm_mmap.map[node].next) {
		if(brcm_mmap.map[node].type == BOOT_MEM_RESERVED)
			res += brcm_mmap.map[node].size;
	}
	printk("%s: Returning Size 0x%08lx\n", __FUNCTION__, res);
	return(res);
}
EXPORT_SYMBOL(get_RSVD_size);

#else
#define brcm_init_memory_map
#define brcm_insert_ram_node(start,size,type,bm)
#define brcm_default_boot_mem
#define brcm_print_memory_map
#define brcm_reserve_bootmem_node(firstUsableAddr)
#endif // #define CONFIG_MIPS_BRCM97XXX

static inline void parse_cmdline_early(void)
{
	char c = ' ', *to = command_line, *from = saved_command_line;
	unsigned long start_at, mem_size;
	int len = 0;
	int usermem = 0;

#if defined (CONFIG_MIPS_BCM7440)
        int retVal;
        phys_t rsvdSize;
        phys_t pgStructSize = sizeof(struct page);
        /*
        ** lower_mem_ram_size specifies the zero-based
        ** memory footprint required to boot linux,
        ** found to require 18MB to allow for the ISA
        ** bounce pool.
        **
        ** HARD-CODE ALERT:
        **  mem_bank_size = 256MB for 97440 with 7440A0 chip.
        */
        unsigned long mem_bank_size      = 256*1024*1024;
        unsigned long lower_mem_ram_size = 18*1024*1024;
        unsigned long upper_mem_ram_size = 0;
        static int    mmap_defaulted     = 0;
        static int    mmap_manual        = 0;
#endif


	printk("Determined physical RAM map:\n");
	print_memory_map();

	brcm_init_memory_map();

	for (;;) {
		/*
		 * "mem=XXX[kKmM]" defines a memory region from
		 * 0 to <XXX>, overriding the determined size.
		 * "mem=XXX[KkmM]@YYY[KkmM]" defines a memory region from
		 * <YYY> to <YYY>+<XXX>, overriding the determined size.
		 *
		 * The followings are BRCM extension to/deviation from the documentation:
		 * "mem=XXX[KkmM]$YYY[KkmM]" defines a memory region from
		 * <YYY> to <YYY>+<XXX> to be excluded from kernel usable RAM
		 * # is used to mark an ACPI region, but since we don't support it,
		 * we have a non-conformant behavior, by using # and $ interchangeably.
		 * This is done to allow the user to specify # in lieu of $ because
		 * the CFE would interprete the $, and thus requires an escape sequence.
		 */
		if (c == ' ' && !memcmp(from, "mem=", 4)) {
			if (to != command_line)
				to--;
			/*
			 * If a user specifies memory size, we
			 * blow away any automatically generated
			 * size.
			 */
			if (usermem == 0) {
#ifndef CONFIG_MIPS_BRCM
				boot_mem_map.nr_map = 0;
#endif
				usermem = 1;
			}
			mem_size = memparse(from + 4, &from);
#if defined (CONFIG_MIPS_BCM7440)
                        upper_mem_ram_size = 0;
#endif

			switch (*from) {
			case '@':
				start_at = memparse(from + 1, &from);
#if defined (CONFIG_MIPS_BCM7440)
                                if (mmap_defaulted) {
                                        /*
                                        ** This combination of boot parameters 
				        ** will surely cause problems !
                                        */
                                        printk("%s: ILLEGAL MEMORY SPECIFICATION ON BOOT COMMAND LINE - <size 0x%08lx, start 0x%08lx> IGNORED!!!\n",
                                                __FUNCTION__, mem_size, start_at);
                                        break;
                                }
                                mmap_manual = 1;
                                brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RAM, &brcm_bm);
#else
                                brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RAM, &brcm_bm);
#endif

				break;
			case '$':
			case '#': /* CFE will interprete $, so non-conformant alternate syntax is supported */
				start_at = memparse(from + 1, &from);
#if defined (CONFIG_MIPS_BCM7440)
                                {
                                        if (mmap_defaulted) {
                                                /*
                                                ** This combination of boot parameters will surely cause
                                                ** problems !
                                                */
                                                printk("%s: ILLEGAL MEMORY SPECIFICATION ON BOOT COMMAND LINE - <size 0x%08lx, start 0x%08lx> IGNORED!!!\n",
                                                        __FUNCTION__, mem_size, start_at);
                                                break;
                                        }
                                        mmap_manual = 1;

                                        /*
                                        ** We must reserve 12MB memory at physical address 0
                                        ** for Linux boot. Linux needs at least 32MB total.
                                        ** Otherwise, this is an illegal command line.
                                        */
                                        if (start_at < mem_bank_size) {

                                                if (start_at < lower_mem_ram_size)
                                                        start_at = lower_mem_ram_size;
                                                if ((start_at + mem_size) > (2*mem_bank_size - 32*1024*1024)) {
                                                        printk("%s: INVALID MEMORY SPECIFICATION ON BOOT COMMAND LINE - USE DEFAULTS!!!\n", __FUNCTION__);
                                                        brcm_default_boot_mem();
                                                        break;
                                                }

                                                /*
                                                ** Allow for the Linux BOOT_MEM_RAM allocation in
                                                ** the first (zero-based) node.
                                                */
                                                if ((start_at + mem_size) > mem_bank_size) {
                                                        /*
                                                        ** This will extend into the second memory bank
                                                        */
                                                        upper_mem_ram_size = (start_at + mem_size) - mem_bank_size;
                                                        lower_mem_ram_size = mem_size - upper_mem_ram_size;
                                                        retVal = brcm_insert_ram_node(start_at, lower_mem_ram_size, BOOT_MEM_RESERVED, &brcm_bm);
                                                }
                                                else {
                                                        retVal = brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RESERVED, &brcm_bm);
                                                }
                                        }
                                        else if (!brcm_test_ram_node(start_at, mem_size, BOOT_MEM_RESERVED)) {
                                                retVal = brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RESERVED, &brcm_bm);
                                                upper_mem_ram_size = 0;
                                        }

                                        while (upper_mem_ram_size) {

                                                unsigned long next_size;
                                                /*
                                                ** Reserve the remaining memory nodes.
                                                ** Carve off enough memory from the start of each
                                                ** discontiguous memory node to allow for storage
                                                ** of the page structs for the node. Also check if
                                                ** upper_mem_ram_size is GT the page struct storage
                                                ** allocation. If so, simply retain the larger of the
                                                ** two values for BOOT_MEM_RAM.
                                                **
                                                ** If brcm_bm.size returned from brcm_insert_ram_node()
                                                ** is 1MB or less, don't bother performing the calculation.
                                                ** The reserved node size is rounded up to the nearest MB.
                                                */
                                                rsvdSize = (((PFN_DOWN(brcm_bm.size) * pgStructSize) + 0xfffff) & ~0xfffff);
                                                if ((brcm_bm.addr & (mem_bank_size - 1)) == 0) {
                                                        brcm_bm.addr += rsvdSize;
                                                        next_size = brcm_bm.size - rsvdSize;
                                                }
                                                else {
                                                        next_size = brcm_bm.size;
                                                }
                                                if (upper_mem_ram_size <= next_size) {
                                                        next_size = upper_mem_ram_size;
                                                        upper_mem_ram_size = 0;
                                                }
                                                else {
                                                        upper_mem_ram_size -= next_size;
                                                }

                                                retVal = brcm_insert_ram_node(brcm_bm.addr, next_size, BOOT_MEM_RESERVED, &brcm_bm);

                                                if (!upper_mem_ram_size || retVal <= 0 || brcm_bm.size == 0) break;
                                        }
                                }
#else
                                brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RESERVED, &brcm_bm);
#endif

				break;
			default:

#if defined (CONFIG_MIPS_BCM7440)
                                {
                                        start_at = 0;
                                        if (mmap_manual || mmap_defaulted) {
                                                /*
                                                ** This combination of boot parameters will surely cause
                                                ** problems !
                                                */
                                                printk("%s: ILLEGAL MEMORY SPECIFICATION ON BOOT COMMAND LINE - <size 0x%08lx, start 0x%08lx> IGNORED!!!\n",
                                                        __FUNCTION__, mem_size, start_at);
                                                break;
                                        }
                                        mmap_defaulted = 1;

                                        /*
                                        ** Restrict the Linux BOOT_MEM_RAM allocation in
                                        ** the first (zero-based) node to minimum boot
                                        ** memory requirements. The remainder of the specified
                                        ** Linux memory allocation will come from the upper
                                        ** memory node(s).
                                        */
                                        if (mem_size > lower_mem_ram_size) {
                                                upper_mem_ram_size = mem_size - lower_mem_ram_size;
                                                retVal = brcm_insert_ram_node(start_at, lower_mem_ram_size, BOOT_MEM_RAM, &brcm_bm);
                                        }
                                        else {
                                                retVal = brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RAM, &brcm_bm);
                                        }

                                        while (retVal > 0 && brcm_bm.size > 0x100000) {
                                                /*
                                                ** Reserve the remaining memory nodes.
                                                ** Carve off enough memory from the start of each
                                                ** discontiguous memory node to allow for storage
                                                ** of the page structs for the node. Also check if
                                                ** upper_mem_ram_size is GT the page struct storage
                                                ** allocation. If so, simply retain the larger of the
                                                ** two values for BOOT_MEM_RAM.
                                                **
                                                ** If brcm_bm.size returned from brcm_insert_ram_node()
                                                ** is 1MB or less, don't bother performing the calculation.
                                                ** The reserved node size is rounded up to the nearest MB.
                                                */
                                                rsvdSize = (((PFN_DOWN(brcm_bm.size) * pgStructSize) + 0xfffff) & ~0xfffff);
                                                if (upper_mem_ram_size && upper_mem_ram_size > rsvdSize) {
                                                        rsvdSize = upper_mem_ram_size;
                                                }
                                                brcm_bm.addr += rsvdSize;
                                                brcm_bm.size -= rsvdSize;
                                                retVal = brcm_insert_ram_node(brcm_bm.addr, brcm_bm.size, BOOT_MEM_RESERVED, &brcm_bm);
                                        }
                                }
#else
                                start_at = 0;
                                brcm_insert_ram_node(start_at, mem_size, BOOT_MEM_RAM, &brcm_bm);
#endif

			}
			
		}
		c = *(from++);
		if (!c)
			break;
		if (CL_SIZE <= ++len)
			break;
		*(to++) = c;
	}
	*to = '\0';

	if (!usermem) {
		/* Default memory map */
		brcm_default_boot_mem();
	}

    	printk("User-defined physical RAM map:\n");
	brcm_print_memory_map();

}


static inline int parse_rd_cmdline(unsigned long* rd_start, unsigned long* rd_end)
{
	/*
	 * "rd_start=0xNNNNNNNN" defines the memory address of an initrd
	 * "rd_size=0xNN" it's size
	 */
	unsigned long start = 0;
	unsigned long size = 0;
	unsigned long end;
	char cmd_line[CL_SIZE];
	char *start_str;
	char *size_str;
	char *tmp;

	strcpy(cmd_line, command_line);
	*command_line = 0;
	tmp = cmd_line;
	/* Ignore "rd_start=" strings in other parameters. */
	start_str = strstr(cmd_line, "rd_start=");
	if (start_str && start_str != cmd_line && *(start_str - 1) != ' ')
		start_str = strstr(start_str, " rd_start=");
	while (start_str) {
		if (start_str != cmd_line)
			strncat(command_line, tmp, start_str - tmp);
		start = memparse(start_str + 9, &start_str);
		tmp = start_str + 1;
		start_str = strstr(start_str, " rd_start=");
	}
	if (*tmp)
		strcat(command_line, tmp);

	strcpy(cmd_line, command_line);
	*command_line = 0;
	tmp = cmd_line;
	/* Ignore "rd_size" strings in other parameters. */
	size_str = strstr(cmd_line, "rd_size=");
	if (size_str && size_str != cmd_line && *(size_str - 1) != ' ')
		size_str = strstr(size_str, " rd_size=");
	while (size_str) {
		if (size_str != cmd_line)
			strncat(command_line, tmp, size_str - tmp);
		size = memparse(size_str + 8, &size_str);
		tmp = size_str + 1;
		size_str = strstr(size_str, " rd_size=");
	}
	if (*tmp)
		strcat(command_line, tmp);

#ifdef CONFIG_MIPS64
	/* HACK: Guess if the sign extension was forgotten */
	if (start > 0x0000000080000000 && start < 0x00000000ffffffff)
		start |= 0xffffffff00000000;
#endif

	end = start + size;
	if (start && end) {
		*rd_start = start;
		*rd_end = end;
		return 1;
	}
	return 0;
}


static inline void bootmem_init(void)
{
	unsigned long start_pfn;
	unsigned long reserved_end = (unsigned long)&_end;
#ifndef CONFIG_SGI_IP27
	unsigned long first_usable_pfn;
#ifndef CONFIG_DISCONTIGMEM
        unsigned long bootmap_size;
#else
        unsigned long bootmap_size[NR_NODES];
#endif

	int i;
#endif
	unsigned long firstUsableAddr;
#ifdef CONFIG_BLK_DEV_INITRD
	int initrd_reserve_bootmem = 0;

	/* Board specific code should have set up initrd_start and initrd_end */
 	ROOT_DEV = Root_RAM0;
	if (parse_rd_cmdline(&initrd_start, &initrd_end)) {
		reserved_end = max(reserved_end, initrd_end);
		initrd_reserve_bootmem = 1;
	} else {
		unsigned long tmp;
		u32 *initrd_header;

#if 1 //def CONFIG_MIPS_BRCM
	extern void* __initramfs_start;
	extern void* __initramfs_end;
	
	//ROOT_DEV = Root_RAM0;
	initrd_start = (unsigned long) &__initramfs_start;
	initrd_end = (unsigned long) &__initramfs_end;
#endif

		tmp = ((reserved_end + PAGE_SIZE-1) & PAGE_MASK) - sizeof(u32) * 2;
		if (tmp < reserved_end)
			tmp += PAGE_SIZE;
		initrd_header = (u32 *)tmp;
		if (initrd_header[0] == 0x494E5244) {
			initrd_start = (unsigned long)&initrd_header[2];
			initrd_end = initrd_start + initrd_header[1];
			reserved_end = max(reserved_end, initrd_end);
			initrd_reserve_bootmem = 1;
		}
/* */
	}
#endif	/* CONFIG_BLK_DEV_INITRD */

	/*
	 * Partially used pages are not usable - thus
	 * we are rounding upwards.
	 */
	start_pfn = PFN_UP(CPHYSADDR(reserved_end));

#ifndef CONFIG_SGI_IP27
	/* Find the highest page frame number we have available.  */
	max_pfn = 0;
	first_usable_pfn = -1UL;
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end;

		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end = PFN_DOWN(boot_mem_map.map[i].addr
		      + boot_mem_map.map[i].size);

		if (start >= end)
			continue;
		if (end > max_pfn)
			max_pfn = end;
		if (start < first_usable_pfn) {
			if (start > start_pfn) {
				first_usable_pfn = start;
			} else if (end > start_pfn) {
				first_usable_pfn = start_pfn;
			}
		}
	}

	/*
	 * Determine low and high memory ranges
	 */
	max_low_pfn = max_pfn;
	if (max_low_pfn > MAXMEM_PFN) {
		max_low_pfn = MAXMEM_PFN;
#ifndef CONFIG_HIGHMEM
		/* Maximum memory usable is what is directly addressable */
		printk(KERN_WARNING "Warning only %ldMB will be used.\n",
		       MAXMEM >> 20);
		printk(KERN_WARNING "Use a HIGHMEM enabled kernel.\n");
#endif
	}

#ifdef CONFIG_HIGHMEM
	/*
	 * Crude, we really should make a better attempt at detecting
	 * highstart_pfn
	 */
	highstart_pfn = highend_pfn = max_pfn;
	if (max_pfn > MAXMEM_PFN) {
		highstart_pfn = MAXMEM_PFN;
		printk(KERN_NOTICE "%ldMB HIGHMEM available.\n",
		       (highend_pfn - highstart_pfn) >> (20 - PAGE_SHIFT));
	}
#endif

#ifndef CONFIG_DISCONTIGMEM
	/* Initialize the boot-time allocator with low memory only.  */
	bootmap_size = init_bootmem(first_usable_pfn, max_low_pfn);
#else
#if defined ( CONFIG_MIPS_BCM97438 )

	min_low_pfn = first_usable_pfn;
	bootmap_size[0] = init_bootmem_node(NODE_DATA(0),
                                            first_usable_pfn,
                                            PFN_UP(0),
                                            PFN_DOWN(0x10000000));

	if (g_board_RAM_size > 0x10000000) {
		bootmap_size[1] = init_bootmem_node(NODE_DATA(1),
	                                            PFN_UP(0x20000000),
		                                    PFN_UP(0x20000000),
		    PFN_DOWN((g_board_RAM_size - 0x10000000) + 0x20000000));
	} 
	else
		bootmap_size[1] = 0;
#elif defined ( CONFIG_MIPS_BCM7440 )
        /* ADT Assume 1GB of RAM for now, use get_RAM_size() otherwise */
        min_low_pfn = first_usable_pfn;
        /* first 256MB :  0x00000000 - 0x10000000 */
        bootmap_size[0] = init_bootmem_node(NODE_DATA(0),
                                            first_usable_pfn,
                                            PFN_UP(0),
                                            PFN_DOWN(0x10000000));
        if (g_board_RAM_size == 0x20000000)
        {
                /* next 256MB  :  0x20000000 - 0x30000000 */
                bootmap_size[1] = init_bootmem_node(NODE_DATA(1),
                                                    PFN_UP(0x20000000),
                                                    PFN_UP(0x20000000),
                                                    PFN_DOWN(0x30000000));
        }
        else
        {
                /* next 512MB  :  0x20000000 - 0x40000000 */
                bootmap_size[1] = init_bootmem_node(NODE_DATA(1),
                                                    PFN_UP(0x20000000),
                                                    PFN_UP(0x20000000),
                                                    PFN_DOWN(0x40000000));
#ifdef NEVER
                /* next 256MB  :  0x50000000 - 0x60000000 */
                bootmap_size[2] = init_bootmem_node(NODE_DATA(2),
                                                    PFN_UP(0x50000000),
                                                    PFN_UP(0x50000000),
                                                    PFN_DOWN(0x60000000));
#endif
        }

#else
           #error "Unsupported platform"
#endif
#endif



	/*
	 * Register fully available low RAM pages with the bootmem allocator.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long curr_pfn, last_pfn, size;

                printk(KERN_WARNING "bootmem_init: map %d type %d\n",
                                i, (int)boot_mem_map.map[i].type);

		/*
		 * Reserve usable memory.
		 */
		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		/*
		 * We are rounding up the start address of usable memory:
		 */
		curr_pfn = PFN_UP(boot_mem_map.map[i].addr);
		if (curr_pfn >= max_low_pfn)
			continue;
		if (curr_pfn < start_pfn)
			curr_pfn = start_pfn;

		/*
		 * ... and at the end of the usable range downwards:
		 */
		last_pfn = PFN_DOWN(boot_mem_map.map[i].addr
				    + boot_mem_map.map[i].size);

                printk(KERN_WARNING "bootmem_init: curr_pfn 0x%08lx last_pfn 0x%08lx max_low_pfn 0x%08lx\n", curr_pfn, last_pfn - 1, max_low_pfn);

		if (last_pfn > max_low_pfn)
			last_pfn = max_low_pfn;

		/*
		 * Only register lowmem part of lowmem segment with bootmem.
		 */
		size = last_pfn - curr_pfn;
		if (curr_pfn > PFN_DOWN(HIGHMEM_START))
			continue;
		if (curr_pfn + size - 1 > PFN_DOWN(HIGHMEM_START))
			size = PFN_DOWN(HIGHMEM_START) - curr_pfn;
		if (!size)
			continue;

		/*
		 * ... finally, did all the rounding and playing
		 * around just make the area go away?
		 */
		if (last_pfn <= curr_pfn)
			continue;

		/* Register lowmem ranges */
#ifndef CONFIG_DISCONTIGMEM
                free_bootmem(PFN_PHYS(curr_pfn), PFN_PHYS(size));
#else
                {
                        unsigned long paddr = PFN_PHYS(curr_pfn);
                        int node;

                        printk(KERN_WARNING "bootmem_init: curr_pfn 0x%08lx size 0x%08lx\n", curr_pfn, size);

                        /* Find the node  this memory map belongs to and free it */
                        for (node=0; node<numnodes; node++) {
                                printk(KERN_WARNING "bootmem_init: node %d (start)\n", node);
                                bootmem_data_t *bdata = NODE_DATA(node)->bdata;

                                if (paddr >= bdata->node_boot_start &&
                                    PFN_UP(paddr + size) < bdata->node_low_pfn)
				{

                                        printk("bootmem_init: Free node %d, NODE_DATA 0x%08x, PFN_PHYS(curr_pfn) 0x%08x, PFN_PHYS(size) 0x%08x\n",
                                                node, 
				 	        NODE_DATA(node), 
					        PFN_PHYS(curr_pfn), 
					        PFN_PHYS(size));
                                        free_bootmem_node(NODE_DATA(node), 
					                  PFN_PHYS(curr_pfn), 
					                  PFN_PHYS(size));
                                        break;
                                }
                                printk(KERN_WARNING "bootmem_init: node %d (end)\n", node);
                        }
                }
#endif

	}

	/* Reserve the bootmap memory.  */

#ifndef CONFIG_DISCONTIGMEM
	reserve_bootmem(PFN_PHYS(first_usable_pfn), bootmap_size);
	firstUsableAddr = PFN_PHYS(first_usable_pfn) + bootmap_size;
#else

        /*
         * reserve the memory bitmaps so they can't be allocated
         */
        printk("bootmem_init: Reserve node 0, NODE_DATA 0x%08x, PFN_PHYS(first_usable_pfn) 0x%08x, bootmap_size 0x%08x\n",
                        NODE_DATA(0), 
	                PFN_PHYS(first_usable_pfn), 
	                PFN_PHYS(bootmap_size[0]));
        reserve_bootmem_node(NODE_DATA(0), 
	                     PFN_PHYS(first_usable_pfn), 
	                     bootmap_size[0]);

#if defined (CONFIG_MIPS_BCM97438) || defined(CONFIG_MIPS_BCM7440)
        if (bootmap_size[1]) {
                printk("bootmem_init: Reserve node 1, NODE_DATA 0x%08x, addr 0x20000000, bootmap_size 0x%08x\n",
                                NODE_DATA(1), bootmap_size[1]);
                reserve_bootmem_node(NODE_DATA(1), 0x20000000, bootmap_size[1]);
        }
#ifdef NEVER
        if (bootmap_size[2]) {
                printk("bootmem_init: Reserve node 2, NODE_DATA 0x%08x, addr 0x50000000, bootmap_size 0x%08x\n",
                                NODE_DATA(2), bootmap_size[2]);
                reserve_bootmem_node(NODE_DATA(2), 0x50000000, bootmap_size[2]);
        }
#endif
        firstUsableAddr = PFN_PHYS(first_usable_pfn) + bootmap_size[0];
#endif

#endif // CONFIG_DISCONTIGMEM

#ifdef CONFIG_MIPS_BRCM
	brcm_reserve_bootmem_node(firstUsableAddr);

#endif

#endif /* !CONFIG_SGI_IP27 */



#ifdef CONFIG_BLK_DEV_INITRD
	initrd_below_start_ok = 1;
	if (initrd_start) {
		unsigned long initrd_size = ((unsigned char *)initrd_end) - ((unsigned char *)initrd_start);
		printk("Initial ramdisk at: 0x%p (%lu bytes)\n",
		       (void *)initrd_start, initrd_size);

		if (CPHYSADDR(initrd_end) > PFN_PHYS(max_low_pfn)) {
			printk("initrd extends beyond end of memory "
			       "(0x%0*Lx > 0x%0*Lx)\ndisabling initrd\n",
			       sizeof(long) * 2,
			       (unsigned long long)CPHYSADDR(initrd_end),
			       sizeof(long) * 2,
			       (unsigned long long)PFN_PHYS(max_low_pfn));
			initrd_start = initrd_end = 0;
			initrd_reserve_bootmem = 0;
		}

		if (initrd_reserve_bootmem)
			reserve_bootmem(CPHYSADDR(initrd_start), initrd_size);
	}
#endif /* CONFIG_BLK_DEV_INITRD  */
           printk(KERN_WARNING "bootmem_init: Done \n");

}

static inline void resource_init(void)
{
	int i;

#if defined(CONFIG_MIPS64) && !defined(CONFIG_BUILD_ELF64)
	/*
	 * The 64bit code in 32bit object format trick can't represent
	 * 64bit wide relocations for linker script symbols.
	 */
	code_resource.start = CPHYSADDR(&_text);
	code_resource.end = CPHYSADDR(&_etext) - 1;
	data_resource.start = CPHYSADDR(&_etext);
	data_resource.end = CPHYSADDR(&_edata) - 1;
#else
	code_resource.start = virt_to_phys(&_text);
	code_resource.end = virt_to_phys(&_etext) - 1;
	data_resource.start = virt_to_phys(&_etext);
	data_resource.end = virt_to_phys(&_edata) - 1;
#endif

	/*
	 * Request address space for all standard RAM.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		struct resource *res;
		unsigned long start, end;

		start = boot_mem_map.map[i].addr;
		end = boot_mem_map.map[i].addr + boot_mem_map.map[i].size - 1;
		if (start >= MAXMEM)
			continue;
		if (end >= MAXMEM)
			end = MAXMEM - 1;

		res = alloc_bootmem(sizeof(struct resource));
		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
		case BOOT_MEM_ROM_DATA:
			res->name = "System RAM";
			break;
		case BOOT_MEM_RESERVED:
		default:
			res->name = "reserved";
		}

		res->start = start;
		res->end = end;

		res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
		request_resource(&iomem_resource, res);

		/*
		 *  We don't know which RAM region contains kernel data,
		 *  so we try it repeatedly and let the resource manager
		 *  test it.
		 */
		request_resource(res, &code_resource);
		request_resource(res, &data_resource);
	}
}

#undef PFN_UP
#undef PFN_DOWN
#undef PFN_PHYS

#undef MAXMEM
#undef MAXMEM_PFN

void __init setup_arch(char **cmdline_p)
{
	cpu_probe();
	prom_init();
	cpu_report();
#ifdef CONFIG_DISCONTIGMEM
        cpu_cache_init();
        tlb_init();
#endif

#if defined(CONFIG_VT)
#if defined(CONFIG_VGA_CONSOLE)
        conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
        conswitchp = &dummy_con;
#endif
#endif

	/* call board setup routine */
	plat_setup();

	strlcpy(command_line, arcs_cmdline, sizeof(command_line));
	strlcpy(saved_command_line, command_line, COMMAND_LINE_SIZE);

	*cmdline_p = command_line;

	parse_cmdline_early();
	bootmem_init();
	paging_init();
	printk("Before resource_init\n");
	resource_init();
	printk("After resource_init\n");
}

static struct cpu cpu[1];

static int __init topology_init(void)
{
	return(register_cpu(cpu, 0, NULL));
}

subsys_initcall(topology_init);

int __init fpu_disable(char *s)
{
	cpu_data[0].options &= ~MIPS_CPU_FPU;

	return 1;
}

__setup("nofpu", fpu_disable);

int __init dsp_disable(char *s)
{
	cpu_data[0].options &= ~MIPS_ASE_DSP;

	return 1;
}

__setup("nodsp", dsp_disable);
