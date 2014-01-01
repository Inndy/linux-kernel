/************************************************************************/
/*                                                                     	*/
/*   MODULE:  bcm93730.h                                   		*/
/*   DATE:    March 7th 2000                                            */
/*   PURPOSE: Define addresses of major hardware components of          */
/*            BCM93725                                                  */
/*                                                                      */
/************************************************************************/
#ifndef __BCM93730_MAP_H
#define __BCM93730_MAP_H


#if __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Board Version Definitions						*/
/************************************************************************/
#define BOARD_ID_V00	0x0000
#define BOARD_ID_V01	0x0001
#define BOARD_ID_V02	0x0002

/****************************************************************************
* PLD Revision ID
****************************************************************************/
#define PLD_ID_V00              0x0000
#define PLD_ID_V01              0x0001
#define PLD_ID_V02              0x0002

#define _SETTOP_		/* for titanapifinal.cpp. */

/*************************************************************************/
/* 3250 external crystal frequency                                       */
/*************************************************************************/
#define XTAL_FREQ_3250			24.0
#define SYS_XTAL_FREQ_3250		24.0

#define	QAM_ANNEX_MODE			eAnnexB		/* make use Annex and XTAL_FREQ match */

/*************************************************************************/
/* display configurations                                                */
/*************************************************************************/

#define VIDEO_DISPLAY_FORMAT   0 /*eFMT_NTSC*/

/*************************************************************************/
/* symbol rates                                                          */
/*************************************************************************/
#define DEFAULT_QAM64_SYM_RATE		5056900
#define DEFAULT_QAM256_SYM_RATE		5360500


#ifndef PHYS_TO_K0
#define PHYS_TO_K0(x)	((x) | 0x80000000)
#endif
#ifndef PHYS_TO_K1
#define PHYS_TO_K1(x)	((x) | 0xa0000000)
#endif


#ifndef K0_TO_PHYS
#define K0_TO_PHYS(x)	((x) & 0x1fffffff)
#endif
#ifndef K1_TO_PHYS
#define K1_TO_PHYS(x)	(K0_TO_PHYS(x))
#endif
#ifndef K0_TO_K1
#define K0_TO_K1(x)		(PHYS_TO_K1(x))
#endif
#ifndef K1_TO_K0
#define K1_TO_K0(x)		(PHYS_TO_K0(K1_TO_PHYS(x)))
#endif


/************************************************************************/
/* Address Definitions							*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/*------------------- Address Memory Map ------------------------------*/
/*----------------------------------------------------------------------*/
#define BCM_7020_REG_BASE      		0xb0000000      /* 7020 Register Base   */
#define BCM_7020_REG_BASE_NOCACHE      	0xb0000000      /* 7020 Register Base   */
#define BCM_7020_GRAPH_COEF_NOCACHE    0xb0000800      /* 7020 Graphic Coef.   */
#define BCM_7020_VDEC_COEF_NOCACHE     0xb0000C00      /* 7020 VDEC Coef.      */
#define BCM_7020_NABTS_NOCACHE         0xb0001000      /* 7020 NABTS RAM       */
#define BCM_7020_DENC_NOCACHE          0xb0001400      /* 7020 DENC            */
#define BCM_7020_ADP_NOCACHE           0xb0001800      /* 7020 ADP             */
#define BCM_7020_TITAN_NOCACHE         0xb0002000      /* 7020 TITAN           */
#define BCM_7020_OX_NOCACHE            0xb0004000      /* 7020 OX              */


/***********************************************************************
* venom2 reg defs                                                      *
************************************************************************/

#define VENOM2_PCI_MEM_WIN0_CPU_ADDR_NOCACHE		0xb4000000
#define VENOM2_PCI_MEM_WIN1_CPU_ADDR_NOCACHE		0xb8000000
#define VENOM2_PCI_MEM_WIN2_CPU_ADDR_NOCACHE		0xa8000000
#define VENOM2_PCI_MEM_WIN3_CPU_ADDR_NOCACHE		0xac000000

#define VENOM2_PCI_IO_WIN0_CPU_ADDR_NOCACHE		0xb3000000
#define VENOM2_PCI_IO_WIN1_CPU_ADDR_NOCACHE		0xb3200000
#define VENOM2_PCI_IO_WIN2_CPU_ADDR_NOCACHE		0xa3400000

#define VENOM2_PCI_CFG_ADDR_NOCACHE            	0xb3c00000

