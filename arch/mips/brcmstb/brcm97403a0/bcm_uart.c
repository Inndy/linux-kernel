/*---------------------------------------------------------------------------

    Copyright (c) 2001-2006 Broadcom Corporation                     /\
                                                              _     /  \     _
    _____________________________________________________/ \   /    \   / \_
                                                            \_/      \_/  
    
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    File: bcm_uart.c

    Description: 
    Simple UART driver for 71xx (UARTA & B)
    Simple UART driver for 7400 16550V2 style UART (UARTC)

    when	who what
-----	---	----
051011	tht	Original coding
 ------------------------------------------------------------------------- */

#define DFLT_BAUDRATE   115200

/*
 * On the 7401C0, UARTB is the default, with UARTC being the 2nd serial port.  UARTA is not used,
 * so we have the following mapping
 *
 *  Hardware			Linux
 *  UARTA				Not used
 *  UARTB				ttyS0
 * 	UARTC				ttyS1  - 16550A style UART 
 *
 ***ADT**
 * Starting with 7403A0 UARTA is install as ttyS1, UARTB as ttyS0, and 
 * UARTC as ttyS2
 *
 */

typedef struct {
	unsigned long/*char*/	uRxStatus;
	unsigned long/*char*/	uRxData;
	unsigned long/*char*/	UNUSED;
	unsigned long/*char*/	uControl;
	unsigned long/*char*/	uBaudRateHi;
	unsigned long/*char*/	uBaudRateLo;
	unsigned long/*char*/	uTxStatus;
	unsigned long/*char*/	uTxData;
} 	Uart7401;

#if 0
#define UART7401_UARTB_BASE		0xb04001a0  
#define stUart ((volatile Uart7401 * const) UART7401_UARTB_BASE)
#endif

static unsigned long uart_base[2] = {0xB04001A0, 0xB0400180};
static volatile Uart7401 *stUart;
static volatile Uart7401 *conUart;

#define DFLT_BAUDRATE   115200

/* 
 * UART IP pin assignments from the 7403A0 RDB SUN_TOP_CTRL_PIN_MUX_CTRL.
 *  bits field is 3 bits wide
 *
 *      UART    GPIO    MUX     bits    value
 *      ______________________________________
 *      0 tx    49      11      02:00   1
 *      0 rx    50      11      05:03   1
 *      ______________________________________
 *      1 tx     9       7      02:00   1
 *      1 rx    10       7      05:03   1
 *      ______________________________________
 *      2 tx    42      10      11:09   1
 *      2 rx    39      10      02:00   1
 *      2 rtsb  41      10      08:06   3
 *      2 ctsb  40      10      05:03   3
 *      ______________________________________
 *
 */

struct uart_pin_assignment {
            int tx_mux;
            int tx_pos;
            int rx_mux;
            int rx_pos;
    };

struct uart_pin_assignment brcm_7403A0_uart_pins[3] = {
        { 11,  0, 11,  3},
        {  7,  0,  7,  3},
        { 10,  9, 10,  0}
    } ;

struct ctsrts_pin_assignment {
            int cts_mux;
            int cts_pos;
            int rts_mux;
            int rts_pos;
    };

struct ctsrts_pin_assignment brcm_7403A0_ctsrts_pins[3] = {
        {  0,  0,  0,  0},
        {  0,  0,  0,  0},
        { 10,  3, 10,  6}
    } ;


#define SUN_TOP_CTRL_PIN_MUX_CTRL_0     (0xb0404094)

extern int console_uart;
extern int uart_initialized;

void
serial_set_pin_mux(int chan)
{
        int mux, mask;
        struct uart_pin_assignment *pins;
	struct ctsrts_pin_assignment *ctsrts;
        volatile unsigned long *MuxCtrl;

        /* Pin mux control registers 0-10 are contiguous */
         MuxCtrl = (volatile unsigned long*) SUN_TOP_CTRL_PIN_MUX_CTRL_0;

        if (chan < 0 || chan > 2)
                return;

        pins = &brcm_7403A0_uart_pins[chan];

        /* tx */
        mux  = pins->tx_mux;
        mask = 0x07 <<  pins->tx_pos;
        MuxCtrl[mux] &= ~mask;
        MuxCtrl[mux] |= 1 << pins->tx_pos;

        /* rx */
        mux  = pins->rx_mux;
        mask = 0x07 <<  pins->rx_pos;
        MuxCtrl[mux] &= ~mask;
        MuxCtrl[mux] |= 1 << pins->rx_pos;

	ctsrts = &brcm_7403A0_ctsrts_pins[chan];

	/* cts */
	if (ctsrts->cts_mux != 0)
	{
        	mux  = ctsrts->cts_mux;
        	mask = 0x07 <<  ctsrts->cts_pos;
        	MuxCtrl[mux] &= ~mask;
        	MuxCtrl[mux] |= 3 << ctsrts->cts_pos;
	}


	/* rts */
        if (ctsrts->rts_mux != 0)
        {
                mux  = ctsrts->rts_mux;
                mask = 0x07 <<  ctsrts->rts_pos;
                MuxCtrl[mux] &= ~mask;
                MuxCtrl[mux] |= 3 << ctsrts->rts_pos;
        }

}



