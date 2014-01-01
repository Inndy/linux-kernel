/*
 * arch/mips/brcm/irq.c
 *
 * Copyright (C) 2001 Broadcom Corporation
 *                    Steven J. Hill <shill@broadcom.com>
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
 *
 * Interrupt routines for Broadcom eval boards
 *
 * 10-23-2001   SJH    Created
 * 10-25-2001   SJH    Added comments on interrupt handling
 */
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>

#ifdef CONFIG_REMOTE_DEBUG
#include <asm/gdb-stub.h>
extern void breakpoint(void);
#endif

extern asmlinkage unsigned int do_IRQ(int irq, struct pt_regs *regs);
extern void brcm_led_update(unsigned long digit);

/*
 * Following is the complete map of interrupts in the system. Please
 * keep this up to date and make sure you comment your changes in the
 * comment block above with the date, your initials and what you did.
 *
 * There are two different interrupts in the system. The first type
 * is an actual hardware interrupt. We have a total of eight MIPS
 * interrupts. Two of them are software interrupts and are ignored.
 * The remaining six interrupts are actually monitored and handled
 * by low-level interrupt code in 'int-handler.S' that call dispatch
 * functions in this file to call the IRQ descriptors in the Linux
 * kernel.
 * 
 * The second type is the Linux kernel system interrupt which is
 * a table of IRQ descriptors (see 'include/linux/irq.h' and
 * 'include/linux/interrupt.h') that relate the hardware interrupt
 * handler types to the software IRQ descriptors. This is where all
 * of the status information and function pointers are defined so
 * that registration, releasing, disabling and enabling of interrupts
 * can be performed. Multiple system interrupts can be tied to a
 * single hardware interrupt. Examining this file along with the
 * other three aforementioned files should solidify the concepts.
 *
 * The first table simply shows the MIPS IRQ mapping:
 *
 *   MIPS IRQ   Source
 *   --------   ------
 *       0      Software *Ignored*
 *       1      Software *Ignored*
 *       2      Hardware *Unused*
 *       3      Hardware *Unused*
 *       4      Hardware (7020/7030 chip)
 *       5      Hardware (3250 chip)
 *       6      Hardware (Ali 1543C with i8259 PIC)
 *       7      R4k timer
 *
 * The second table shows the table of Linux system interrupt
 * descriptors and the mapping to the hardware IRQ sources:
 *
 *   System IRQ   MIPS IRQ   Source
 *   ----------   --------   ------
 *        0           6      *Unused*
 *        1           6      ALi keyboard controller
 *        2           6      Cascade for second half of i8259 PIC
 *        3           6      *Unused*
 *        4           6      *Unused*
 *        5           6      Input from POD
 *        6           6      Ethernet (AMD or Broadcom)
 *        7           6      ALi parallel port
 *        8           6      ALI realtime clock 
 *        9           6      Cascading i8259 PIC
 *       10           6      ALi keyboard data
 *       11           6      ALi USB-OHCI
 *       12           6      ALi PS/2 mouse
 *       13           6      *Unused*
 *       14           6      ide0 controller
 *       15           6      PCI Slot INTA / ide1 controller
 *       16           4      Broadcom 7020/7030 chip
 *       17           5      Broadcom 3250 chip (UART A)
 *       18           5      Broadcom 3250 chip (UART B)
 *       19           5      Broadcom 3250 chip (IR keyboard)
 *       20           5      Broadcom 3250 chip (keypad)
 *       21           5      Broadcom 3250 chip (IR blaster)
 *       60           7      R4k timer (used for master system time)
 *
 * Again, I cannot stress this enough, keep this table up to date!!!
 */


/*
 * IRQ7 functions
 */
void brcm_irq7_dispatch(struct pt_regs *regs)
{
	brcm_led_update(4);
	do_IRQ(60, regs);
}

static void brcm_irq7_enable(unsigned int irq)
{
	set_c0_status(STATUSF_IP7);
}

static void brcm_irq7_disable(unsigned int irq)
{
	clear_c0_status(STATUSF_IP7);
}

static void brcm_irq7_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_irq7_startup(unsigned int irq)
{ 
	brcm_irq7_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_irq7_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_irq7_enable(irq);
}

static struct hw_interrupt_type brcm_irq7_type = {
	typename: "MIPS",
	startup: brcm_irq7_startup,
	shutdown: brcm_irq7_disable,
	enable: brcm_irq7_enable,
	disable: brcm_irq7_disable,
	ack: brcm_irq7_ack,
	end: brcm_irq7_end,
	NULL
};


/*
 * IRQ6 functions
 */
void brcm_irq6_dispatch(struct pt_regs *regs)
{
	extern int i8259A_irq_pending(unsigned int irq);
	register int irq;

	brcm_led_update(2);


#if USE_ORIGINAL_CODES

	for (irq = 15 ; irq >= 0 ; irq--)
	{
		/*
		 * Okay, there are two different ways that this
		 * can work. The first method is to find the
		 * first pending interrupt, handle it and exit.
		 * The second way is to loop through all of the
		 * i8259 interrupts and handle all of them and
		 * then exit. Whatever works best for you, make
		 * sure you let others know. (P.S. to implement
		 * the second method, just remove the 'break;'
		 * and move the 'do_IRQ(irq, regs);' to replace
		 * it.)
		 */
		if(i8259A_irq_pending(irq))
		{
			break;
		}
	}

	do_IRQ(irq, regs);
#else
/*
 * THT: PR5279: Spurious interrupt on USB when mounting CAM
 * What happens is that the IRQ line was raised too fast when the
 * write op EOI ack is still posted.  The original codes would have
 * called do_IRQ on the same interrupt over and over again
 * causing the kernel to be stuck in an endless loop with 
 * Interrupt disabled
 * The fix is to call do_IRQ on all IRQ lines,
 * knowing that if the handler is INPROGRESS, we will skip it
 */
	for (irq = 15 ; irq >= 0 ; irq--)
	{
		/* THT: If Pending or INPROGRESS flag is on, skip it, this saves a call to
		  * i8259A_irq_pending, which is slow
		  */
		irq_desc_t *desc = irq_desc + irq;

		if (desc->status & (IRQ_DISABLED | IRQ_INPROGRESS))
			continue;

		 /* THT: End of Hack */
		 
		if(i8259A_irq_pending(irq))
		{
			//break;
			do_IRQ(irq, regs);
		}
	}

	
#endif
}



