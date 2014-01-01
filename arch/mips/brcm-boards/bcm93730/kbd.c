/*
 * arch/mips/brcm/kbd.c
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
 * Keyboard driver for Broadcom eval boards
 *
 * 10-19-2001   SJH    Created
 */
#include <linux/sched.h>
#include <linux/pc_keyb.h>
#include <asm/io.h>
#include <asm/keyboard.h>

#define BRCM_KEYBOARD_IRQ	1
#define BRCM_MOUSE_IRQ		12
#define KEYBOARD_DATA		0x0060		/* Data buffer */
#define KEYBOARD_CMD_STAT	0x0064		/* Command and status */

static void brcm_request_region(void)
{
	/* No I/O ports are being used on Broadcom board. */
}

static int brcm_request_irq(void (*handler)(int, void *, struct pt_regs *))
{
	return request_irq(BRCM_KEYBOARD_IRQ, handler, 0, "keyboard",
		(void *) &handler);
}                                                                                                        
static int brcm_aux_request_irq(void (*handler)(int, void *, struct pt_regs *))
{
	return request_irq(BRCM_MOUSE_IRQ, handler, SA_SHIRQ, "mouse",
		(void *) &handler);
}
 
static void brcm_aux_free_irq(void)
{
	free_irq(BRCM_MOUSE_IRQ, NULL);
}
 
static unsigned char brcm_read_input(void)
{
	return inb(KEYBOARD_DATA);
}
 
static void brcm_write_output(unsigned char val)
{
	while(inb(KEYBOARD_CMD_STAT) & 0x02);
	outb(val, KEYBOARD_DATA);
}
 
static void brcm_write_command(unsigned char val)
{
	while(inb(KEYBOARD_CMD_STAT) & 0x02);
	outb(val, KEYBOARD_CMD_STAT);
}
 
static unsigned char brcm_read_status(void)
{
	return inb(KEYBOARD_CMD_STAT);
}                                                                                                        
struct kbd_ops brcm_kbd_ops = {
	brcm_request_region,
	brcm_request_irq,

	brcm_aux_request_irq,
	brcm_aux_free_irq,

	brcm_read_input,
	brcm_write_output,
	brcm_write_command,
	brcm_read_status
};
