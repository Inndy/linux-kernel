/*
 * arch/mips/brcm/dbg_io.c
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
 * Remote debugging routines for Broadcom eval boards
 *
 * 10-29-2001   SJH    Created
 * 02-06-2002   SJH    Brought all macros into file
 */
#include <asm/io.h>

/*
 * UART base register offsets
 */
#define BRCM_SERIAL1_IO_OFFSET  0x020007b0
#define BRCM_SERIAL2_IO_OFFSET  0x020007c0
#define	UART_BASE		BRCM_SERIAL2_IO_OFFSET

/*
 * UART register offsets
 */
#define UART_RECV_STATUS	0x03	/* UART recv status register */
#define UART_RECV_DATA		0x02	/* UART recv data register */
#define UART_CONTROL		0x00	/* UART control register */
#define UART_BAUDRATE_HI	0x07	/* UART baudrate register */
#define UART_BAUDRATE_LO	0x06	/* UART baudrate register */
#define UART_XMIT_STATUS	0x05	/* UART xmit status register */
#define UART_XMIT_DATA		0x04	/* UART xmit data register */

/*
 * UART control register definitions
 */
#define UART_PODD		1	/* odd parity */
#define UART_RE			2	/* receiver enable */
#define UART_TE			4	/* transmitter enable */
#define UART_PAREN		8	/* parity enable */
#define UART_BIT8M		16	/* 8 bits character */

/*
 * Receiver status and control register definitions
 */
#define UART_RIE		2	/* receiver interrupt enable */
#define UART_RDRF		4	/* receiver data register full flag */
#define UART_OVRN		8	/* data overrun error */
#define UART_FE			16	/* framing error */
#define UART_PE			32	/* parity error */

/*
 * Transmitter status and control register definitions
 */
#define UART_TDRE		1	/* transmit data register empty flag */
#define UART_IDLE		2	/* transmit in idle state   */
#define UART_TIE		4	/* transmit interrupt enable */


char getDebugChar(void)
{
	while(!(inb(UART_BASE + UART_RECV_STATUS) & UART_RDRF));
	return inb(UART_BASE + UART_RECV_DATA);
}

int putDebugChar(char c)
{
	while((inb(UART_BASE + UART_XMIT_STATUS) & UART_TDRE) != UART_TDRE);
	outb(c, (UART_BASE + UART_XMIT_DATA));
	return 1;
}