#define VENOM2_PCI_CFG_CF8_REG_NOCACHE          0xb3e00cf8
#define VENOM2_PCI_CFG_CFC_REG_NOCACHE         	0xb3e00cfc

#define VENOM2_PCI_CFG_CTRL_REG_NOCACHE        	0xb3c00cfc

#define VENOM2_PCI_CFG_MEM_WIN0_BASE_REG_NOCACHE		0xb3c00058
#define VENOM2_PCI_CFG_MEM_WIN1_BASE_REG_NOCACHE		0xb3c0005c
#define VENOM2_PCI_CFG_MEM_WIN2_BASE_REG_NOCACHE		0xb3c00060
#define VENOM2_PCI_CFG_MEM_WIN3_BASE_REG_NOCACHE		0xb3c00064

#define VENOM2_PCI_CFG_IO_WIN0_BASE_REG_NOCACHE			0xb3c00068
#define VENOM2_PCI_CFG_IO_WIN1_BASE_REG_NOCACHE			0xb3c0006c
#define VENOM2_PCI_CFG_IO_WIN2_BASE_REG_NOCACHE			0xb3c00070

#define VENOM2_PCI_CFG_SDRAM_WIN0_BASE_REG_NOCACHE		0xb3c00010
#define VENOM2_PCI_CFG_SDRAM_WIN1_BASE_REG_NOCACHE		0xb3c00014
#define VENOM2_PCI_CFG_SDRAM_WIN2_BASE_REG_NOCACHE		0xb3c00018
#define VENOM2_PCI_CFG_SDRAM_SIZE_REG_NOCACHE			0xb3c00050
#define VENOM2_PCI_CFG_SDRAM_ENDIAN_TRANS_REG_NOCACHE	0xb3c0007c

#define VENOM2_PCI_CFG_CLASS_CODE_REV_ID_REG_NOCACHE		0xb3c00008

#define VENOM2_PCI_DMA_CTRL_REG_NOCACHE					0xb000030c


/*************************************************************************
*legacy 7030 defs                                                        *
**************************************************************************/
#define BCM_7020_PCI_INT_CONF_NOCACHE			VENOM2_PCI_CFG_ADDR_NOCACHE      /* 7020 PCI Config Base */
#define BCM_7020_PCI_MEM_REMAP_NOCACHE 			VENOM2_PCI_CFG_MEM_WIN0_BASE_REG_NOCACHE      /* 7020 PCI Mem. Remap  */
#define BCM_7020_PCI_IO_REMAP_NOCACHE			VENOM2_PCI_CFG_IO_WIN0_BASE_REG_NOCACHE      /* 7020 PCI I/O Remap   */
#define BCM_7020_PCI_MEM_BASE_ADDR_NOCACHE		VENOM2_PCI_CFG_SDRAM_WIN0_BASE_REG_NOCACHE    /* 7020 pci MEM base address */
#define BCM_7020_PCI_MEM_SIZE_NOCACHE			VENOM2_PCI_CFG_SDRAM_SIZE_REG_NOCACHE      /* 7020 pci mem size reg. */
#define BCM_7020_PCI_CONTROL					VENOM2_PCI_CFG_CTRL_REG_NOCACHE
#define BCM_7020_PCI_EXT_CF8_NOCACHE			VENOM2_PCI_CFG_CF8_REG_NOCACHE      /* 7020 PCI Ext CF8     */
#define BCM_7020_PCI_EXT_CFC_NOCACHE			VENOM2_PCI_CFG_CFC_REG_NOCACHE      /* 7020 PCI Ext CFC     */
#define PHYS_7020_PCI_EXT_IO				(K1_TO_PHYS(VENOM2_PCI_IO_WIN0_CPU_ADDR_NOCACHE))      /* 7020 PCI Ext IO      */
#define PHYS_7020_PCI_EXT_MEM				(K1_TO_PHYS(VENOM2_PCI_MEM_WIN0_CPU_ADDR_NOCACHE))      /* 7020 PCI Ext Mem.    */
#define BCM_7020_PCI_EXT_IO_NOCACHE    	(PHYS_7020_PCI_EXT_IO|0xa0000000)
#define BCM_7020_PCI_EXT_MEM_NOCACHE   	(PHYS_7020_PCI_EXT_MEM|0xa0000000)

#define BOARD_PLD_REV_NOCACHE		0xbfa00000
#define JUMPER_STAT_REV_NOCACHE		0xbf800000

#define PHYS_7020_SDRAM			0x00000000
#define SDRAM_BASE_7020              	0x80000000
#define SDRAM_BASE_7020_NOCACHE      	0xa0000000

