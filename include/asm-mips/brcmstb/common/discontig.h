/*
 * include/asm/brcmstb/common/discontig.h
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
 *defines for Broadcom eval boards with discontiguous memory model.
 *
 * 050106 THT Created
 * 60814  JWF 2.6 version
 */
/* JWF  this is specific to BRCM97438 with only 2 memory banks */

#ifndef __ASM_BRCMSTB_DISCONTIG_H
#define __ASM_BRCMSTB_DISCONTIG_H

#include <linux/mmzone.h>  

#if defined ( CONFIG_MIPS_BCM97438 )
#define NR_NODES	2
#define MAX_NR_NODES	2
#elif defined ( CONFIG_MIPS_BCM7440 )
#define NR_NODES	2
#define MAX_NR_NODES	2
#endif

extern pg_data_t discontig_node_data[];
extern struct page* node_mem_maps[NR_NODES];
extern int node_sizes[NR_NODES];
extern int numnodes;


/* PR30707: Picked up wrong NODE_DATA define */
#ifdef NODE_DATA
#undef NODE_DATA
#endif
#define NODE_DATA(nid)		(&discontig_node_data[nid])


#ifdef NODE_MEM_MAP
#undef NODE_MEM_MAP
#endif
#define NODE_MEM_MAP(nid)	(NODE_DATA(nid)->node_mem_map)


#if defined ( CONFIG_MIPS_BCM7440 )

#ifdef NEVER
#define HIGH_RAM_BASE   0x50000000UL    // 1280MB
#define HIGH_RAM_SIZE   0x10000000UL    //  256MB
#define HIGH_RAM_END    (HIGH_RAM_BASE + HIGH_RAM_SIZE)
#endif

#define UPPER_RAM_BASE  0x20000000UL     // 512MB
#define UPPER_RAM_SIZE  0x10000000UL     // 256MB
#define UPPER_RAM_END   (UPPER_RAM_BASE+UPPER_RAM_SIZE)

#else

#define UPPER_RAM_BASE  0x20000000UL	// 512MB
#define UPPER_RAM_SIZE 	0x10000000UL	// 256MB
#define UPPER_RAM_END	(UPPER_RAM_BASE+UPPER_RAM_SIZE)

#endif

#define LOWER_RAM_SIZE 	0x10000000UL	// 256MB
#define LOWER_RAM_END	0x10000000UL	// 256MB

#if defined ( CONFIG_MIPS_BCM7440 )

#ifdef NEVER
#define HIGH_RAM_VBASE  0xE0000000UL
#endif

#define UPPER_RAM_VBASE 0xd8000000UL    
#define LOWER_RAM_VBASE PAGE_OFFSET

#else
#define UPPER_RAM_VBASE 0xe0000000UL	
#define LOWER_RAM_VBASE	PAGE_OFFSET
#endif

#define IS_PA_UPPER_RAM(pa) (pa >= UPPER_RAM_BASE && pa < UPPER_RAM_END)
#define IS_KADDR_LOWER_RAM(kaddr) (kaddr >= LOWER_RAM_VBASE && kaddr < (LOWER_RAM_VBASE + LOWER_RAM_SIZE))
#define IS_KADDR_UPPER_RAM(kaddr) (kaddr >= UPPER_RAM_VBASE && kaddr < (UPPER_RAM_VBASE + UPPER_RAM_SIZE))

#if defined ( CONFIG_MIPS_BCM7440 )

#ifdef NEVER
#define IS_PA_HIGH_RAM(pa) (pa >= HIGH_RAM_BASE && pa < HIGH_RAM_END)
#define IS_KADDR_HIGH_RAM(kaddr) (kaddr >= HIGH_RAM_VBASE && kaddr < (HIGH_RAM_VBASE + HIGH_RAM_SIZE))
#endif

#endif


