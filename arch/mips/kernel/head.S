/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * Written by Ralf Baechle and Andreas Busse
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2003 Ralf Baechle
 * Copyright (C) 1996 Paul M. Antoine
 * Modified for DECStation and hence R3000 support by Paul M. Antoine
 * Further modifications by David S. Miller and Harald Koerfgen
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Kevin Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/threads.h>

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/page.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#ifdef CONFIG_SGI_IP27
#include <asm/sn/addrs.h>
#include <asm/sn/sn0/hubni.h>
#include <asm/sn/klkernvars.h>
#endif

#ifdef CONFIG_BRCM_PCI_SLAVE
#if defined( CONFIG_MIPS_BCM7402S ) 
#include <asm/brcmstb/brcm97401a0/bcmmips.h>
#include <asm/brcmstb/brcm97401a0/bcmuart.h>


#define BCHP_SUN_TOP_CTRL_SW_RESET 	            		0x00404010
#define BCHP_SUN_TOP_CTRL_SEMAPHORE_8          	0x00404048
#define BCHP_SUN_GISB_ARB_TIMER                      	0x0040600c
#define BCHP_SUN_GISB_ARB_REQ_MASK               		0x00406008
#define BCHP_FMISC_SOFT_RESET                        	0x00104000 
#define BCHP_MEMC_0_DDR_PLL_FREQ_DIVIDER_RESET	0x0010683c
#define BCHP_MEMC_0_DDR_PLL_FREQ_CNTL            	0x00106818
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5         	0x00404098
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8         	0x004040a4 

#define BCHP_SUN_TOP_CTRL_RESET_CTRL			0x00404008
//#define BCHP_SUN_TOP_CTRL_SW_RESET			0x00404014
#define BCHP_VCXO_0_RM_CONTROL                   		0x00181700 /* Rate Manager Controls */
#define UART_ADR_BASE		0xb04001a0		// UARTB

#elif defined( CONFIG_MIPS_BCM7402B0S )
#include <asm/brcmstb/brcm97401b0/bcmmips.h>
#include <asm/brcmstb/brcm97401b0/bcmuart.h>


#define BCHP_SUN_TOP_CTRL_SW_RESET 	            		0x00404014
#define BCHP_SUN_TOP_CTRL_SEMAPHORE_8          	0x00404050
#define BCHP_SUN_GISB_ARB_TIMER                      	0x0040600c
#define BCHP_SUN_GISB_ARB_REQ_MASK               		0x00406008
#define BCHP_FMISC_SOFT_RESET                        	0x00104000 
#define BCHP_MEMC_0_DDR_PLL_FREQ_DIVIDER_RESET	0x0010683c
#define BCHP_MEMC_0_DDR_PLL_FREQ_CNTL            	0x00106818
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7         	0x004040b0 
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_10         	0x004040bc 

#define BCHP_SUN_TOP_CTRL_RESET_CTRL			0x00404008
//#define BCHP_SUN_TOP_CTRL_SW_RESET			0x00404014
#define BCHP_VCXO_0_RM_CONTROL                   		0x00181700 /* Rate Manager Controls */
//#define UART_ADR_BASE		0xb04001a0		// UARTB

#define BAUD_VAL(x)     	(((XTALFREQ/8/(x) + 1)/2) - 1)
#define BAUD_VAL_LO(x)  	(BAUD_VAL(x) & 0xff)
#define BAUD_VAL_HI(x)  	((BAUD_VAL(x) >> 8) & 0xff)

#define BAUD_115200_LO  	BAUD_VAL_LO(115200)
#define BAUD_115200_HI  	BAUD_VAL_HI(115200)

#endif /* 7402s, 7402b0s */

#endif /* CONFIG_BRCM_PCI_SLAVE */

	.macro	ARC64_TWIDDLE_PC
#if defined(CONFIG_ARC64) || defined(CONFIG_MAPPED_KERNEL)
	/* We get launched at a XKPHYS address but the kernel is linked to
	   run at a KSEG0 address, so jump there.  */
	PTR_LA	t0, \@f
	jr	t0
\@:
#endif
	.endm

#ifdef CONFIG_SGI_IP27
	/*
	 * outputs the local nasid into res.  IP27 stuff.
	 */
	.macro GET_NASID_ASM res
	dli	\res, LOCAL_HUB_ADDR(NI_STATUS_REV_ID)
	ld	\res, (\res)
	and	\res, NSRI_NODEID_MASK
	dsrl	\res, NSRI_NODEID_SHFT
	.endm
#endif /* CONFIG_SGI_IP27 */

	/*
	 * inputs are the text nasid in t1, data nasid in t2.
	 */
	.macro MAPPED_KERNEL_SETUP_TLB