#define BCM_7020_PCI_MEM_BASE              0x00000000      /* Base of SDRAM relative to PCI Master */   

#define ROM_BASE                        0xbfc00000
#define ROM_BASE_NOCACHE                0xbfc00000
#define ROM_BASE_END                	(0xbfc00000+0x00200000)        

/*----------------------------------------------------------------------
 *-------------------- 68K IO Bus Memory Map ---------------------------
 *----------------------------------------------------------------------
 *	68K IO Bus Addresses are defined by the Chip Select values
 *	loaded into the Bcm7020 IO CS registers.  The ROM Base Address
 *	is defined by the MIPS ISA.  All other addresses may be placed
 *	anywhere in the range allocated by the Bcm7020 for 68K IO.
 *	The Bcm7020 provides a 64 Megabyte Range for 68K IO accesses
 *	beginning at the base address 0xBC00,0000. The Bcm7020 CS
 * 	signals are wired on the Bcm93725 as:			
 *
 *		CS0: BOARD_REV/PLD_REV/ROM - 2Meg
 *		CS1: Flash - 16Meg
 *		CS2: HPNA - 1Meg
 *		CS3: VOIP - 1Meg
 *		CS4: IEEE1394 - 1Meg
 *		CS5: POD - 1Meg
 *                                                             
 *----------------------------------------------------------------------*/

#define PHYS_BOARD_REV      0x1fa00000      /* Board Revision       */
#define PHYS_PLD_REV        0x1fa00000      /* PLD Revision         */
#define PHYS_JUMPER_STAT    0x1f800000      /* jumper setting */

#define PHYS_HPNA_BASE		0x1c000000	/* HPNA Device Base */
#define HPNA_SIZE			0x00100000	/* HPNA Device Base */

#define PHYS_VOIP_BASE		0x1c100000	/* VOIP Device Base */
#define VOIP_SIZE			0x00100000	/* VOIP Device Base */

#define PHYS_IEEE1394_BASE	0x1c200000	/* IEEE1394 Device Base */
#define IEEE1394_SIZE		0x00100000	/* IEEE1394 Device Base */

#define PHYS_POD_BASE		0x1c300000	/* POD Device Base */
#define POD_SIZE			0x00100000	/* POD Device Base */
		/* 
		 * NOTE: we are putting the Flash at 4Meg offset
		 * 	so boards that have only 8Meg will still
		 * 	work the same as boards that have 16Meg
		 */
#define PHYS_FLASH_BASE     0x1e000000      /* Flash Base */
#define FLASH_SIZE          0x01000000      /* 16 Mbytes	*/

	/********************************************************
	 * 68K MIPS Addresses
	 ********************************************************/
#define BOARD_REV           PHYS_TO_K0(PHYS_BOARD_REV)      
#define BOARD_REV_NOCACHE   PHYS_TO_K1(PHYS_BOARD_REV)  
#define PLD_REV             PHYS_TO_K0(PHYS_PLD_REV)      
#define PLD_REV_NOCACHE     PHYS_TO_K1(PHYS_PLD_REV)  
#define JUMPER_STAT			PHYS_TO_K0(PHYS_JUMPER_STAT)
#define JUMPER_STAT_NOCACHE	PHYS_TO_K1(PHYS_JUMPER_STAT)

#define HPNA_BASE_NOCACHE	PHYS_TO_K1(PHYS_HPNA_BASE)  
#define HPNA_BASE_END		PHYS_TO_K1(PHYS_HPNA_BASE+HPNA_SIZE)  

#define VOIP_BASE_NOCACHE	PHYS_TO_K1(PHYS_VOIP_BASE)  
#define VOIP_BASE_END		PHYS_TO_K1(PHYS_VOIP_BASE+VOIP_SIZE)  

#define IEEE1394_BASE_NOCACHE	PHYS_TO_K1(PHYS_IEEE1394_BASE)  
#define IEEE1394_BASE_END	PHYS_TO_K1(PHYS_IEEE1394_BASE+IEEE1394_SIZE)  

#define POD_BASE_NOCACHE	PHYS_TO_K1(PHYS_POD_BASE)  
#define POD_BASE_END		PHYS_TO_K1(PHYS_POD_BASE+POD_SIZE)  

#define FLASH_BASE_NOCACHE	PHYS_TO_K1(PHYS_FLASH_BASE)  
#define FLASH_BASE_END		PHYS_TO_K1(PHYS_FLASH_BASE+FLASH_SIZE)  

