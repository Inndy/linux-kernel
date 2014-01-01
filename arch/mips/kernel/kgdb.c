/*
 * arch/mips/kernel/kgdb.c
 *
 *  Originally written by Glenn Engel, Lake Stevens Instrument Division
 *
 *  Contributed by HP Systems
 *
 *  Modified for SPARC by Stu Grossman, Cygnus Support.
 *
 *  Modified for Linux/MIPS (and MIPS in general) by Andreas Busse
 *  Send complaints, suggestions etc. to <andy@waldorf-gmbh.de>
 *
 *  Copyright (C) 1995 Andreas Busse
 *
 *  Copyright (C) 2003 MontaVista Software Inc.
 *  Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 *  Copyright (C) 2004-2005 MontaVista Software Inc.
 *  Author: Manish Lachwani, mlachwani@mvista.com or manish@koffee-break.com
 *
 *  This file is licensed under the terms of the GNU General Public License
 *  version 2. This program is licensed "as is" without any warranty of any
 *  kind, whether express or implied.
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <asm/system.h>
#include <asm/ptrace.h>		/* for linux pt_regs struct */
#include <linux/kgdb.h>
#include <linux/init.h>
#include <asm/inst.h>
#include <asm/gdb-stub.h>
#include <asm/cacheflush.h>
#include <asm/kdebug.h>

#ifdef DEBUG_IT
#define PRINTK(args...) printk(KERN_ALERT args)
#else
#define PRINTK(args...)
#endif /* DEBUG_IT */



static unsigned int * stepped_address = 0;
static unsigned int   stepped_opcode  = 0;
static int stepped_cp0_status_ie = 0;

static unsigned int * mipsGetNpc
    (
    struct pt_regs * pRegs		/* pointer to task registers */
    );

static struct hard_trap_info {
	unsigned char tt;	/* Trap type code for MIPS R3xxx and R4xxx */
	unsigned char signo;	/* Signal that we map this trap into */
} hard_trap_info[] = {
	{ 6, SIGBUS },		/* instruction bus error */
	{ 7, SIGBUS },		/* data bus error */
	{ 9, SIGTRAP },		/* break */
#if 0  /* Do not intercept this.  Traps come from user mode. */
	{ 10, SIGILL },		/* reserved instruction */
#endif
/*	{ 11, SIGILL },	*/	/* CPU unusable */
	{ 12, SIGFPE },		/* overflow */
	{ 13, SIGTRAP },	/* trap */
	{ 14, SIGSEGV },	/* virtual instruction cache coherency */
	{ 15, SIGFPE },		/* floating point exception */
	{ 23, SIGSEGV },	/* watch */
	{ 31, SIGSEGV },	/* virtual data cache coherency */
	{ 0, 0}			/* Must be last */
};

/* Save the normal trap handlers for user-mode traps. */
void *saved_vectors[32];

struct kgdb_arch arch_kgdb_ops = {
#ifdef CONFIG_CPU_LITTLE_ENDIAN
	.gdb_bpt_instr = {0xd},
#else
	.gdb_bpt_instr = {0x00, 0x00, 0x00, 0x0d},
#endif
};

extern void trap_low(void);
extern void breakinst(void);
extern void init_IRQ(void);

void kgdb_call_nmi_hook(void *ignored)
{
	kgdb_nmihook(smp_processor_id(), (void *)0);
}

void kgdb_roundup_cpus(unsigned long flags)
{
	local_irq_restore(flags);
	smp_call_function(kgdb_call_nmi_hook, 0, 0, 0);
	local_irq_save(flags);
}

static int compute_signal(int tt)
{
	struct hard_trap_info *ht;

	for (ht = hard_trap_info; ht->tt && ht->signo; ht++)
		if (ht->tt == tt)
			return ht->signo;

	return SIGHUP;	/* default for things we don't know about */
}

/*
 * Set up exception handlers for tracing and breakpoints
 */
