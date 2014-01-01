#ifndef _UPG_H
#define _UPG_H


/****************************************************************
* File         : bcmupg.h
* Function     : This file defines the registers for the BCM3250
*                UPG.
* Author       : Dat Lam
****************************************************************/

#define UPG_UNKNOWN     -1

/* UPG Registers                                */
#define BLAST_NUMSEQ            0x00000000      /* Blaster number sequence      */
#define BLAST_CONTROL           0x00000001      /* Blaster control              */
#define BLAST_INDXPRE           0x00000002
#define BLAST_PRIMPRE           0x00000003
#define BLAST_REPINDX           0x00000004
#define BLAST_REPNUM            0x00000005
#define BLAST_CARRHI            0x00000006
#define BLAST_CARRLO            0x00000007
#define BLAST_SEQ               0x0x000008
#define BLAST_MOD               0x00000030

#define INT_ENABLE              PCI_ADDR8(0x0000004c)       /* UPG Interrupt enable         */
#define INT_STATUS              PCI_ADDR8(0x0000004e)       /* UPG Interrupt status         */

#define KEYPAD_DATA             PCI_ADDR8(0x00000090)
#define LED_KEYROW32            PCI_ADDR8(0x00000090)       /* keypad row 3 & 2             */
#define LED_KEYROW10            PCI_ADDR8(0x00000091)       /* keypad row 1 & 0             */
#define LED_PRESCALAR           PCI_ADDR8(0x00000092)       /* Prescalar                    */
#define LED_PRESCHI             PCI_ADDR8(0x00000092)       /* Prescalar HI                 */
#define LED_PRESCLO             PCI_ADDR8(0x00000093)       /* Prescalar LO                 */
#define LED_DUTY_CYCLE          PCI_ADDR8(0x00000094)       /* LED duty cycle               */
#define LED_DUTYOFF             PCI_ADDR8(0x00000094)       /* LED duty cycle off           */
#define LED_DUTYON              PCI_ADDR8(0x00000095)       /* LED duty cycle on            */
#define LED_DIGIT2              PCI_ADDR8(0x00000096)       /* LED Digit 2                  */
#define LED_DIGIT1              PCI_ADDR8(0x00000097)       /* LED Digit 1                  */
#define LED_DIGIT4              PCI_ADDR8(0x00000098)       /* LED Digit 4                  */
#define LED_DIGIT3              PCI_ADDR8(0x00000099)       /* LED Digit 3                  */
#define LED_DEBOUNCE            PCI_ADDR8(0x0000009A)       /* LED Terminal Debounce Count  */
#define LED_STATUS              PCI_ADDR8(0x0000009B)       /* LED Status output            */
#define LED_IRQEN               PCI_ADDR8(0x0000009D)       /* LED & Keypad IRQ enable      */

#define KBD_STATUS              PCI_ADDR8(0x00000080)       /* IR Status                    */
#define KBD_DATA_BYTE3          PCI_ADDR8(0x00000082)       /* IR data byte 3               */
#define KBD_DATA_BYTE2          PCI_ADDR8(0x00000083)       /* IR data byte 2               */
#define KBD_DATA_BYTE1          PCI_ADDR8(0x00000084)       /* IR data byte 1               */
#define KBD_DATA_BYTE0          PCI_ADDR8(0x00000085)       /* IR data byte 0               */
#define KBD_COMMAND             PCI_ADDR8(0x00000086)       /* IR command                   */

#define UARTA_BASE              0x000000B0                      /* base address of UARTA        */
#define UARTB_BASE              0x000000C0                      /* base address of UARTB        */
#define BCM3250_RFTUNER         0x000000D0                      /* base address of RF Tuner     */

#define UART_RECV_STATUS        PCI_ADDR8(0)           /* UART recv status register unsigned char      */
#define UART_RECV_DATA          PCI_ADDR8(1)           /* UART recv data register unsigned char        */
#define UART_CONTROL            PCI_ADDR8(3)           /* UART control register unsigned short         */
#define UART_BAUDRATE_HI        PCI_ADDR8(4)           /* UART baudrate register unsigned short        */
#define UART_BAUDRATE_LO        PCI_ADDR8(5)           /* UART baudrate register unsigned short        */
#define UART_XMIT_STATUS        PCI_ADDR8(6)           /* UART xmit status register unsigned char      */
#define UART_XMIT_DATA          PCI_ADDR8(7)            /* UART xmit data register unsigned char        */

              /* UART control register definitions */