/*
* Board's MAC addresses are stored in Flash and
* should be configured using BootROM
*
* MAC addresses assigned in following order:
*
* MAC_ADDR_0 (offset 0x1f80c )  CableModem
* MAC_ADDR_1 (offset 0x1f814 )  On-board Ethernet ( BCM 4413)
* MAC_ADDR_2 (offset 0x1f81c )  HPNA
* MAC_ADDR_3 - 15 (offset 0x1f824 - 0x1f884 )  Reserved
*/     
#define CONFIG_FLASH_BASE	(FLASH_BASE_NOCACHE + 0x00fe0000)
#define CONFIG_FLASH_SIZE	(0x00020000)
#define FLASH_MAC_ADDR_0	(0x0001f80c)
#define FLASH_MAC_ADDR_1	(0x0001f814)
#define FLASH_MAC_ADDR_2	(0x0001f81c)
#define FLASH_MAC_ADDR_3	(0x0001f824)

     /*
	 * Bcm7020 Chip Select Register Values
	 * bits[31:16] = bits [25:10] of Upper address in range
	 * bits[15:0 ] = bits [25:10] of Lower address in range
	 * don't forget the endian.
	 */
#define IO_CS_VAL(l,u)	(((l & 0x03fffc00) >> 10) | ((u & 0x03fffc00) << 6))

#define HPNA_IO_CS	(IO_CS_VAL(HPNA_BASE_NOCACHE,HPNA_BASE_END))
#define VOIP_IO_CS	(IO_CS_VAL(VOIP_BASE_NOCACHE,VOIP_BASE_END))
#define IEEE1394_IO_CS	(IO_CS_VAL(IEEE1394_BASE_NOCACHE,IEEE1394_BASE_END))
#define POD_IO_CS	(IO_CS_VAL(POD_BASE_NOCACHE,POD_BASE_END))

#define FLASH_IO_CS	(IO_CS_VAL(FLASH_BASE_NOCACHE,FLASH_BASE_END))
#define ROM_IO_CS	(IO_CS_VAL(JUMPER_STAT_NOCACHE,ROM_BASE_END))
	

/*----------------------------------------------------------------------
 *------------------- Physical Memory Size -----------------------------
 *----------------------------------------------------------------------*/
#define BCM7020_MEM_SIZE        0x04000000      /* 64 Mbytes            */
#define BCM_7020_PCI_MEM_SIZE   BCM7020_MEM_SIZE
#define BCM3250_MEM_SIZE        0x00800000      /* 8 Mbytes             */

#if __cplusplus
}
#endif


/****************************************************************************
*
* MIPS External Interrupts:
*       MIPS_SYSINT0:   IRQ_VOIP
*       MIPS_SYSINT1:   IRQ_1394
*       MIPS_SYSINT2:   IRQ_7020
*       MIPS_SYSINT3:   IRQ_3250
*       MIPS_SYSINT4:   IRQ_ALI
*       MIPS_SYSINT5:   IRQ_TIMER
*
****************************************************************************/
	/* 
	 * CPU SysInt 
	 */
#define	SYSINT_VOIP	0	
#define SYSINT_1394	1
#define SYSINT_7020	2
#define SYSINT_3250	3
#define SYSINT_ALI	4
#define SYSINT_INT5	5

#define SR_INT_MASK(x)	(1<<(x+10))
#define CR_INT_MASK(x)	(1<<(x+10))

	/*
	 * ALi is the PIC for most of the peripheral interrupts
	 * These definitions are used to configure it.
	 */
#define ALI_IRQ_INTA			15	/* input from PCI Slot INTA
						 * normally ATA Bus Secondary IDE 
						 * we use this as the INTA input
						 * from the PCI Slot (IDSEL13) since
						 * the Secondary IDE is not wired. */ 
#define ALI_IRQ_IDE			14	/* ATA Bus Primary IDE */ 
#define ALI_IRQ_13			13	/* not used */
#define ALI_IRQ_12			12	/* used for mouse data */ 
#define ALI_IRQ_USB			11	/* ALi USB */ 
#define ALI_IRQ_10			10	/* used for keyboard data */ 
#define ALI_IRQ_9			9	/* used for cascading 8259 */ 
#define ALI_IRQ_RTC			8	/* ALi RTC */ 
#define ALI_IRQ_PARALLEL	7	/* ALi Parallel Port */ 
#define ALI_IRQ_ENET		6	/* BCM4413 HPNA/V.90/ENET */
#define ALI_IRQ_POD			5	/* input from POD  */ 
#define ALI_IRQ_4			4	/* not used */
#define ALI_IRQ_3			3	/* not used */ 
#define ALI_IRQ_2			2	/* Cascade from second half of ALi 8259 */ 
#define ALI_IRQ_KEYBOARD		1	/* ALi Keyboard */ 
#define ALI_IRQ_0			0	/* not used */