#ifdef CONFIG_MAPPED_KERNEL
	/*
	 * This needs to read the nasid - assume 0 for now.
	 * Drop in 0xffffffffc0000000 in tlbhi, 0+VG in tlblo_0,
	 * 0+DVG in tlblo_1.
	 */
	dli	t0, 0xffffffffc0000000
	dmtc0	t0, CP0_ENTRYHI
	li	t0, 0x1c000		# Offset of text into node memory
	dsll	t1, NASID_SHFT		# Shift text nasid into place
	dsll	t2, NASID_SHFT		# Same for data nasid
	or	t1, t1, t0		# Physical load address of kernel text
	or	t2, t2, t0		# Physical load address of kernel data
	dsrl	t1, 12			# 4K pfn
	dsrl	t2, 12			# 4K pfn
	dsll	t1, 6			# Get pfn into place
	dsll	t2, 6			# Get pfn into place
	li	t0, ((_PAGE_GLOBAL|_PAGE_VALID| _CACHE_CACHABLE_COW) >> 6)
	or	t0, t0, t1
	mtc0	t0, CP0_ENTRYLO0	# physaddr, VG, cach exlwr
	li	t0, ((_PAGE_GLOBAL|_PAGE_VALID| _PAGE_DIRTY|_CACHE_CACHABLE_COW) >> 6)
	or	t0, t0, t2
	mtc0	t0, CP0_ENTRYLO1	# physaddr, DVG, cach exlwr
	li	t0, 0x1ffe000		# MAPPED_KERN_TLBMASK, TLBPGMASK_16M
	mtc0	t0, CP0_PAGEMASK
	li	t0, 0			# KMAP_INX
	mtc0	t0, CP0_INDEX
	li	t0, 1
	mtc0	t0, CP0_WIRED
	tlbwi
#else
	mtc0	zero, CP0_WIRED
#endif
	.endm

	/*
	 * For the moment disable interrupts, mark the kernel mode and
	 * set ST0_KX so that the CPU does not spit fire when using
	 * 64-bit addresses.  A full initialization of the CPU's status
	 * register is done later in per_cpu_trap_init().
	 */
	.macro	setup_c0_status set clr
	.set	push
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU0|\set|0x1f|\clr
	xor	t0, 0x1f|\clr
	mtc0	t0, CP0_STATUS
	.set	noreorder
	sll	zero,3				# ehb
	.set	pop
	.endm

	.macro	setup_c0_status_pri
#ifdef CONFIG_MIPS64
	setup_c0_status ST0_KX 0
#else
	setup_c0_status 0 0
#endif
	.endm

	.macro	setup_c0_status_sec
#ifdef CONFIG_MIPS64
	setup_c0_status ST0_KX ST0_BEV
#else
	setup_c0_status 0 ST0_BEV
#endif
	.endm

	/*
	 * Reserved space for exception handlers.
	 * Necessary for machines which link their kernels at KSEG0.
	 */
	.fill	0x400

EXPORT(stext)					# used for profiling
EXPORT(_stext)

// For PCI slave hosts, kernel_entry must coincide with load address,
// since it is now the boot vector.
#if !defined( CONFIG_QEMU ) && !defined (CONFIG_BRCM_PCI_SLAVE)

	__INIT
#endif




NESTED(kernel_entry, 16, sp)			# kernel entry point

#ifdef CONFIG_BRCM_PCI_SLAVE
xxx:
        li      a0, '7'
        li      t0, UART_ADR_BASE
        li      t2, TXDREGEMT
1:      lw      t1, UART_TXSTAT(t0)
        and     t1, t1, t2
        bne     t1, t2, 1b
        nop
        sw      a0, UART_TXDATA(t0)
 
        nop
        //b	xxx
        nop
        
	bal		pci_slave_init
	nop

return_from_pci_slave_init:

#else
	setup_c0_status_pri
#endif

#ifdef CONFIG_MIPS_BRCM97XXX
#ifdef CONFIG_SMP
	.extern  g_boot_config
	# Check to see which TP we are running on.
	mfc0    $8, $22, 3
	srl     $8, $8, 31
	beqz     $8, 1f
	nop
	
	# ---------------------------------	
	# We are in TP1
	# ---------------------------------	
InTp1:	
	# struct BootConfig {
	#     unsigned long signature;   //  0
	#     unsigned long ncpus;       //  4
	#     unsigned long func_addr;   //  8
	#     unsigned long sp;          // 12
	#     unsigned long gp;          // 16
	#     unsigned long arg;         // 20
	# }
	la       t0,	g_boot_config	# grab boot config data...
	lw       sp,	12(t0)
	lw       gp,	16(t0)
	lw       t1,	 8(t0)
	lw       a0,	20(t0)
	jr       t1
	nop