#define UART_PODD               1  	/* odd parity      */
#define UART_RE                 2	/* receiver enable */
#define UART_TE                 4   /* transmitter enable */
#define UART_PAREN              8   /* parity enable  */
#define UART_BIT8M              16  /* 8 bits character */

             /* receiver status and control register definitions */
#define UART_RIE                2   /* receiver interrupt enable */
#define UART_RDRF               4    /* receiver data register full flag */
#define UART_OVRN               8    /* data overrun error */
#define UART_FE                 16   /* framing error */
#define UART_PE                 32   /* parity error   */

              /* transmitter status and control register definitions */
#define UART_TDRE               1     /* transmit data register empty flag */
#define UART_IDLE               2     /* transmit in idle state   */
#define UART_TIE                4     /* transmit interrupt enable */


/* SDRAM Controller Registers           */
#define SDRAM_INIT              0x00000000      /* SDRAM Controller Init */
#define SDRAM_CONFIG            0x00000004      /* SDRAM Configuration */
#define SDRAM_RFSH              0x00000008      /* SDRAM Refresh Control */
#define SDRAM_BASE              0x0000000C      /* SDRAM Base        */


/****************************************************************************
* C Structures
****************************************************************************/

/* Definitions in LED_SWR register */
#define LED_RESET           1   /* software reset in LED_SWR register */
#define KEYPAD_IRQM         2   /* mask for keypad interrupt request */
                                /* 1 = no mask, 0 = mask */
   
#ifndef LED_XXX
#define LED_XXX	/* avoid duplicate symbols */

/* LED segment definitions.     */
#define LED_ON		0x80
#define LED_TOP_OFF	0x01
#define LED_RU_OFF  0x02 
#define LED_RL_OFF	0x04
#define LED_BOT_OFF	0x08
#define LED_LU_OFF	0x10
#define LED_LL_OFF	0x20
#define LED_MID_OFF	0x40

#define LED_A		(0x88)
#define LED_B		(0x83)
#define LED_C       (0xc6)
#define LED_D		(0xa1)
#define LED_E		(0x86)
#define LED_F		(0x8e)
#define LED_G		(0x82)
#define LED_H		(0x89)
#define LED_I		(0xf9)
#define LED_J		(0xe1)
#define LED_K		(0x89)
#define LED_L		(0xc7)
#define LED_M		(0xc9)
#define LED_N		(0xcb)
#define LED_O		(0xc0)
#define LED_Q		(0xc0)
#define LED_P		(0x8c)
#define LED_R		(0x88)
#define LED_S		(0x92)
#define LED_T		(0xf8)
#define LED_U		(0x81)
#define LED_W		(0x81)
#define LED_V		(0x81)
#define LED_X		(0x89)
#define LED_Y		(0x99)
#define LED_Z		(0xa4)

#define LED_DASH	(0xbf)

#define LED_NUM0	(0xc0)
#define LED_NUM1	(0xf9)
#define LED_NUM2	(0xa4)
#define LED_NUM3	(0xb0)
#define LED_NUM4	(0x99)
#define LED_NUM5	(0x92)
#define LED_NUM6	(0x82)
#define LED_NUM7	(0xf8)
#define LED_NUM8	(0x80)
#define LED_NUM9	(0x90)
#endif  /* LED_XXX */

/* Keypad/LED register values for LED Brightness levels */
#define HIGH_BRIGHTNESS_PRESCALAR       0x0055
#define HIGH_BRIGHTNESS_DUTYCYCLE       0x01AA
#define HIGH_BRIGHTNESS_DEBOUNCE        0x8000

#define DIM_BRIGHTNESS_PRESCALAR        0x02A8
#define DIM_BRIGHTNESS_DUTYCYCLE        0x2020
#define DIM_BRIGHTNESS_DEBOUNCE         0xFF00

/*
 *  Front Panel Keys
 */
#define FP_POWER	0x00
#define FP_UP		0x08
#define FP_DOWN		0x01
#define FP_RIGHT	0x05
#define FP_LEFT		0x04
#define FP_SELECT	0x0c
#define FP_GUIDE	0x09
#define FP_MENU		0x0d
#define FP_CHUP		0x02
#define FP_CHDOWN	0x06
#define FP_VOLUP	0x0a
#define FP_VOLDOWN	0x0e