#define NEC_OEM_CLOCK_FREQ    81000000                /* Hz */
#define NEC_CPU_CLOCK         81000000                /* Hz */

#define QED_OEM_CLOCK_FREQ    56000000                /* Hz */
#define QED_CPU_CLOCK         (56000000*2)            /* Hz */


#define BCM7020_SYS_MEM_SIZE	((BCM7020_MEM_SIZE*3)/4)
#define BCM7020_GRAPH_MEM_SIZE	(BCM7020_MEM_SIZE-BCM7020_SYS_MEM_SIZE)

/****************************************************************************
* BCM7020c1 Registers
****************************************************************************/
#include "venom2.h"	/* Graphics Registers */
/** * This is from \SetTop\portinginterface\mpegvideo\venom2\include * (
replacement of bintdefs.h and bregdef.h) */
#include "mpegaddress.h"

	/*
	 * macros for kernel mode access by offset
	 */
#define WriteReg7020(o,d) (*(volatile unsigned long*)(BCM_7020_REG_BASE_NOCACHE+o)=d)
	
#define ReadReg7020(o)	 (*(volatile unsigned long*)(BCM_7020_REG_BASE_NOCACHE+o)) 
	

/************************************************************************/
/* Endian Swapping Macros                                                                                               */
/************************************************************************/
#if defined(MIPSEL) || defined(__MIPSEL__) || defined( _LITTLE_ENDIAN_ ) || defined(CONFIG_CPU_LITTLE_ENDIAN)
#ifndef SWAP_END16
 #define SWAP_END16(x) ( ((x&0xff00)>>8) |((x&0x00ff)<<8) )
#endif

#ifndef SWAP_END32
 #define SWAP_END32(x) ( ((x&0xff000000)>>24) |((x&0x00ff0000)>>8) | ((x&0x0000ff00)<<8)  |((x&0x000000ff)<<24) )
#endif

#else
/* Big Endian */
#ifndef SWAP_END16
 #define SWAP_END16(x) ( x )
#endif

#ifndef SWAP_END32
 #define SWAP_END32(x) ( x )
#endif
#endif  /* BigEndian */

/****************************************************************************
* BCM3250 Registers
****************************************************************************/
#include "bcmaccess.h"	/* macros for register access */
#include "bcmupg.h"	/* definitions for UPG section of 3250 */
#include "bcm3250.h"	/* definitions for the 3250 */

#define RX_UART_DATA            0x0001
#define RX_IR_DATA              0x0002
#define RX_KEYPAD_DATA          0x0004
#define RX_UART_FULL            0x0100
#define RX_IR_FULL              0x0200
#define RX_KEYPAD_FULL          0x0400

#define N_SIO_CHANNELS	2	/* 2 uarts */


/****************************************************************************
 * PCI Bus Bridge Definitions
 ****************************************************************************/
	/********************************************************
	 * Standard PCI Configuration Registers
	 * CF8 Configuration Register format:
	 * 	Bit[31]		ENABLE
	 * 	Bit[30-11]	PCI Device Number
	 * 	Bit[10-2]	Device Register Offset
	 ********************************************************/
#define PCI_CF8_VENDOR          0x00
#define PCI_CF8_DEVICE          0x02	
#define PCI_CF8_CMD             0x04
#define PCI_CF8_STAT            0x06
#define PCI_CF8_CC_REV          0x08
#define PCI_CF8_HDR_CFG         0x0C
#define PCI_CF8_IO_BASE         0x10	/* BAR0 */
#define PCI_CF8_MEM_BASE        0x14	/* BAR1 */
#define PCI_CF8_REG_BASE        0x18	/* BAR2 */
#define PCI_CF8_PCU_BASE        0x1C	/* BAR3 */
#define PCI_CF8_BAR4		0x20	/* BAR4 */
	/********************************************************
	 * Standard PCI CFC Configuration Register Values
	 ********************************************************/