#endif // SMP


#ifndef CONFIG_BRCM_PCI_SLAVE
1:	jal	cfe_start
	nop
#endif

#if defined( CONFIG_MIPS_BCM7440B0) && defined(CONFIG_MIPS_BCM7440B_SECOND_CPU)
#define GLOBAL_REG_RBUS_START 0x10000000
#define MR24K_BRIDGE_REGS_1_REG_BRIDGE_CTL 0x1f8000c
        # Check to see which CPU we are running on.
        mfc0    t0, $15, 1
        andi    t0, 0x3ff
        srl     t0, t0, 1
        bnez     $8, 1f
        nop

        # ---------------------------------
        # We are in CPU 1 - Start CPU 2 and wait
        # ---------------------------------
        # Start second CPU
        la      a0, (GLOBAL_REG_RBUS_START+0xA0000000)
        lw      v1, MR24K_BRIDGE_REGS_1_REG_BRIDGE_CTL(a0)
        and     v1, 0xFFFFFFF3
        or      v1, 0x1
        sw      v1, MR24K_BRIDGE_REGS_1_REG_BRIDGE_CTL(a0)
        # wait forever
2:     wait
        b       2b
        nop

1:

#endif
#endif // BRCM97XXX

#ifdef CONFIG_SGI_IP27
	GET_NASID_ASM	t1
	move	t2, t1				# text and data are here
	MAPPED_KERNEL_SETUP_TLB
#endif /* IP27 */

	ARC64_TWIDDLE_PC

	PTR_LA		t0, __bss_start		# clear .bss
	LONG_S		zero, (t0)
	PTR_LA		t1, __bss_stop - LONGSIZE
1:
	PTR_ADDIU	t0, LONGSIZE
	LONG_S		zero, (t0)
	bne		t0, t1, 1b

	LONG_S		a0, fw_arg0		# firmware arguments
	LONG_S		a1, fw_arg1
	LONG_S		a2, fw_arg2
	LONG_S		a3, fw_arg3

	MTC0		zero, CP0_CONTEXT	# clear context register
	PTR_LA		$28, init_thread_union
	PTR_ADDIU	sp, $28, _THREAD_SIZE - 32
	set_saved_sp	sp, t0, t1
	PTR_SUBU	sp, 4 * SZREG		# init stack pointer

	j		start_kernel
	END(kernel_entry)

#if defined( CONFIG_QEMU ) || defined (CONFIG_BRCM_PCI_SLAVE)

	__INIT
#endif

#ifdef CONFIG_SMP
/*
 * SMP slave cpus entry point.  Board specific code for bootstrap calls this
 * function after setting up the stack and gp registers.
 */
NESTED(smp_bootstrap, 16, sp)
	setup_c0_status_sec

#ifdef CONFIG_SGI_IP27
	GET_NASID_ASM	t1
	dli	t0, KLDIR_OFFSET + (KLI_KERN_VARS * KLDIR_ENT_SIZE) + \
		    KLDIR_OFF_POINTER + CAC_BASE
	dsll	t1, NASID_SHFT
	or	t0, t0, t1
	ld	t0, 0(t0)			# t0 points to kern_vars struct
	lh	t1, KV_RO_NASID_OFFSET(t0)
	lh	t2, KV_RW_NASID_OFFSET(t0)
	MAPPED_KERNEL_SETUP_TLB
	ARC64_TWIDDLE_PC
#endif /* CONFIG_SGI_IP27 */

	j	start_secondary
	END(smp_bootstrap)
#endif /* CONFIG_SMP */

	__FINIT

	.comm	kernelsp,    NR_CPUS * 8, 8
	.comm	pgd_current, NR_CPUS * 8, 8

	.comm	fw_arg0, SZREG, SZREG		# firmware arguments
	.comm	fw_arg1, SZREG, SZREG
	.comm	fw_arg2, SZREG, SZREG
	.comm	fw_arg3, SZREG, SZREG

	.macro page name, order
	.comm	\name, (_PAGE_SIZE << \order), (_PAGE_SIZE << \order)
	.endm

	/*
	 * On 64-bit we've got three-level pagetables with a slightly
	 * different layout ...
	 */
	page	swapper_pg_dir, _PGD_ORDER
#ifdef CONFIG_MIPS64
	page	invalid_pmd_table, _PMD_ORDER
#endif
	page	invalid_pte_table, _PTE_ORDER


#ifdef CONFIG_BRCM_PCI_SLAVE
#include "../brcmstb/common/reset.S"
#include "../brcmstb/common/cacheALib.S"
#endif

