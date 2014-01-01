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
 * Early serial output driver
 */
#include "asm/brcm/bcm93730/bcmupg.h"
#include "asm/serial.h"
#include "asm/brcm/bcm93730/bcm93730.h"
#define UART_DBG_BASE (BCM_3250_UPG+UARTB_BASE)

void
uart_putc(char c)
{
	char* base = (char*) UART_DBG_BASE;
	int i;

	for (i=0; i < 50000; i++) {
		if (base[UART_XMIT_STATUS] & UART_TDRE)
			break;
	}
	base[UART_XMIT_DATA] = c;
	return;
}

void
uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
    	uart_putc(*s++);
    }
}


void 
//bcm71xx_uart_init(uint32 uClock)
uart_init(unsigned long uClock)
{
    unsigned long uBaudRate;
    unsigned char* base = (char*) UART_DBG_BASE;

    return;
    
	// Calculate BaudRate register value => PeriphClk / UartBaud / 16
    uBaudRate = uClock / (BASE_BAUD * 16);
	//uBaudRate++;
	base[UART_CONTROL] = 0x17;

	// Set the BAUD rate
	base[UART_BAUDRATE_LO] = (uBaudRate & 0xFF);
	base[UART_BAUDRATE_HI] = ((uBaudRate >> 8) & 0xFF);
	/*stUartB->uBaudRateLo = (uBaudRate & 0xFF);
	stUartB->uBaudRateHi = ((uBaudRate >> 8) & 0xFF); */

	// Enable the UART, 8N1, Tx & Rx enabled
	//stUart->uControl = 0x16;
	//base[UART_CONTROL] = 0x16;
}