/* These funcitons fail for non-ram addresses */
static __inline__ unsigned long 
__pa(unsigned long x)
{

#if defined ( CONFIG_MIPS_BCM7440 )
#ifdef NEVER
	if (x == HIGH_RAM_END)
		return HIGH_RAM_END;
	else if (IS_KADDR_HIGH_RAM(x))
		return (HIGH_RAM_BASE + (x - HIGH_RAM_VBASE));
	else
#endif
#endif
	if (x == (UPPER_RAM_VBASE + UPPER_RAM_SIZE)) // For high_memory
		return UPPER_RAM_END;

	else if (IS_KADDR_UPPER_RAM(x))
		return (UPPER_RAM_BASE + (x - UPPER_RAM_VBASE));

	else if (x == (LOWER_RAM_VBASE + LOWER_RAM_SIZE)) // For high_memory with only one node
		return (LOWER_RAM_SIZE);

	else if (IS_KADDR_LOWER_RAM(x))
		return (x - PAGE_OFFSET);
		
	printk(KERN_ERR "__pa invaild address 0x%08lx\n", x);
	BUG();
}

static __inline__ void* 
__va(unsigned long x)
{

#if defined ( CONFIG_MIPS_BCM7440 )
#ifdef NEVER
	if (x >= HIGH_RAM_BASE && x <= HIGH_RAM_END)
                return ((void*) (HIGH_RAM_VBASE + x - HIGH_RAM_BASE));
	else
#endif

#endif
	if (x >= UPPER_RAM_BASE && x <= UPPER_RAM_END) /* Use <= for high_memory */
		return ((void*) (UPPER_RAM_VBASE + x - UPPER_RAM_BASE));
	else
		return ((void *)((unsigned long) (x) + PAGE_OFFSET));
}


static __inline__ int
pfn_valid( unsigned long pfn)
{
	int i;
	unsigned long first, last;

	for (i = 0; i < numnodes; i++) {
		first = NODE_DATA(i)->node_start_pfn;
		last = NODE_DATA(i)->node_start_pfn + NODE_DATA(i)->node_spanned_pages;

		if (pfn >= first && pfn < last)
			return (1);
	}
	return(0);
}

#if defined ( CONFIG_MIPS_BCM7440 )
#ifdef NEVER
#define kern_addr_valid(x) (IS_KADDR_LOWER_RAM((unsigned long )x) || IS_KADDR_UPPER_RAM((unsigned long )x) || IS_KADDR_HIGH_RAM((unsigned long )x))
#endif

#define kern_addr_valid(x) (IS_KADDR_LOWER_RAM((unsigned long )x) || IS_KADDR_UPPER_RAM((unsigned long )x))

#else
#define kern_addr_valid(x) (IS_KADDR_LOWER_RAM((unsigned long )x) || IS_KADDR_UPPER_RAM((unsigned long )x))

#endif
/*		__pa fails for invalid virtual addresses 
#define kern_addr_valid(x)  pfn_valid(__pa(x) >> PAGE_SHIFT)
*/


#ifdef notdef
/* this code is in linux/mm.h */
struct page * pfn_to_page(unsigned long  pfn) ;
unsigned long page_to_pfn( struct page *p);

struct page *
pfn_to_page(unsigned long  pfn) 
{

	int i;
 	unsigned long first, last;

	for (i = 0; i < numnodes; i++) {
		first = NODE_DATA(i)->node_start_pfn;
		last = first + NODE_DATA(i)->node_spanned_pages;

		if (pfn >= first && pfn < last) {
			return( NODE_MEM_MAP(i) + (pfn - first));
		}
	}
	return((struct page *)0);
}

unsigned long 
page_to_pfn( struct page *p)
{									
	int i;
	struct page *first, *last;

	for (i = 0; i < numnodes; i++) {
		first = NODE_MEM_MAP(i);
		last = first + NODE_DATA(i)->node_spanned_pages;

		if  (p >= first && p < last) {
			return( NODE_DATA(i)->node_start_pfn + (p - first));
		}
	}
	return(0);
}

#endif //notdef

#define page_to_phys(page) (page_to_pfn(page) << PAGE_SHIFT)

#ifdef notdef
#define virt_to_page(kaddr) \
	((struct page*) (((unsigned long) (kaddr)) < (unsigned long)__va(LOWER_RAM_END) \
		? NODE_MEM_MAP(0) + (__pa((unsigned long )kaddr) >> PAGE_SHIFT) \
		:  (IS_KADDR_UPPER_RAM((unsigned long)kaddr) \
			? NODE_MEM_MAP(1) + ((__pa((unsigned long )kaddr)  - UPPER_RAM_BASE) >> PAGE_SHIFT) \
			: 0)))