void handle_exception(struct pt_regs *regs)
{
    	int error;
	int trap = (regs->cp0_cause & 0x7c) >> 2;

	PRINTK("%s at 0x%lx (trap %d)\n",__FUNCTION__,
		regs->cp0_epc,trap);

	/* We fall into that case when attempting to access a bad memory area.
	 * A common reason is a bus error */

        if (atomic_read(&debugger_active) && kgdb_may_fault)
		{
		PRINTK("%s:recovering ! ...\n",__FUNCTION__);
		/* Restore our previous state. */
		kgdb_fault_longjmp(kgdb_fault_jmp_regs);
		/* Not reached. */    
		}
	
	if (atomic_read(&debugger_active))
		kgdb_nmihook(smp_processor_id(), regs);

	if (atomic_read (&kgdb_setting_breakpoint))
		if ((trap == 9) && (regs->cp0_epc == (unsigned long)breakinst))
			regs->cp0_epc += 4;

	if (stepped_opcode != 0) {

		PRINTK("Step done at 0x%x putting back 0x%x\n",
		       (int)stepped_address,(int) stepped_opcode);

		/* restores original instruction */
		
		if ((error = kgdb_set_mem((char *)stepped_address,(
				          unsigned char *) &stepped_opcode,
				          BREAK_INSTR_SIZE)) < 0)
			{
			PRINTK("Unable to restore original instruction\n");
			return;
			}

		flush_icache_range((long)stepped_address,
				   (long)stepped_address + 4);
		
		/* Restore original interrupts in cpsr regs */
		regs->cp0_status |= stepped_cp0_status_ie;

	}
	stepped_opcode = 0;
	
#ifdef DEBUG_IT	
	{
	unsigned long op;
	unsigned long addr = regs->cp0_epc;
	
	PRINTK("%s: It's very likely an actual BP.\n",__FUNCTION__);

	if (kgdb_get_mem((char *) addr,
			  (unsigned char *) &op,
			  BREAK_INSTR_SIZE) < 0)
		{
		PRINTK("Unable to read memory at 0x%x\n",addr);
		}
	else
		{
		PRINTK("op is 0x%x\n",(int) op);
		if (op != *(unsigned long*)arch_kgdb_ops.gdb_bpt_instr)
		    PRINTK("OOPS this is not a bp !\n");
		else
		    PRINTK("YES this is a bp !\n");    
	
		}
	}
#endif /* DEBUG_IT */	

	
	kgdb_handle_exception(0, compute_signal(trap), 0, regs);

	/* In SMP mode, __flush_cache_all does IPI */
	__flush_cache_all();
}

void set_debug_traps(void)
{
	struct hard_trap_info *ht;
	unsigned long flags;

	local_irq_save(flags);

	for (ht = hard_trap_info; ht->tt && ht->signo; ht++)
		saved_vectors[ht->tt] = set_except_vector(ht->tt, trap_low);

	local_irq_restore(flags);
}

#if 0
/* This should be called before we exit kgdb_handle_exception() I believe.
 * -- Tom
 */
void restore_debug_traps(void)
{
	struct hard_trap_info *ht;
	unsigned long flags;

	local_irq_save(flags);
	for (ht = hard_trap_info; ht->tt && ht->signo; ht++)
		set_except_vector(ht->tt, saved_vectors[ht->tt]);
	local_irq_restore(flags);
}
#endif

void regs_to_gdb_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
	int reg = 0;
	unsigned long *ptr = gdb_regs;

	for (reg = 0; reg < 32; reg++)
		*(ptr++) = regs->regs[reg];

	*(ptr++) = regs->cp0_status;
	*(ptr++) = regs->lo;
	*(ptr++) = regs->hi;
	*(ptr++) = regs->cp0_badvaddr;
	*(ptr++) = regs->cp0_cause;
	*(ptr++) = regs->cp0_epc;

	return;
}