#define PCI_ENABLE              0x80000000
#define PCI_IO_ENABLE           0x00000001
#define PCI_MEM_ENABLE          0x00000002
#define PCI_BUS_MASTER          0x00000004

	/********************************************************
	 * BCM93725 PCI Bus Device Connections
	 *
         * PCI Device Number    PCI Address     PCI Device
         *      0               AD16            Bcm4413
         *      1               AD17            BCM3250 
         *      2               AD18            ALI ISA 153310b9
         *      3               AD19
         *      4               AD20
         *      5               AD21
         *      6               AD22
         *      7               AD23
         *      8               AD24
         *      9               AD25
         *      10              AD26
         *      11              AD27            ALI IDE 522910B9
         *      12              AD28			ALI PMU 710110b9
         *      13              AD29            SLOT A
         *      14              AD30            SLOT B
         *      15              AD31            ALI USB 523710b9
 	 ********************************************************/
#define PCI_IDSEL(x)		(((x)&0x1f)<<11)
#define PCI_FNSEL(x)		(((x)&0x7)<<8)

#define BCM4410_IDSEL		0
#define BCM4410_VID		0x441014e4
#define BCM4411_IDSEL		0
#define BCM4411_VID		0x441114e4
#define BCM4412_IDSEL		0
#define BCM4412_VID		0x441214e4
#define BCM3250_IDSEL		1
#define BCM3250_VID		0x325014e4	/* PCI Vendor ID */
#define ALI_ISA_IDSEL		2
#define ALI_ISA_VID		0x153310b9	/* PCI Vendor ID */
#define ALI_IDE_IDSEL		11
#define ALI_IDE_VID		0x522910B9	/* PCI Vendor ID */
#define ALI_PMU_IDSEL		12
#define ALI_PMU_VID		0x710110b9	/* PCI Vendor ID */
#define ALI_USB_IDSEL		15
#define ALI_USB_VID		0x523710b9	/* PCI Vendor ID */
	/* 
	 * Only one PCI slot is populated with a connector
	 */
#define SLOTA_IDSEL		        13
#define NE2000_IDSEL		    SLOTA_IDSEL	
#define SIIG_IDE_VID		    0x00051191	/* SIIG IDE Controller card */
#define LINKSYS_ETHERNET_VID	0x09401050	/* Linksys NE2000 Ethernet card */
#define SMC_ETHERNET_VID	    0x802910ec	/* SMC NE2000 Ethernet card */
#define BCM7041_VID		    0xA000141F  /* BCM7041 MPEG2 Encoder */

	/********************************************************
	 * PCI Bus Addresses
	 ********************************************************/
		/* 
		 * Base Definitions
		 */
#define MIPS_IO_BASE		PHYS_7020_PCI_EXT_IO
#define MIPS_MEM_BASE		PHYS_7020_PCI_EXT_MEM



			/*
			 * 7020b subtracts the mips pci io window addr.
			 * don't use offset since pci io legacy is zero-based.
			 */
#define BCM_7020_IO_REMAP  	0x00000000     /*(0-MIPS_IO_BASE)*/	

			/*
			 * 7020b subtracts mips pci mem window addr so add it back
			 */
#define BCM_7020_MEM_REMAP  	0x14000000   

			/*
			 * add the MIPS Segment offset
			 */
#define MIPS_IO_ADDR(x)		(PHYS_TO_K1(x + MIPS_IO_BASE))

#define MIPS_MEM_ADDR(x)	(PHYS_TO_K1(x))


		/*
		 * IO Access value programmed in Device BAR
		 * The IO Access range is 1 Megabyte
		 */
#define ALI_IDE_OFFSET		0x000001f0
#define ALI_PARALLEL_OFFSET	0x00000378	
#define NE2000_PCI_IO_BASE	0x00006000	

		/*
		 * Memory Access value programmed in Device BAR
		 * The Memory Access range is 64 Megabyte
		 */
#define PCI_MEMORY_BASE_   0x14000000    /*0x18100000*/

			/* 
			 * Bcm3250 SRAM Base Address Register
			 */
#define BCM_3250_PCI_MEM_BASE   (PCI_MEMORY_BASE_+0x00000000)
#define BCM_3250_PCI_MEM_SIZE   0x01000000	/* requires 16Meg bytes */

			/* 
			 * Bcm3250 Registers Base Address Register
			 * NOTE: Registers are accessed as Memory
			 */
#define BCM_3250_PCI_REG_BASE   (BCM_3250_PCI_MEM_BASE+BCM_3250_PCI_MEM_SIZE)
#define BCM_3250_PCI_REG_SIZE   0x00010000	/* requires 57344 bytes*/

			/*
			 * BCM4413 Memory Base Address Register
			 */
