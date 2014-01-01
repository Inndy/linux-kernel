#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/swap.h>
#ifdef CONFIG_DISCONTIGMEM
extern unsigned long max_pfn;
#endif


void show_mem(void)
{
	int pfn, total = 0, reserved = 0;
	int shared = 0, cached = 0;
	int highmem = 0;
	struct page *page;
	int i;

	printk("Mem-info:\n");
	show_free_areas();
	printk("Free swap:       %6ldkB\n", nr_swap_pages<<(PAGE_SHIFT-10));

#ifdef CONFIG_DISCONTIGMEM
	pfn = max_pfn;
	printk("max pfn 0x%08x\n", pfn);
	for (i = 0; i < numnodes; i++) {
		unsigned long first, last, size;

		first = NODE_DATA(i)->node_start_pfn;
		size =  NODE_DATA(i)->node_spanned_pages;
		last = first + size - 1;
		printk("node %d first 0x%08lx last 0x%08lx size 0x%08lx\n",
				i, first, last, size);
	}
#else
	pfn = max_mapnr;
#endif

	while (pfn-- > 0) {
		page = pfn_to_page(pfn);
#ifdef CONFIG_DISCONTIGMEM
                if (page == (struct page *)0) {
                        continue;
                }
#endif

		total++;
		if (PageHighMem(page))
			highmem++;
		if (PageReserved(page))
			reserved++;
		else if (PageSwapCache(page))
			cached++;
		else if (page_count(page))
			shared += page_count(page) - 1;
	}
	printk("%d pages of RAM\n", total);
	printk("%d pages of HIGHMEM\n",highmem);
	printk("%d reserved pages\n",reserved);
	printk("%d pages shared\n",shared);
	printk("%d pages swap cached\n",cached);
}
