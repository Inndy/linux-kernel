#ifndef __LINUX_GFP_H
#define __LINUX_GFP_H

#include <linux/config.h>
#include <linux/mmzone.h>
#include <linux/stddef.h>
#include <linux/linkage.h>


#if defined( CONFIG_MIPS_BRCM97XXX ) && defined ( CONFIG_DISCONTIGMEM )
// Need this to get proper definition for NODE_DATA
#include <asm/brcmstb/common/discontig.h>
#endif

struct vm_area_struct;

/*
 * GFP bitmasks..
 */
/* Zone modifiers in GFP_ZONEMASK (see linux/mmzone.h - low two bits) */
#define __GFP_DMA	0x01
#define __GFP_HIGHMEM	0x02

/*
 * Action modifiers - doesn't change the zoning
 *
 * __GFP_REPEAT: Try hard to allocate the memory, but the allocation attempt
 * _might_ fail.  This depends upon the particular VM implementation.
 *
 * __GFP_NOFAIL: The VM implementation _must_ retry infinitely: the caller
 * cannot handle allocation failures.
 *
 * __GFP_NORETRY: The VM implementation must not retry indefinitely.
 */
#define __GFP_WAIT	0x10u	/* Can wait and reschedule? */
#define __GFP_HIGH	0x20u	/* Should access emergency pools? */
#define __GFP_IO	0x40u	/* Can start physical IO? */
#define __GFP_FS	0x80u	/* Can call down to low-level FS? */
#define __GFP_COLD	0x100u	/* Cache-cold page required */
#define __GFP_NOWARN	0x200u	/* Suppress page allocation failure warning */
#define __GFP_REPEAT	0x400u	/* Retry the allocation.  Might fail */
#define __GFP_NOFAIL	0x800u	/* Retry for ever.  Cannot fail */
#define __GFP_NORETRY	0x1000u	/* Do not retry.  Might fail */
#define __GFP_NO_GROW	0x2000u	/* Slab internal usage */
#define __GFP_COMP	0x4000u	/* Add compound page metadata */
#define __GFP_ZERO	0x8000u	/* Return zeroed page on success */
#define __GFP_NOMEMALLOC 0x10000u /* Don't use emergency reserves */

#define __GFP_BITS_SHIFT 20	/* Room for 20 __GFP_FOO bits */
#define __GFP_BITS_MASK ((1 << __GFP_BITS_SHIFT) - 1)

/* if you forget to add the bitmask here kernel will crash, period */
#define GFP_LEVEL_MASK (__GFP_WAIT|__GFP_HIGH|__GFP_IO|__GFP_FS| \
			__GFP_COLD|__GFP_NOWARN|__GFP_REPEAT| \
			__GFP_NOFAIL|__GFP_NORETRY|__GFP_NO_GROW|__GFP_COMP| \
			__GFP_NOMEMALLOC)

#define GFP_ATOMIC	(__GFP_HIGH)
#define GFP_NOIO	(__GFP_WAIT)
#define GFP_NOFS	(__GFP_WAIT | __GFP_IO)
#define GFP_KERNEL	(__GFP_WAIT | __GFP_IO | __GFP_FS)
#define GFP_USER	(__GFP_WAIT | __GFP_IO | __GFP_FS)
#define GFP_HIGHUSER	(__GFP_WAIT | __GFP_IO | __GFP_FS | __GFP_HIGHMEM)

/* Flag - indicates that the buffer will be suitable for DMA.  Ignored on some
   platforms, used as appropriate on others */

#define GFP_DMA		__GFP_DMA


/*
 * There is only one page-allocator function, and two main namespaces to
 * it. The alloc_page*() variants return 'struct page *' and as such
 * can allocate highmem pages, the *get*page*() variants return
 * virtual kernel addresses to the allocated page(s).
 */

/*
 * We get the zone list from the current node and the gfp_mask.
 * This zone list contains a maximum of MAXNODES*MAX_NR_ZONES zones.
 *
 * For the normal case of non-DISCONTIGMEM systems the NODE_DATA() gets
 * optimized to &contig_page_data at compile-time.
 */