/* bcm4413 iLine 20 */
#define BCM4410_PCI_MEM_BASE	(BCM_3250_PCI_REG_BASE+BCM_3250_PCI_REG_SIZE) 
#define BCM4410_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

/* bcm4413 codec */
#define BCM4411_PCI_MEM_BASE	(BCM4410_PCI_MEM_BASE+BCM4410_PCI_MEM_SIZE)
#define BCM4411_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

/* bcm4413 ENET */
#define BCM4412_PCI_MEM_BASE	(BCM4411_PCI_MEM_BASE+BCM4411_PCI_MEM_SIZE)
#define BCM4412_PCI_MEM_SIZE	0x00001000	/* requires 4096 bytes */

			/*
			 * USB Base Address Register
			 */
#define ALI_USB_MEM_BASE	(BCM4412_PCI_MEM_BASE + BCM4412_PCI_MEM_SIZE )
#define ALI_USB_MEM_SIZE	0x00000100	/* requires 224 bytes */

#define BCM7041_MEM_BASE      ((ALI_USB_MEM_BASE + ALI_USB_MEM_SIZE + 0x20000)&0xfffe0000)
#define BCM7041_MEM_SIZE      (6*0x20000)

/****************************************************************************
 * PCI Device Definitions
 ****************************************************************************/

		/*
		 * MIPS program address for ALi
		 */
#include "ali.h"
#define ALI_IDE_BASE		MIPS_IO_ADDR(ALI_IDE_OFFSET)
#define ALI_PARALLEL_BASE	MIPS_IO_ADDR(ALI_PARALLEL_OFFSET)	


		/*
		 * MIPS program address for  Bcm3250
		 */
#define BCM_3250_GLOBAL		MIPS_MEM_ADDR(BCM_3250_PCI_REG_BASE+GLOBAL_REG)
#define BCM_3250_UPG		MIPS_MEM_ADDR(BCM_3250_PCI_REG_BASE+UPG)
#define BCM_3250_SDRAM_CNTRL	MIPS_MEM_ADDR(BCM_3250_PCI_REG_BASE+SDRAM_CONTROLLER)
#define BCM_3250_REG_BASE_NOCACHE	MIPS_MEM_ADDR(BCM_3250_PCI_REG_BASE)

         /*
		 * MIPS program address for PCI Card
		 */
#define ETHER16LAN_IOADDR		MIPS_IO_ADDR(0)  /* reads BAR for offset  */
#define ETHER16LAN_MEMADDR		MIPS_MEM_ADDR(0) /* reads BAR for offset  */

/******************************************************************
 * Bad stuff left over from Irvine code.
 * These macros should be eliminated and a DeviceRead/Write used. 
 ******************************************************************/
/* Generic I/O variable definitions */
#define ubOUTMSB  (*(volatile unsigned char*) (BCM_3250_UPG+GIO_OUTMSB))
#define ubOUTLSB  (*(volatile unsigned char*) (BCM_3250_UPG+GIO_OUTLSB))

/* Generic I/O direction variable definitions */
#define ubDIRMSB  (*(volatile unsigned char*) (BCM_3250_UPG+GIO_DIRMSB))
#define ubDIRLSB  (*(volatile unsigned char*) (BCM_3250_UPG+GIO_DIRLSB))

/* Generic I/O Expansion variable definitions */
#define ubEXPANSION      (*(volatile unsigned char*)(BCM_3250_UPG+GIO_EXPANSION))

/* Generic I/O variable definitions */
#define uwGENIO0 (*(volatile unsigned short*) (BCM_3250_UPG+GENERIC_IOPORT0))

/* Generic I/O direction variable definitions */
#define uwGENDR0i (*(volatile unsigned short*) (BCM_3250_UPG+GENERIC_IODIR0))

                             
/* register variables */
#define ucLED_Digit1 (*(volatile unsigned char*)(BCM_3250_UPG+LED_DIGIT1))
#define ucLED_Digit2 (*(volatile unsigned char*)(BCM_3250_UPG+LED_DIGIT2))
#define ucLED_Digit3 (*(volatile unsigned char*)(BCM_3250_UPG+LED_DIGIT3))
#define ucLED_Digit4 (*(volatile unsigned char*)(BCM_3250_UPG+LED_DIGIT4))
#define ucLED_Status (*(volatile unsigned char*)(BCM_3250_UPG+LED_STATUS))
#define ucLED_Debounce (*(volatile unsigned char*)(BCM_3250_UPG+LED_DEBOUNCE))
#define ucLED_Swr 	(*(volatile unsigned char*)(BCM_3250_UPG+LED_IRQEN))