void gdb_regs_to_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
	int reg = 0;
	unsigned long *ptr = gdb_regs;

	for (reg = 0; reg < 32; reg++)
		regs->regs[reg] = *(ptr++);

	regs->cp0_status = *(ptr++);
	regs->lo = *(ptr++);
	regs->hi = *(ptr++);
	regs->cp0_badvaddr = *(ptr++);
	regs->cp0_cause = *(ptr++);
	regs->cp0_epc = *(ptr++);

	return;
}

/*
 * Similar to regs_to_gdb_regs() except that process is sleeping and so
 * we may not be able to get all the info.
 */
void sleeping_thread_to_gdb_regs(unsigned long *gdb_regs, struct task_struct *p)
{
	int reg = 0;
	struct thread_info *ti = p->thread_info;
	unsigned long ksp = (unsigned long)ti + THREAD_SIZE -32;
	struct pt_regs *regs = (struct pt_regs *)ksp - 1;
	unsigned long *ptr = gdb_regs;

	for (reg = 0; reg < 16; reg++)
		*(ptr++) = regs->regs[reg];

	/* S0 - S7 */
	for (reg = 16; reg < 24; reg++)
		*(ptr++) = regs->regs[reg];

	for (reg = 24; reg < 28; reg++)
		*(ptr++) = 0;

	/* GP, SP, FP, RA */
	for (reg = 28; reg < 32; reg++)
		*(ptr++) = regs->regs[reg];

	*(ptr++) = regs->cp0_status;
	*(ptr++) = regs->lo;
	*(ptr++) = regs->hi;
	*(ptr++) = regs->cp0_badvaddr;
	*(ptr++) = regs->cp0_cause;
	*(ptr++) = regs->cp0_epc;

	return;
}

/*
 * Calls linux_debug_hook before the kernel dies. If KGDB is enabled,
 * then try to fall into the debugger
 */
static int kgdb_mips_notify(struct notifier_block *self, unsigned long cmd,
			void *ptr)
{
	struct die_args *args = (struct die_args *)ptr;
	struct pt_regs *regs = args->regs;
	int trap = (regs->cp0_cause & 0x7c) >> 2;

	/* See if KGDB is interested. */
	if (user_mode(regs))
		/* Userpace events, ignore. */
		return NOTIFY_DONE;

	kgdb_handle_exception(trap, compute_signal(trap), 0, regs);
	return NOTIFY_OK;
}

static struct notifier_block kgdb_notifier = {
	.notifier_call = kgdb_mips_notify,
};

/*
 * Handle the 's' and 'c' commands
 */
int kgdb_arch_handle_exception(int vector, int signo, int err_code,
				char *InBuffer, char *outBuffer,
				struct pt_regs *regs) {
	char *ptr;
	unsigned long address;
	int error;

	switch (InBuffer[0]) {
		case 's':
		    	{
			unsigned int * next_addr;

			PRINTK("KGDB: s command at 0x%x\n",(int)regs->cp0_epc);

			/* handle the optional parameter */
			ptr = &InBuffer[1];
			if (kgdb_hex2long (&ptr, &address))
				regs->cp0_epc = address;
			
			atomic_set(&cpu_doing_single_step,-1);

			kgdb_may_fault = 1;
			if ((kgdb_fault_setjmp(kgdb_fault_jmp_regs)) != 0) {
				kgdb_may_fault = 0;
			return -EINVAL;
			}
			
			next_addr = mipsGetNpc(regs);
			
			stepped_address = next_addr;

			PRINTK("KGDB next pc 0x%x\n", (int)next_addr);
			
			/* Saves original instruction */
			if ((error = kgdb_get_mem((char *)next_addr,
						  (unsigned char *) &stepped_opcode,
						  BREAK_INSTR_SIZE)) < 0)
			    	{
				PRINTK ("Unable to access opcode at next pc 0x%x\n", 
					(int)next_addr);

				return error;
				break;
				}
				
			/* Sets the temporary breakpoint */
			if ((error = kgdb_set_mem((char *)next_addr, 
					          arch_kgdb_ops.gdb_bpt_instr,
				                  BREAK_INSTR_SIZE)) < 0)
			    	{
				PRINTK("Unable to write tmp BP at next pc 0x%x\n", 
					(int)next_addr);
				    
				return error;
				break;
				}

			kgdb_may_fault = 0;

			stepped_cp0_status_ie = regs->cp0_status & ST0_IE;
			
			/* masks interrupts */
			
			regs->cp0_status &= ~ST0_IE;

			/* Flush cache */
			
			flush_icache_range((long)next_addr,
					   (long)next_addr + 4);
			if (kgdb_contthread)
				atomic_set(&cpu_doing_single_step,
					   smp_processor_id());

			PRINTK("step armed over 0x%x\n", (int)regs->cp0_epc);

			return 0;
			break;
			}
		case 'c':
			PRINTK("KGDB: c command at 0x%x\n",(int)regs->cp0_epc);
			
			/* handle the optional parameter */
			ptr = &InBuffer[1];
			if (kgdb_hex2long (&ptr, &address))
				regs->cp0_epc = address;

			atomic_set(&cpu_doing_single_step,-1);

			PRINTK("%s done OK\n",__FUNCTION__);
			
			return 0;
			break;
	}

	return -1;
}


