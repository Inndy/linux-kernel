/*
 *  drivers/char/brcmserial.h
 *
 *  Copyright (C) 2002 Steven J. Hill (shill@broadcom.com)
 */
#ifndef __BRCMSERIAL_H
#define __BRCMSERIAL_H

/*
 * UART base addresses
 */
#define BRCM_SERIAL1_BASE	0xb50007b0
#define BRCM_SERIAL2_BASE	0xb50007c0

/*
 * IRQ stuff
 */
#define BRCM_SERIAL1_IRQ	17
#define BRCM_SERIAL2_IRQ	18
#define UAIRQ			0x10
#define UBIRQ			0x08

/*
 * UPG Clock is determined by:
 *                              XTAL_FREQ * SYSPLL_FSEL / PB_SEL_VALUE
 *                                      = 24MHz * 9 / 8 = 27MHz
 */
#define BRCM_BASE_BAUD          1687500         /* (UPG Clock / 16) */

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

#endif /* __BRCMSERIAL_H */
