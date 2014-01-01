/*
 * linux/arch/mips/brcmstb/common/discontig.c
 *
 * Discontiguous memory support.
 *
 * Copyright (C) 2006 Broadcom Corporation
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
 * when   who what
 * 060501 tht Initial codings for 97438 NUMA
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <asm/mmzone.h>

int numnodes = 1;

int cache_debug;

/*
 * Our node_data structure for discontiguous memory.
 */

static bootmem_data_t node_bootmem_data[NR_NODES];

pg_data_t discontig_node_data[NR_NODES] = {
#if defined ( CONFIG_MIPS_BCM97438 )
  { bdata: &node_bootmem_data[0] },
  { bdata: &node_bootmem_data[1] },
#elif defined ( CONFIG_MIPS_BCM7440 )
  { bdata: &node_bootmem_data[0] },
  { bdata: &node_bootmem_data[1] },
#ifdef NEVER
  { bdata: &node_bootmem_data[2] },
#endif
#else
#error "Neither BCM97438 nor BCM7440 is supported"
#endif
};

struct page  *node_mem_maps[NR_NODES];

EXPORT_SYMBOL(numnodes);
EXPORT_SYMBOL(node_mem_maps);

int node_sizes[NR_NODES];
EXPORT_SYMBOL(node_sizes);

EXPORT_SYMBOL(discontig_node_data);

unsigned long get_RAM_size(void);

void __init brcm_numa_init(void) 
{
	unsigned int ram_size;
	/*
	 * first 256Mb is in 'node' 0.
	 * second 256Mb is in 'node' 1.
	 */
	ram_size = get_RAM_size();

	if (ram_size <= (256 << 20)) 
		numnodes = 1;
	else
		numnodes = NR_NODES;

	/* limit numnodes to 1 */
//	numnodes = 1;
	printk("brcm_numa_init: numnodes = %d\n", numnodes);
}