/****************************************************************************
* IR Receiver Registers
****************************************************************************/
#define IRR_BASE            (0x80)   /* base address of IR Receiver */

/* Register addresses */
#define IRR_STATUS          PCI_ADDR8((IRR_BASE+1))          /* IRR status register unsigned char            */
#define IRR_CODE1           PCI_ADDR8((IRR_BASE+0))          /* Code 1 length register unsigned char         */
#define IRR_CODE0           PCI_ADDR8((IRR_BASE+1))          /* Code 0 length register unsigned char         */
#define IRR_PREAMBLE_HI     PCI_ADDR8((IRR_BASE+2))          /* Preamble length register unsigned char       */
#define IRR_PREAMBLE_LO     PCI_ADDR8((IRR_BASE+3))          /* Preamble length register unsigned char       */
#define IRR_DATABUF1        PCI_ADDR8((IRR_BASE+4))          /* IRR data buffer register unsigned char       */
#define IRR_DATABUF0        PCI_ADDR8((IRR_BASE+5))          /* IRR data buffer register unsigned char       */
#define IRR_PRESCALER       PCI_ADDR8((IRR_BASE+6))          /* IRR Prescaler register unsigned char         */ 
#define IRR_TIME_OUT        PCI_ADDR8((IRR_BASE+7))          /* IRR Time out regsiter unsigned char          */

/* bit mask definitions within IRR_STATUS */
#define IRRIN               1   /* IR state bit 0 */
#define IRRBAD              2   /* IR state bit 1 */
#define IRVREP              4   /* IR state bit 2 */
#define IRRCV               8   /* IR receiver state receive mode */
#define IRREP               16  /* valid repeat code received */
#define IRDAV               32  /* valid IR data available in buffer */
#define IRNPR               64  /* invalid PREAMBLE received */
#define IRNDA               128 /* data not completely received in time */

/* bit mask definitions within IRR_PREAMBLE */
#define IRRST               0x0200   /* IR receiver reset control line */
#define IHFAIL              0x0400   /* IR receiver hardware failure */
#define IRQEN               0x0800   /* enable IR receiver interrupt */

/* IRR reset time delay */
#define IRR_RESET_DELAY     2       /* ticks */

/****************************************************************************
* IR Blaster Registers
****************************************************************************/
#define IRB_BASE            (0x00) /* base address of IR Blaster */

/* Register addresses */
#define IRB_CONTROL         (IRB_BASE+0) /* IRB control/numseq register unsigned short */
#define IRB_PRESCALAR       (IRB_BASE+2) /* IRB index clock/prescalar unsigned short */
#define IRB_REPEAT          (IRB_BASE+4) /* IRB repeat index, numrepeat reg unsigned short */
#define IRB_CARRIER         (IRB_BASE+6) /* IRB carrier hi/low register unsigned short */
#define IRB_SEQUENCEX       (IRB_BASE+8) /* IRB sequence memory base 20 unsigned short */
#define IRB_CARRIER_INDEX   (IRB_BASE+48) /* IRB carrier index base 14 unsigned short */

/* Bit and mask definitions for control and number of sequences register */
#define IRB_CARRINH         0x0001  /* Carrier inhibit (Flash IR)
                                    0 = carrier modulated (PWM IR) */
#define IRB_COFFLEV         0x0002  /* logic level of carrier off */
#define IRB_IRQINH          0x0004  /* disable interrupt when set */
#define IRB_CLI             0x0008  /* clear interrupt */
#define IRB_RST             0x0010  /* reset IR blaster */
#define IRB_PAGE            0x0020  /* sequence page 0 or 1 select (0==page 0) */
#define IRB_INTENSIFY       0x0040  /* infra-red intensity level */

#ifdef COMMENT
#define IRB_ZING            0x0040  /* ZING enable control line */
#endif  /* COMMENT ZING removed from hardware */

#define IRB_BLAST           0x0080  /* go initiate IR blasting sequence */
#define IRB_NUMSEQ          0x7F00  /* mask for number of sequences */
#define IRB_MAX_SEQ         80      /* max number of IRB sequences ver 1000 */
#define IRB_MAX_SEQ0        40      /* max number of IRB sequences ver 0000 */
#define IRB_PAGE_SIZE       40      /* page size in bytes */