/* register variables */
#define ucIRR_Status (*(volatile unsigned char *)(BCM_3250_UPG+IRR_STATUS))
#define ucIRR_Code1  (*(volatile unsigned char *)(BCM_3250_UPG+IRR_CODE1))
#define ucIRR_Code0  (*(volatile unsigned char *)(BCM_3250_UPG+IRR_CODE0))
#define ucIRR_Preamble_Hi (*(volatile unsigned char *)(BCM_3250_UPG+IRR_PREAMBLE_HI))
#define ucIRR_Preamble_Lo (*(volatile unsigned char*)(BCM_3250_UPG+IRR_PREAMBLE_LO))
#define ucIRR_DataBuf1    (*(volatile unsigned char*)(BCM_3250_UPG+IRR_DATABUF1))
#define ucIRR_DataBuf0    (*(volatile unsigned char*)(BCM_3250_UPG+IRR_DATABUF0))
#define ucIRR_Prescaler   (*(volatile unsigned char*)(BCM_3250_UPG+IRR_PRESCALER))
#define ucIRR_Timeout     (*(volatile unsigned char*)(BCM_3250_UPG+IRR_TIME_OUT))

/* Register variables */
#define uwIRB_Control (*(volatile unsigned short*)(BCM_3250_UPG+IRB_CONTROL))
#define uwIRB_Prescalar (*(volatile unsigned short*)(BCM_3250_UPG+IRB_PRESCALAR))
#define uwIRB_Repeat (*(volatile unsigned short*)(BCM_3250_UPG+IRB_REPEAT))
#define uwIRB_Carrier (*(volatile unsigned short*)(BCM_3250_UPG+IRB_CARRIER))
#define ucIRB_Sequencex (*(volatile unsigned char*)(BCM_3250_UPG+IRB_SEQUENCEX))
#define uwIRB_Ctix (*(volatile unsigned shor *)(BCM_3250_UPG+IRB_CARRIER_INDEX))

#define ucUartARxStatus (*(volatile unsigned char*)(BCM_3250_UPG+UARTA_BASE+UART_RECV_STATUS))
#define ucUartABaudRate_Hi (*(volatile unsigned char*)(BCM_3250_UPG+UARTA_BASE+UART_BAUDRATE_HI))
#define ucUartABaudRate_Lo      (*(volatile unsigned char*) (BCM_3250_UPG+UARTA_BASE+UART_BAUDRATE_LO))
#define ucUartARxControl        (*(volatile unsigned char  *) (BCM_3250_UPG+UARTA_BASE+UART_CONTROL))
#define ucUartARxData           (*(volatile unsigned char  *) (BCM_3250_UPG+UARTA_BASE+UART_RECV_DATA))

#define ucUartATxStatus         (*(volatile unsigned char  *) (BCM_3250_UPG+UARTA_BASE+UART_XMIT_STATUS))
#define ucUartATxData           (*(volatile unsigned char  *) (BCM_3250_UPG+UARTA_BASE+UART_XMIT_DATA))

#define ucUartBRxStatus         (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_RECV_STATUS))
#define ucUartBBaudRate_Hi      (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_BAUDRATE_HI))
#define ucUartBBaudRate_Lo      (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_BAUDRATE_LO))
#define ucUartBRxControl        (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_CONTROL))
#define ucUartBRxData           (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_RECV_DATA))

#define ucUartBTxStatus         (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_XMIT_STATUS))
#define ucUartBTxData           (*(volatile unsigned char  *) (BCM_3250_UPG+UARTB_BASE+UART_XMIT_DATA))

#define ucKeyPadRow32           (*(volatile unsigned char *) (BCM_3250_UPG+LED_KEYROW32))
#define ucKeyPadRow10           (*(volatile unsigned char *) (BCM_3250_UPG+LED_KEYROW10))


#define EnableKeypadReset      (ucLED_Swr = LED_RESET)
#define DisableKeypadReset     (ucLED_Swr = (~LED_RESET) & (~KEYPAD_IRQM))
#define EnableKeypadInterrupt  (ucLED_Swr = KEYPAD_IRQM)
#define DisableKeypadInterrupt  (ucLED_Swr = (~LED_RESET) & (~KEYPAD_IRQM))
#define ucUPGInterrupt      (*(volatile unsigned char *) (BCM_3250_UPG+INT_STATUS))
#define ucUPGIntEnable      (*(volatile unsigned char *) (BCM_3250_UPG+INT_ENABLE))


#endif /* __BCM93725_MAP_H */
