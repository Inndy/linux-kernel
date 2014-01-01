/*
 * arch/mips/brcm-boards/bcm93730/setup.c
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
 * Setup for Broadcom eval boards
 *
 * 10-23-2001   SJH    Created
 */
#include <linux/config.h>
#include <asm/cpu.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/addrspace.h>
#include <asm/irq.h>
#include <asm/reboot.h>
#include <asm/time.h>


#ifdef CONFIG_BLK_DEV_IDE
#include <linux/ide.h>
extern struct ide_ops std_ide_ops;
#endif
#ifdef CONFIG_PC_KEYB
#include <asm/keyboard.h>
extern struct kbd_ops brcm_kbd_ops;
#endif

static void brcm_machine_restart(char *command)
{
	static void (*back_to_prom)(void) = (void (*)(void)) 0xbfc00000;

	/* Reboot */
	back_to_prom();
}

static void brcm_machine_halt(void)
{
	printk("Broadcom eval board halted.\n");
	while (1);
}

static void brcm_machine_power_off(void)
{
	printk("Broadcom eval board halted. Please turn off power.\n");
	while (1);
}

#ifndef PRID_IMP_R5510
#define PRID_IMP_R5510		0x5500
#endif

static __init void brcm_time_init(void)
{
	extern unsigned int mips_hpt_frequency;
	unsigned int cpuId, majorId;
	void uart_puts(const char*);

	/* Set the counter frequency, according to supported CPU types */
	cpuId = read_c0_prid();
	switch (majorId = cpuId & 0xff00) {
	case PRID_IMP_R5432: /* NEC 5432 */
		printk("BCM937XX with NEC 5432 CPU\n");
		uart_puts("BCM937XX with NEC 5432 CPU\n");
		mips_hpt_frequency = 81000000;  /* NEC_CPU_CLOCK/2; */
		break;
	case PRID_IMP_NEVADA: /* QED processors */
		printk("BCM937XX with QED CPU\n");
		uart_puts("BCM937XX with QED CPU\n");
		mips_hpt_frequency = (2*56000000)/2;
		break;
	case PRID_IMP_R5510: /* NEC VR5510 */
		printk("BCM937XX with NEC VR5510 CPU\n");
		uart_puts("BCM937XX with NEC VR5510 CPU 200MHz\n");
		mips_hpt_frequency = 400000000/2;
		break;
	default:
		/* Set me */
printk( "Unrecognized CPU: %0x, majorId=%0x, ComparedValue=%0x.\n", cpuId, majorId, PRID_IMP_R5510);
printk("***************Set CPU speed in arch/mips/brcm/setup.c\n");
{
char msg[133];	
sprintf(msg,  "Unrecognized CPU: %0x, majorId=%0x, ComparedValue=%0x.\n", cpuId, majorId, PRID_IMP_R5510);
uart_puts(msg);
}
		mips_hpt_frequency = 81000000;
		break;
	}

}

static __init void brcm_timer_setup(struct irqaction *irq)
{
	unsigned int count;

	/* Connect the timer interrupt */
	irq->dev_id = (void *) irq;
	setup_irq(60, irq);

	/* Generate first timer interrupt */
	count = read_c0_count();
	write_c0_compare(count + 1000);
}

void __init bus_error_init(void) { /* nothing */ }

void __init brcm_setup(void)
{
	extern void brcm_irq_setup(void);
	extern int panic_timeout;
	extern const unsigned long mips_io_port_base;

	irq_setup = brcm_irq_setup;
	//mips_io_port_base = KSEG1ADDR(0x13000000);

	_machine_restart = brcm_machine_restart;
	_machine_halt = brcm_machine_halt;
	_machine_power_off = brcm_machine_power_off;

	board_time_init = brcm_time_init;
	board_timer_setup = brcm_timer_setup;
	panic_timeout = 180;

#ifdef CONFIG_BLK_DEV_IDE
	ide_ops = &std_ide_ops;
#endif
#ifdef CONFIG_PC_KEYB
	kbd_ops = &brcm_kbd_ops;
#endif
#ifdef CONFIG_VT
	conswitchp = &dummy_con;
#endif
}