/*
 * We use kgdb_early_setup so that functions we need to call now don't
 * cause trouble when called again later.
 */
int kgdb_arch_init(void)
{
	/* Board-specifics. */
	/* Force some calls to happen earlier. */
	if (kgdb_early_setup == 0) {
		trap_init();
		init_IRQ();
		kgdb_early_setup = 1;
	}

	/* Set our traps. */
	/* This needs to be done more finely grained again, paired in
	 * a before/after in kgdb_handle_exception(...) -- Tom */

	set_debug_traps();
	notifier_chain_register(&mips_die_chain, &kgdb_notifier);

	return 0;
}

/* Copyright (c) 1996-2001 Wind River Systems, Inc. */
/* <WRS_ADDED> */
unsigned int * mipsGetNpc
    (
    struct pt_regs * pRegs		/* pointer to task registers */
    )
    {
    int	rsVal;
    int	rtVal;
    int	disp;
    unsigned int  machInstr;
    unsigned long npc;
    unsigned long pc;
    
    if (pRegs == NULL)
	{
	panic ("%s: NULL pRegs !\n",__FUNCTION__);
	}
    
#if	FALSE
    /*
     * If we are in a branch delay slot, the pc has been changed in the
     * breakpoint handler to match with the breakpoint address.
     * It is modified to have its normal value.
     */

    if (pRegs->cp0_cause & CAUSE_BD)
	pRegs->cp0_epc--;
#endif	/* FALSE */

    pc        = pRegs->cp0_epc;
    machInstr = *(unsigned long *)pc;
    
    /* Default instruction is the next one. */

    npc = pc + 4;

    /*
     * Do not report the instruction in a branch delay slot as the
     * next pc.  Doing so will mess up the WDB_STEP_OVER case as
     * the branch instruction is re-executed.
     */

    /*
     * Check if we are on a branch likely instruction, which will nullify
     * the instruction in the slot if the branch is taken.
     * Also, pre-extract some of the instruction fields just to make coding
     * easier.
     */

    rsVal = pRegs->regs[(machInstr >> 21) & 0x1f];
    rtVal = pRegs->regs[(machInstr >> 16) & 0x1f];
    disp = ((int) ((machInstr & 0x0000ffff) << 16)) >> 14;
    if ((machInstr & 0xf3ff0000) == 0x41020000)	/* BCzFL  */
	{
	int copId = (machInstr >> 26) & 0x03;
	npc = pc + 8;
	switch (copId)
	    {
	  case 1:
#if 0	      
#ifndef SOFT_FLOAT
	    if ((pRegs->fpcsr & FP_COND) != FP_COND)
		npc = disp + pc + 4;
#endif	/* !SOFT_FLOAT */
#endif /* 0 */	    
	    break;
	    }
	}
    else if ((machInstr & 0xf3ff0000) == 0x41030000)	/* BCzTL  */
	{
	int copId = (machInstr >> 26) & 0x03;
	npc = pc + 8;
	switch (copId)
	    {
	  case 1:
#if 0	      
#ifndef SOFT_FLOAT
	    if ((pRegs->fpcsr & FP_COND) == FP_COND)
		npc = disp + pc + 4;
#endif	/* !SOFT_FLOAT */
#endif /* 0 */	    
	    break;
	    }
	}
    else if (((machInstr & 0xfc1f0000) == 0x04130000)		/* BGEZALL*/
	     || ((machInstr & 0xfc1f0000) == 0x04030000))	/* BGEZL  */
	{
	if (rsVal >= 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc1f0000) == 0x5c000000)	/* BGTZL  */
	{
	if (rsVal > 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc1f0000) == 0x58000000)	/* BLEZL  */
	{
	if (rsVal <= 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if (((machInstr & 0xfc1f0000) == 0x04120000)		/* BLTZALL*/
	     || ((machInstr & 0xfc1f0000) == 0x04020000))	/* BLTZL  */
	{
	if (rsVal < 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc000000) == 0x50000000)	/* BEQL   */
	{
	if (rsVal == rtVal)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc000000) == 0x54000000)	/* BNEL   */
	{
	if (rsVal != rtVal)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if (((machInstr & 0xfc000000) == 0x08000000)	 	/* J    */
	|| ((machInstr & 0xfc000000) == 0x0c000000)) 	/* JAL  */
	npc = ((machInstr & 0x03ffffff) << 2) | 
	       (pc        & 0xf0000000);
    else if (((machInstr & 0xfc1f07ff) == 0x00000009)	/* JALR */
	     || ((machInstr & 0xfc1fffff) == 0x00000008))	/* JR   */
	npc = pRegs->regs[(machInstr >> 21) & 0x1f];
    else if ((machInstr & 0xf3ff0000) == 0x41000000)	/* BCzF   */
	{
	int copId = (machInstr >> 26) & 0x03;
	npc = pc + 8;
	switch (copId)
	    {
	  case 1:
#if 0	      
#ifndef SOFT_FLOAT
	    if ((pRegs->fpcsr & FP_COND) != FP_COND)
		npc = disp + pc + 4;
#endif	/* !SOFT_FLOAT */
#endif /* 0 */	    
	    break;
	    }
	}
    else if ((machInstr & 0xf3ff0000) == 0x41010000)	/* BCzT   */
	{
	int copId = (machInstr >> 26) & 0x03;
	npc = pc + 8;
	switch (copId)
	    {
	  case 1:
#if 0	      	      
#ifndef SOFT_FLOAT
	    if ((pRegs->fpcsr & FP_COND) == FP_COND)
		npc = disp + pc + 4;
#endif	/* !SOFT_FLOAT */
#endif /* 0 */
	    break;
	    }
	}
    else if ((machInstr & 0xfc000000) == 0x10000000)	/* BEQ    */
	{
	if (rsVal == rtVal)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if (((machInstr & 0xfc1f0000) == 0x04010000)	/* BGEZ   */
	     || ((machInstr & 0xfc1f0000) == 0x04110000))	/* BGEZAL */
	{
	if (rsVal >= 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc1f0000) == 0x1c000000)	/* BGTZ   */
	{
	if (rsVal > 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc1f0000) == 0x18000000)	/* BLEZ   */
	{
	if (rsVal <= 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if (((machInstr & 0xfc1f0000) == 0x04000000)	/* BLTZ   */
	     || ((machInstr & 0xfc1f0000) == 0x04100000))	/* BLTZAL */
	{
	if (rsVal < 0)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else if ((machInstr & 0xfc000000) == 0x14000000)	/* BNE    */
	{
	if (rsVal != rtVal)
	    npc = disp + pc + 4;
	else
	    npc = pc + 8;
	}
    else
	{
	/* normal instruction */
	}
    
    return (unsigned int *) npc;
    }
/* </WRS_ADDED> */