#else
#if defined ( CONFIG_MIPS_BCM7440 )
#ifdef NEVER
#define virt_to_page(kaddr) \
        ((struct page*) ((IS_KADDR_LOWER_RAM(kaddr))  \
                ? NODE_MEM_MAP(0) + (__pa((unsigned long )kaddr) >> PAGE_SHIFT) \
                :  (IS_KADDR_UPPER_RAM((unsigned long)kaddr) \
                        ? NODE_MEM_MAP(1) + ((__pa((unsigned long )kaddr)  - UPPER_RAM_BASE) >> PAGE_SHIFT) \
                        :  (IS_KADDR_HIGH_RAM((unsigned long)kaddr) \
                                ? NODE_MEM_MAP(2) + ((__pa((unsigned long )kaddr)  - HIGH_RAM_BASE) >> PAGE_SHIFT) \
                                : 0))))
#endif

#define virt_to_page(kaddr) \
        ((struct page*) ((IS_KADDR_LOWER_RAM(kaddr))  \
                ? NODE_MEM_MAP(0) + (__pa((unsigned long )kaddr) >> PAGE_SHIFT) \
                :  (IS_KADDR_UPPER_RAM((unsigned long)kaddr) \
                        ? NODE_MEM_MAP(1) + ((__pa((unsigned long )kaddr)  - UPPER_RAM_BASE) >> PAGE_SHIFT) \
                        : 0)))


#else
#define virt_to_page(kaddr) \
	((struct page*) ((IS_KADDR_LOWER_RAM(kaddr))  \
		? NODE_MEM_MAP(0) + (__pa((unsigned long )kaddr) >> PAGE_SHIFT) \
		:  (IS_KADDR_UPPER_RAM((unsigned long)kaddr) \
			? NODE_MEM_MAP(1) + ((__pa((unsigned long )kaddr)  - UPPER_RAM_BASE) >> PAGE_SHIFT) \
			: 0)))

#endif
#endif

#if defined ( CONFIG_MIPS_BCM7440 )
#ifdef NEVER
#define VALID_PAGE(page) \
        ((unsigned long)(page) >= (unsigned long)NODE_MEM_MAP(0) && \
         (unsigned long)(page) <  (unsigned long)(NODE_MEM_MAP(0) + \
                                            NODE_DATA(0)->node_spanned_pages) \
           ? 1 \
           : ((unsigned long)(page) >= (unsigned long)NODE_MEM_MAP(1) && \
              (unsigned long)(page) < (unsigned long)(NODE_MEM_MAP(1) + \
                                           NODE_DATA(1)->node_spanned_pages) \
                 ? 1 \
                 : ((unsigned long)(page) >= (unsigned long)NODE_MEM_MAP(2) && \
                    (unsigned long)(page) < (unsigned long)(NODE_MEM_MAP(2) + \
                                            NODE _DATA(2)->node_spanned_pages) \
                        ? 1 : 0))) 
#endif

#define VALID_PAGE(page) \
        ((unsigned long) (page) >= (unsigned long) NODE_MEM_MAP(0) && \
         (unsigned long) (page) < (unsigned long) (NODE_MEM_MAP(0) + NODE_DATA(0)->node_spanned_pages) \
                ? 1 \
                : ((unsigned long) (page) >= (unsigned long) NODE_MEM_MAP(1) && \
                (unsigned long) (page) < (unsigned long) (NODE_MEM_MAP(1) + NODE_DATA(1)->node_spanned_pages) \
                        ? 1 : 0))


#else	

#define VALID_PAGE(page) \
	((unsigned long) (page) >= (unsigned long) NODE_MEM_MAP(0) && \
	 (unsigned long) (page) < (unsigned long) (NODE_MEM_MAP(0) + NODE_DATA(0)->node_spanned_pages) \
		? 1 \
		: ((unsigned long) (page) >= (unsigned long) NODE_MEM_MAP(1) && \
		(unsigned long) (page) < (unsigned long) (NODE_MEM_MAP(1) + NODE_DATA(1)->node_spanned_pages) \
			? 1 : 0))

#endif

#define virt_addr_valid(kaddr) (pfn_valid(__pa((unsigned long)kaddr) >> PAGE_SHIFT))


#define nth_page(page,n) pfn_to_page(page_to_pfn((page)) + (n))

#ifdef notdef
static inline void *
lowmem_page_address(struct page *page)
{
	return __va(page_to_pfn(page) << PAGE_SHIFT);
}
#endif
#endif