#ifndef HAVE_ARCH_FREE_PAGE
static inline void arch_free_page(struct page *page, int order) { }
#endif

#ifdef CONFIG_DISCONTIGMEM
extern int numnodes;
#endif


extern struct page *
FASTCALL(__alloc_pages(unsigned int, unsigned int, struct zonelist *));

static inline struct page *alloc_pages_node(int nid, unsigned int __nocast gfp_mask,
						unsigned int order)
{
#ifdef CONFIG_DISCONTIGMEM
	struct page *page =  (struct page *)0;
#endif

	if (unlikely(order >= MAX_ORDER))
		return NULL;

#ifdef CONFIG_DISCONTIGMEM
       /*
        * For Bcm97438 implementation, we know that if gpf_mask requires DMA
        * we need to use the lower bank so we have no choice, but use NODE 0
        */
       if ((numnodes == 1) || (gfp_mask & __GFP_DMA)) {
/* 
           printk("alloc_pages_node: trying node 0 GFP_DMA mask 0x%x\n", gfp_mask);
 */
           return __alloc_pages(gfp_mask, order,
                       NODE_DATA(0)->node_zonelists + (gfp_mask & GFP_ZONEMASK));
       }

       /* try upper memory node first, then lower, but only when memory is really there */
/* 
        printk("alloc_pages_node: trying node 1 mask 0x%x\n", gfp_mask);
*/
	if (NODE_DATA(1)->node_zonelists != NULL) {
		struct zonelist* zlist = NODE_DATA(1)->node_zonelists + (gfp_mask & GFP_ZONEMASK);
		if (zlist->zones[0] != NULL) {
			
	       	page = __alloc_pages(gfp_mask, order,
	                       NODE_DATA(1)->node_zonelists + (gfp_mask & GFP_ZONEMASK));
		
		}
	}
       if (page == (struct page *)0) {
/*
           printk("alloc_pages_node: trying node 0 mask 0x%x\n", gfp_mask);
*/
           page = __alloc_pages(gfp_mask, order,
                       NODE_DATA(0)->node_zonelists + (gfp_mask & GFP_ZONEMASK));
       }
       return(page);
#else

	return __alloc_pages(gfp_mask, order,
		NODE_DATA(nid)->node_zonelists + (gfp_mask & GFP_ZONEMASK));
#endif

}

#ifdef CONFIG_NUMA
extern struct page *alloc_pages_current(unsigned int __nocast gfp_mask, unsigned order);

static inline struct page *
alloc_pages(unsigned int __nocast gfp_mask, unsigned int order)
{
	if (unlikely(order >= MAX_ORDER))
		return NULL;

	return alloc_pages_current(gfp_mask, order);
}
extern struct page *alloc_page_vma(unsigned __nocast gfp_mask,
			struct vm_area_struct *vma, unsigned long addr);
#else
#if defined (CONFIG_MIPS_BCM7440)
#define alloc_pages(gfp_mask, order) \
			alloc_pages_node(0, gfp_mask, order)
#else
#define alloc_pages(gfp_mask, order) \
			alloc_pages_node(numa_node_id(), gfp_mask, order)
#endif

#define alloc_page_vma(gfp_mask, vma, addr) alloc_pages(gfp_mask, 0)
#endif
#define alloc_page(gfp_mask) alloc_pages(gfp_mask, 0)

extern unsigned long FASTCALL(__get_free_pages(unsigned int __nocast gfp_mask, unsigned int order));
extern unsigned long FASTCALL(get_zeroed_page(unsigned int __nocast gfp_mask));

#define __get_free_page(gfp_mask) \
		__get_free_pages((gfp_mask),0)

#define __get_dma_pages(gfp_mask, order) \
		__get_free_pages((gfp_mask) | GFP_DMA,(order))

extern void FASTCALL(__free_pages(struct page *page, unsigned int order));
extern void FASTCALL(free_pages(unsigned long addr, unsigned int order));
extern void FASTCALL(free_hot_page(struct page *page));
extern void FASTCALL(free_cold_page(struct page *page));

#define __free_page(page) __free_pages((page), 0)
#define free_page(addr) free_pages((addr),0)

void page_alloc_init(void);

#endif /* __LINUX_GFP_H */
