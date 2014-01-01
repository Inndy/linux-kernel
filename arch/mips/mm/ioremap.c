/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * (C) Copyright 1995 1996 Linus Torvalds
 * (C) Copyright 2001, 2002 Ralf Baechle
 */
#include <linux/module.h>
#include <asm/addrspace.h>
#include <asm/byteorder.h>

#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#if defined( CONFIG_DISCONTIGMEM ) && defined( CONFIG_MIPS_BRCM97XXX )
#include <asm/brcmstb/common/discontig.h>
#endif


static inline void remap_area_pte(pte_t * pte, unsigned long address,
	phys_t size, phys_t phys_addr, unsigned long flags)
{
	phys_t end;
	unsigned long pfn;
	pgprot_t pgprot = __pgprot(_PAGE_GLOBAL | _PAGE_PRESENT | __READABLE
	                           | __WRITEABLE | flags);

	address &= ~PMD_MASK;
	end = address + size;
	if (end > PMD_SIZE)
		end = PMD_SIZE;
	if (address >= end)
		BUG();
	pfn = phys_addr >> PAGE_SHIFT;
	do {
		if (!pte_none(*pte)) {
			printk("remap_area_pte: page already exists\n");
			BUG();
		}
		set_pte(pte, pfn_pte(pfn, pgprot));
		address += PAGE_SIZE;
		pfn++;
		pte++;
	} while (address && (address < end));
}

static inline int remap_area_pmd(pmd_t * pmd, unsigned long address,
	phys_t size, phys_t phys_addr, unsigned long flags)
{
	phys_t end;

	address &= ~PGDIR_MASK;
	end = address + size;
	if (end > PGDIR_SIZE)
		end = PGDIR_SIZE;
	phys_addr -= address;
	if (address >= end)
		BUG();
	do {
		pte_t * pte = pte_alloc_kernel(&init_mm, pmd, address);
		if (!pte)
			return -ENOMEM;
		remap_area_pte(pte, address, end - address, address + phys_addr, flags);
		address = (address + PMD_SIZE) & PMD_MASK;
		pmd++;
	} while (address && (address < end));
	return 0;
}

static int remap_area_pages(unsigned long address, phys_t phys_addr,
	phys_t size, unsigned long flags)
{
	int error;
	pgd_t * dir;
	unsigned long end = address + size;

	phys_addr -= address;
	dir = pgd_offset(&init_mm, address);
	flush_cache_all();
	if (address >= end)
		BUG();
	spin_lock(&init_mm.page_table_lock);
	do {
		pud_t *pud;
		pmd_t *pmd;

		error = -ENOMEM;
		pud = pud_alloc(&init_mm, dir, address);
		if (!pud)
			break;
		pmd = pmd_alloc(&init_mm, pud, address);
		if (!pmd)
			break;
		if (remap_area_pmd(pmd, address, end - address,
					 phys_addr + address, flags))
			break;
		error = 0;
		address = (address + PGDIR_SIZE) & PGDIR_MASK;
		dir++;
	} while (address && (address < end));
	spin_unlock(&init_mm.page_table_lock);
	flush_tlb_all();
	return error;
}

/*
 * Generic mapping function (not visible outside):
 */

/*
 * Remap an arbitrary physical address space into the kernel virtual
 * address space. Needed when the kernel wants to access high addresses
 * directly.
 *
 * NOTE! We need to allow non-page-aligned mappings too: we will obviously
 * have to convert them into an offset in a page-aligned mapping, but the
 * caller shouldn't need to know that small detail.
 */

#define IS_LOW512(addr) (!((phys_t)(addr) & (phys_t) ~0x1fffffffULL))

