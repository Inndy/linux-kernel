/*
 * include/asm/brcm/serial.h
 *
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * UART defines for Broadcom eval boards
 *
 * 10-19-2001   SJH    Created 
 */

#ifndef __ASM_BRCM_SERIAL_H
#define __ASM_BRCM_SERIAL_H

#if defined(CONFIG_BCM93730)

/*
 * UART base addresses
 */
#define BRCM_SERIAL1_BASE	0xb50007b0
#define BRCM_SERIAL2_BASE	0xb50007c0
#define BRCM_SERIAL1_IO_OFFSET	0x020007b0
#define BRCM_SERIAL2_IO_OFFSET	0x020007c0

/*
 * IRQ stuff
 */
#define BRCM_SERIAL1_IRQ	17
#define BRCM_SERIAL2_IRQ	17
#define UAIRQ_BIT		4
#define UBIRQ_BIT		3
#define UAIRQ			0x10
#define UBIRQ			0x08

/*
 * UPG Clock is determined by:
 * 				XTAL_FREQ * SYSPLL_FSEL / PB_SEL_VALUE
 *					= 24MHz * 9 / 8 = 27MHz
 */
#define BRCM_BASE_BAUD		1687500		/* (UPG Clock / 16) */


#else

#if defined(CONFIG_BCM97115)
#include <asm/brcm/bcm97115/bcm97115.h>
#include <asm/brcm/bcm97115/bcmintrnum.h>
#elif defined(CONFIG_BCM97320)
#include <asm/brcm/bcm97320/bcm97320.h>
#include <asm/brcm/bcm97320/bcmintrnum.h>
#elif defined(CONFIG_BCM97110)
#error "BCM7110 not ready yet"
#elif defined(CONFIG_BCM97315)
#error "BCM7315 not ready yet"
#else
#error "unknown BCM STB chip!!!"
#endif /* if BCM97115, else error */


/*
 * UART base addresses
 */
#define BRCM_SERIAL1_BASE	UPG_UART_A_BASE
#define BRCM_SERIAL2_BASE	UPG_UART_B_BASE

/*
 * IRQ stuff
 */
#define BRCM_SERIAL1_IRQ	BCM_LINUX_UARTA_IRQ
#define BRCM_SERIAL2_IRQ	BCM_LINUX_UARTA_IRQ

/* bit defines in UPG int control reg. */
#define UAIRQ			UPG_UA_IRQ //0x10
#define UBIRQ			UPG_UB_IRQ //0x08

/*
 * UPG Clock is determined by:
 * 				XTAL_FREQ * SYSPLL_FSEL / PB_SEL_VALUE
 *					= 24MHz * 9 / 8 = 27MHz
 */
#define BRCM_BASE_BAUD		(XTALFREQ1/16) //1687500		/* (UPG Clock / 16) */


#endif /* if BCM93730 else */


/*
 * UART register offsets
 */
#define UART_RECV_STATUS	UARTA_RXSTAT //0x03	/* UART recv status register */
#define UART_RECV_DATA		UARTA_RXDATA //0x02	/* UART recv data register */
#define UART_CONTROL		UARTA_CONTROL //0x00	/* UART control register */
#define UART_BAUDRATE_HI	UARTA_BAUDHI //0x07	/* UART baudrate register */
#define UART_BAUDRATE_LO	UARTA_BAUDLO //0x06	/* UART baudrate register */
#define UART_XMIT_STATUS	UARTA_TXSTAT //0x05	/* UART xmit status register */
#define UART_XMIT_DATA		UARTA_TXDATA //0x04	/* UART xmit data register */

/*
 * UART control register definitions
 */
#define UART_PODD		PODD //1	/* odd parity */
#define UART_RE			RXEN //2	/* receiver enable */
#define UART_TE			TXEN //4	/* transmitter enable */
#define UART_PAREN		PAREN //8	/* parity enable */
#define UART_BIT8M		BITM8 //16	/* 8 bits character */

/*
 * Receiver status and control register definitions
 */
#define UART_RIE		RXINTEN //2	/* receiver interrupt enable */
#define UART_RDRF		RXDATARDY //4	/* receiver data register full flag */
#define UART_OVRN		OVERRUNERR //8	/* data overrun error */
#define UART_FE			FRAMEERR //16	/* framing error */
#define UART_PE			PARERR //32	/* parity error */

/*
 * Transmitter status and control register definitions
 */
#define UART_TDRE		TXDREGEMT //1	/* transmit data register empty flag */
#define UART_IDLE		IDLE //2	/* transmit in idle state   */
#define UART_TIE		TXINTEN //4	/* transmit interrupt enable */

#endif /* __ASM_BRCM_SERIAL_H */