/* UART I/O structure - maps ASIC UART register group */
#ifdef _C_STRUCT_
typedef struct _UART_DEVICE
{
    volatile unsigned char   ucRecvStatus;
    volatile unsigned char   ucRecvData;
    volatile unsigned short   uwControl;
    volatile unsigned short   uwBaudrate;
    volatile unsigned char   ucXmitStatus;
    volatile unsigned char   ucXmitData;
} UART_DEVICE;
#endif _C_STRUCT_


/****************************************************************************
* UART QUOD Macros
****************************************************************************/

/* Compute baud rate
 * assume 27.0 MHz system clock -- NOT anymore, now using clock_speed
 * baud rate register value = (system_clock / (16 * baudrate)) -1
 * example:
 *      BRR = (27000000 / (16 * baudrate)) - 1
 *            (1687500 / baudrate) - 1
 */
#define UARTbaudrate(x)     (((( (clock_speed>>4) / (x)) + 5L ) / 10L) - 1L)

/* Pointer variables */
#define pUARTA              ((volatile UART_DEVICE *) (UARTA_BASE))
#define pUARTB              ((volatile UART_DEVICE *) (UARTB_BASE))

/****************************************************************************
* Interrupt Registers
****************************************************************************/

/*
 * These bits represent the offset from BCM3250 interrupts
 * in the vector table for the corresponding ISR
 */
#define UAIRQ_BIT               4
#define UBIRQ_BIT               3
#define IRBIRQ_BIT              2
#define KLIRQ_BIT               1
#define IRRIRQ_BIT              0

#define UAIRQ                   0x10
#define UBIRQ                   0x08
#define IRBIRQ                  0x04
#define KLIRQ                   0x02
#define IRRIRQ                  0x01

/****************************************************************************
* Generic I/O Registers
****************************************************************************/
#if 0
/* Generic I/O Port Registers */
#define GENERIC_IOPORT0  PCI_ADDR16(0x70)  /* unsigned short Generic I/O Port 
*/

/* Generic I/O Port Registers */
#define GIO_OUTMSB       PCI_ADDR8(0x72)  /* unsigned char Generic I/O Port */
#define GIO_OUTLSB       PCI_ADDR8(0x73)  /* unsigned char Generic I/O Port */

/* Generic I/O Direction Registers */
#define GENERIC_IODIR0   PCI_ADDR16(0x74)  /* unsigned char Generic I/O Dir */
#define GIO_DIRMSB       PCI_ADDR8(0x74)  /* unsigned char Generic I/O Dir */
#define GIO_DIRLSB       PCI_ADDR8(0x75)  /* unsigned char Generic I/O Dir */

/* Generic I/O Expansion Registers */
#define GIO_EXPANSION    PCI_ADDR8(0x76)  /* unsigned char Generic I/O Exp */
#endif

/* Generic I/O Port Registers */
#define GENERIC_IOPORT0  PCI_ADDR16(0x00)  /* unsigned short Generic I/O Port */

/* Generic I/O Port Registers */
#define GIO_OUTMSB       PCI_ADDR8(0x02)  /* unsigned char Generic I/O Port */
#define GIO_OUTLSB       PCI_ADDR8(0x03)  /* unsigned char Generic I/O Port */

/* Generic I/O Direction Registers */
#define GENERIC_IODIR0   PCI_ADDR16(0x04)  /* unsigned char Generic I/O Dir */
#define GIO_DIRMSB       PCI_ADDR8(0x04)  /* unsigned char Generic I/O Dir */
#define GIO_DIRLSB       PCI_ADDR8(0x05)  /* unsigned char Generic I/O Dir */

/* Generic I/O Expansion Registers */
#define GIO_EXPANSION    PCI_ADDR8(0x06)  /* unsigned char Generic I/O Exp */

/******************************************************************
 * Baud Rate Table
 ******************************************************************/
#define BAUD_1200_HI    0x05
#define BAUD_1200_LO    0x7D
#define BAUD_2400_HI    0x02
#define BAUD_2400_LO    0xBE
#define BAUD_4800_HI    0x01
#define BAUD_4800_LO    0x5F
#define BAUD_9600_HI    0x00
#define BAUD_9600_LO    0xAF
#define BAUD_19200_HI   0x00
#define BAUD_19200_LO   0x57
#define BAUD_38400_HI   0x00
#define BAUD_38400_LO   0x2B
#define BAUD_57600_HI   0x00
#define BAUD_57600_LO   0x1C
#define BAUD_115200_HI  0x00
#define BAUD_115200_LO  0x0E

#endif /* this should be the last line */