void * __ioremap(phys_t phys_addr, phys_t size, unsigned long flags)
{
	struct vm_struct * area;
	unsigned long offset;
	phys_t last_addr;
	void * addr;

	phys_addr = fixup_bigphys_addr(phys_addr, size);

	/* Don't allow wraparound or zero size */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	/*
	 * Map uncached objects in the low 512mb of address space using KSEG1,
	 * otherwise map using page tables.
	 */
	if (IS_LOW512(phys_addr) && IS_LOW512(last_addr) &&
	    flags == _CACHE_UNCACHED)
		return (void *) CKSEG1ADDR(phys_addr);

#ifdef CONFIG_DISCONTIGMEM
#if defined ( CONFIG_MIPS_BCM97438 )
       if (IS_PA_UPPER_RAM(phys_addr) && flags == _CACHE_UNCACHED) {
               printk(KERN_ERR "Upper DDR at %08lx cannot be mapped uncached\n", phys_addr);
               return NULL;
       }
#elif defined ( CONFIG_MIPS_BCM7440 )
        if (IS_PA_UPPER_RAM(phys_addr) && (flags == _CACHE_UNCACHED)) {
               printk(KERN_ERR "Upper/High DDR at %08lx cannot be mapped uncached\n", phys_addr);
               return NULL;
       }

#endif

#endif

#ifndef CONFIG_DISCONTIGMEM
  #ifdef CONFIG_MIPS_BRCM97XXX

  #if defined( CONFIG_MIPS_BCM7038A0 )
	if (((phys_addr >= 0xd0000000) && (phys_addr <= 0xe060000b)))
		
  #elif defined( CONFIG_MIPS_BCM7038B0 ) || defined( CONFIG_MIPS_BCM7038C0 ) \
  	|| defined( CONFIG_MIPS_BCM7400 ) 
	if (((phys_addr >= 0xd0000000) && (phys_addr <= 0xf060000b)))
		
  #elif defined( CONFIG_MIPS_BCM3560 ) \
  	|| defined( CONFIG_MIPS_BCM7401 ) || defined( CONFIG_MIPS_BCM7402 ) \
	|| defined( CONFIG_MIPS_BCM7118 ) || defined( CONFIG_MIPS_BCM7403 ) \
	|| defined( CONFIG_MIPS_BCM7452 )
  	if (((((unsigned long) (phys_addr)) >= 0xd0000000) && (((unsigned long) (phys_addr)) <= 0xf060000b)) ||
		(((unsigned long) (phys_addr)) >= 0xff400000))
		
  #else
	if (phys_addr >= 0xffe00000)
  #endif
  
    	return (void *) (phys_addr);
  #endif
#else
  /* 97438 Discontiguous memory model */
  #if defined ( CONFIG_MIPS_BCM97438 )
        if (((phys_addr >= 0xd0000000) && (phys_addr < 0xe0000000)) ||
               ((phys_addr >= 0xf0000000) && (phys_addr <= 0xf060000b)))
                        return (void *) (phys_addr);

       /* else upper ram area is handled just like lower ram, handled below */
  #elif defined ( CONFIG_MIPS_BCM7440 )
        if ((phys_addr >= 0xd0000000) && (phys_addr < 0xd8000000))
                /* 128 MB of PCI-MEM */
                return (void *) (phys_addr);
        if ((phys_addr >= 0xf0000000) && (phys_addr < 0xf2000000))
                /* 32 MB of PCI-IO */
                return (void *) (0xf8000000 + (phys_addr - 0xf0000000));

  #else
       #error "Unsupported discontigmem platform"
  #endif

#endif

	/*
	 * Don't allow anybody to remap normal RAM that we're using..
	 */
	if (phys_addr < virt_to_phys(high_memory)) {
		char *t_addr, *t_end;
		struct page *page;

		t_addr = __va(phys_addr);
		t_end = t_addr + (size - 1);

		for(page = virt_to_page(t_addr); page <= virt_to_page(t_end); page++)
			if(!PageReserved(page))
				return NULL;
	}

	/*
	 * Mappings have to be page-aligned
	 */
	offset = phys_addr & ~PAGE_MASK;
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(last_addr + 1) - phys_addr;

	/*
	 * Ok, go for it..
	 */
	area = get_vm_area(size, VM_IOREMAP);
	if (!area)
		return NULL;
	addr = area->addr;
	if (remap_area_pages((unsigned long) addr, phys_addr, size, flags)) {
		vunmap(addr);
		return NULL;
	}

	return (void *) (offset + (char *)addr);
}

#define IS_KSEG1(addr) (((unsigned long)(addr) & ~0x1fffffffUL) == CKSEG1)

void __iounmap(volatile void __iomem *addr)
{
	struct vm_struct *p;

	if (IS_KSEG1(addr))
		return;

#ifdef CONFIG_MIPS_BRCM97XXX
  #ifndef CONFIG_DISCONTIGMEM
  #if defined( CONFIG_MIPS_BCM7038A0 )
	if ( (((unsigned long)addr >= 0xd0000000) && ((unsigned long)addr <= 0xe060000b)))
		return;
	
  #elif defined( CONFIG_MIPS_BCM7038B0 ) || defined( CONFIG_MIPS_BCM7038C0 ) \
  	|| defined( CONFIG_MIPS_BCM7400 )
	if ( (((unsigned long)addr >= 0xd0000000) && ((unsigned long)addr <= 0xf060000b)))
		return;

  #elif defined( CONFIG_MIPS_BCM3560 ) \
  	|| defined( CONFIG_MIPS_BCM7401 ) || defined( CONFIG_MIPS_BCM7402 ) \
 	|| defined( CONFIG_MIPS_BCM7118 ) || defined( CONFIG_MIPS_BCM7403 ) \
	|| defined( CONFIG_MIPS_BCM7452 )
  	if (((((unsigned long) (addr)) >= 0xd0000000) && (((unsigned long) (addr)) <= 0xf060000b)) ||
		(((unsigned long) (addr)) >= 0xff400000))
		return;
	
  #else
	if ((unsigned long)addr >= 0xffe00000)
		return;
  #endif
  #else
        #if defined(CONFIG_MIPS_BCM97438 )
           if ((((unsigned long)addr >= 0xd0000000) && ((unsigned long)addr < 0xe0000000)) ||
                           (((unsigned long)addr >= 0xf0000000) && ((unsigned long)addr <= 0xf060000b)))
                                                return;

           /* else upper ram area is handled just like lower ram, handled below */
        #elif defined ( CONFIG_MIPS_BCM7440 )
                if (((unsigned long)addr >= 0xd0000000) &&
                        ((unsigned long)addr <  0xd8000000))
                                /* 128 MB of PCI-MEM */
                                return;
                if (((unsigned long)addr >= 0xf8000000) &&
                        ((unsigned long)addr <  0xfa000000))
                                /* 32 MB of PCI-IO */
                                return;


        #else
           #error "Unsupported discontigmem platform"
        #endif
  #endif

#endif

	p = remove_vm_area((void *) (PAGE_MASK & (unsigned long __force) addr));
	if (!p) {
		printk(KERN_ERR "iounmap: bad address %p\n", addr);
		return;
	}

	kfree(p);
}

EXPORT_SYMBOL(__ioremap);
EXPORT_SYMBOL(__iounmap);