static void brcm_irq5_enable(unsigned int irq)
{
	set_c0_status(STATUSF_IP5);
}

static void brcm_irq5_disable(unsigned int irq)
{
	// In order to allow rmmod on drivers
	//clear_c0_status(STATUSF_IP5);
}

static void brcm_irq5_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_irq5_startup(unsigned int irq)
{ 
	brcm_irq5_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_irq5_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_irq5_enable(irq);
}

void brcm_irq5_dispatch(struct pt_regs *regs)
{
	brcm_led_update(3);
	brcm_irq5_disable(17);
	do_IRQ(17, regs);

	return;
}

static struct hw_interrupt_type brcm_irq5_type = {
	typename: "BCM3250",
	startup: brcm_irq5_startup,
	shutdown: brcm_irq5_disable,
	enable: brcm_irq5_enable,
	disable: brcm_irq5_disable,
	ack: brcm_irq5_ack,
	end: brcm_irq5_end,
	NULL
};


/*
 * IRQ4 functions
 */
static void brcm_irq4_enable(unsigned int irq)
{
	set_c0_status(STATUSF_IP4);
}

static void brcm_irq4_disable(unsigned int irq)
{
	clear_c0_status(STATUSF_IP4);
}

static void brcm_irq4_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_irq4_startup(unsigned int irq)
{ 
	brcm_irq4_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_irq4_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_irq4_enable(irq);
}

static struct hw_interrupt_type brcm_irq4_type = {
	typename: "BCM7030",
	startup: brcm_irq4_startup,
	shutdown: brcm_irq4_disable,
	enable: brcm_irq4_enable,
	disable: brcm_irq4_disable,
	ack: brcm_irq4_ack,
	end: brcm_irq4_end,
	NULL
};

void brcm_irq4_dispatch(struct pt_regs *regs)
{
	brcm_led_update(1);
	brcm_irq4_disable(16);
	do_IRQ(16, regs);
}


/*
 * Broadcom specific IRQ setup
 */
void __init brcm_irq_setup(void)
{
	extern asmlinkage void brcmIRQ(void);

uart_puts("set_except_vector\r\n");
	set_except_vector(0, brcmIRQ);

/* THT: Comment from Irvine Team: 
  * IMHO we do not need to do the following here, the request IRQ would take care of it.
  * //uart_puts("change_c0_status IRQ2,3,4,5\r\n");
  * //change_c0_status(ST0_IM, IE_IRQ2 | IE_IRQ3 | IE_IRQ4 | IE_IRQ5 );
  */
 	change_c0_status(ST0_IM, IE_IRQ4); /* Turn on ALI only */

uart_puts("Enable Interrupt\r\n");
	change_c0_status(ST0_IE, ST0_IE);


uart_puts("Setup timer interrupt \r\n");
	/* Setup timer interrupt */
	irq_desc[60].status = IRQ_DISABLED;
	irq_desc[60].action = 0;
	irq_desc[60].depth = 1;
	irq_desc[60].handler = &brcm_irq7_type;


	/*
	 * Setup mouse and keyboard interrupts. The
	 * interrupts 0 - 15 are handled by the i8259
	 * PIC driver in 'arch/mips/kernel/i8259.c'.
	 * If you want to understand this code, go
	 * read the ALi 1543C document and don't
	 * bother me.
	 */
uart_puts("init 8259A \r\n");
	outb(0x51, 0x03f0);
	outb(0x23, 0x03f0);
	outb(0x07, 0x03f0);
	outb(0x07, 0x03f1);
	outb(0x72, 0x03f0);
	outb(0x0c, 0x03f1);
	outb(0xbb, 0x03f0);


uart_puts("init 7030 IRQ\r\n");
	/* Setup 7030 interrupt */
	irq_desc[16].status = IRQ_DISABLED;
	irq_desc[16].action = 0;
	irq_desc[16].depth = 1;
	irq_desc[16].handler = &brcm_irq4_type;

uart_puts("init BCM3250 IRQ\r\n");
	/* Setup BCM3250 interrupts */
	irq_desc[17].status = IRQ_DISABLED;
	irq_desc[17].action = 0;
	irq_desc[17].depth = 1;
	irq_desc[17].handler = &brcm_irq5_type;

#ifdef CONFIG_REMOTE_DEBUG
	set_debug_traps();
	breakpoint();
#endif
uart_puts("<-- brcm_setup_irq\r\n");
}

void (*irq_setup)(void);

void __init init_IRQ(void)
{
	extern void __init init_i8259_irqs(void);
	extern void __init init_generic_irq(void);

uart_puts("init_generic_irq\r\n");
	init_generic_irq();
uart_puts("init_i8259_irqs\r\n");
	init_i8259_irqs();
uart_puts("irq_setup\r\n");
	irq_setup();
}