/* --------------------------------------------------------------------------
    Name: PutChar
 Purpose: Send a character to the UART
-------------------------------------------------------------------------- */
void 
//PutChar(char c)
uartA_putc(char c)
{

#if 0
    // Wait for Tx Data Register Empty
    while (! (*((volatile unsigned long *)&(conUart->uTxStatus)) & 0x1));

    *((volatile unsigned long *)&(conUart->uTxData)) = c;
#else
	while (!(*((volatile unsigned long*) 0xb0400198) & 1));

	*((volatile unsigned long*) 0xb040019c) = c;
#endif

}

/* --------------------------------------------------------------------------
    Name: PutString
 Purpose: Send a string to the UART
-------------------------------------------------------------------------- */
void 
//PutString(const char *s)
uartA_puts(const char *s)
{
#ifdef CONFIG_BRCM_UART_PRINT
    	while (*s) {
        	if (*s == '\n') {
            		uartA_putc('\r');
        	}
    		uartA_putc(*s++);
    	}
#endif
}


/* --------------------------------------------------------------------------
    Name: PutChar
 Purpose: Send a character to the UART
-------------------------------------------------------------------------- */
void
//PutChar(char c)
uartB_putc(char c)
{

        while (!(*((volatile unsigned long*) 0xb04001b8) & 1));

        *((volatile unsigned long*) 0xb04001bc) = c;

}

/* --------------------------------------------------------------------------
    Name: PutString
 Purpose: Send a string to the UART
-------------------------------------------------------------------------- */
void
//PutString(const char *s)
uartB_puts(const char *s)
{
 #ifdef CONFIG_BRCM_UART_PRINT
       while (*s) {
                if (*s == '\n') {
                        uartB_putc('\r');
                }
                uartB_putc(*s++);
        }
#endif
}

/* --------------------------------------------------------------------------
    Name: PutChar
 Purpose: Send a character to the UART
-------------------------------------------------------------------------- */
void 
//PutChar(char c)
uart_putc(char c)
{

    // Wait for Tx Data Register Empty
    while (! (*((volatile unsigned long *)&(conUart->uTxStatus)) & 0x1));

    *((volatile unsigned long *)&(conUart->uTxData)) = c;

}

/* --------------------------------------------------------------------------
    Name: PutString
 Purpose: Send a string to the UART
-------------------------------------------------------------------------- */
void 
//PutString(const char *s)
uart_puts(const char *s)
{
#ifdef CONFIG_BRCM_UART_PRINT
	if (uart_initialized)
	{
    		while (*s) {
        		if (*s == '\n') {
            			uart_putc('\r');
        		}
    			uart_putc(*s++);
    		}
	}
#endif
}


/* --------------------------------------------------------------------------
    Name: GetChar
 Purpose: Get a character from the UART. Non-blocking
-------------------------------------------------------------------------- */
char 
uart_getc(void)
{
    	char cData = 0;
	volatile unsigned long uStatus = 
	    *((volatile unsigned long *)&(conUart->uRxStatus));

    	if (uStatus & 0x4) {
        	cData = *((volatile unsigned long *)&(conUart->uRxData));

		// Check for Frame & Parity errors
		if (uStatus & (0x10 | 0x20)) {
           	 cData = 0;
        	}
    	}
	return cData;
}



/* --------------------------------------------------------------------------
    Name: bcm71xx_uart_init
 Purpose: Initalize the UARTB abd UARTC
 (Linux knows them as UARTA and UARTB respectively)
-------------------------------------------------------------------------- */
void 
//bcm71xx_uart_init(uint32 uClock)
serial_bcm_init(unsigned long uartport, unsigned long uClock)
{
	unsigned long uBaudRate;
        char msg[40];


	// set the mux
	serial_set_pin_mux(uartport);

	conUart = (volatile Uart7401 * const)uart_base[console_uart];

	stUart  = (volatile Uart7401 * const)uart_base[uartport];
	// Make sure clock is ticking
	//INTC->blkEnables = INTC->blkEnables | UART_CLK_EN;

	// Calculate BaudRate register value => PeriphClk / UartBaud / 16
	uBaudRate = uClock / (DFLT_BAUDRATE * 16);
	//uBaudRate++;

	// Set the BAUD rate
	stUart->uBaudRateLo = (uBaudRate & 0xFF);
	stUart->uBaudRateHi = ((uBaudRate >> 8) & 0xFF);
//	stUartB->uBaudRateLo = (uBaudRate & 0xFF);
//	stUartB->uBaudRateHi = ((uBaudRate >> 8) & 0xFF);

	// Enable the UART, 8N1, Tx & Rx enabled
	stUart->uControl = 0x16;
//	stUartB->uControl = 0x16;
	sprintf(msg, "Done initializing U%d\n", uartport);
	if (uartport == 0)
	{
		uartA_puts(msg);	
	}
	else
	{
		uartB_puts(msg);
	}
}
