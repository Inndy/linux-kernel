/************************************************
 ****   File: bcm3250.h 
 ****   BCM3250 registers definition.
 ************************************************/
#ifndef   BCM3250_HDR
#define   BCM3250_HDR 

/* BCM3250 Register Offset for Sub-Cores.       */
#define GLOBAL_REG              0x00000000      /* Global Registers             */
#define CM_US_TX                0x00000100      /* Cable Modem Upstream Tx      */
#define CM_DS_RX                0x00000200      /* Cable Modem Downstream Rx    */
#define VIDEO_DS_RX             0x00000300      /* Video Downstream Rx          */
#define PID_FILTER              0x00000400      /* PID Filter                   */
#define OUT_OF_BAND_RX          0x00000480      /* Out of Band Receiver         */
#define OUT_OF_BAND_DS_PRO      0x00000600      /* Out of Band Downstream Proc. */
#define UPG                     0x00000700      /* UPG                          */
#define DAVIC_MAC               0x00000800      /* DAVIC MAC                    */
#define DAVIC_MAC_DMA           0x00000A00      /* DAVIC MAC DMA                */
#define SDRAM_CONTROLLER        0x00000B00      /* SDRAM Controller             */
#define DOCSIS_MAC              0x00001000      /* DOCSIS MAC                   */

/* Global Register Map                          */
#define PCI_SUB_ID              0x0000002C       /* PCI Subsystem ID             */
#define PCI_SUB_VENDOR          0x0000002E       /* PCI Subsystem Vender ID      */
#define PCI_MAX_LAT             0x0000003C       /* PCI Mac_Lat                  */
#define PCI_MIN_GNT             0x0000003D       /* PCI Min_Gnt                  */
#define PCI_INT_PIN             0x0000003E       /* PCI Interrupt Pin            */
#define PCI_INT_LINE            0x0000003F       /* PCI Interrupt Line           */
#define CPU_IF_CFG              0x00000040       /* CPU Interface Config. Reg.   */
#define CPU_DEFER_SR1           0x00000044       /* CPU Interface Defer Reg.     */
#define CPU_DEFER_SR2           0x00000048       /* CPU Interface Defer Reg.     */
#define CPU_DEFER_SR3           0x0000004C       /* CPU Interface Defer Reg.     */
#define BCM3250_IRQ_PENDING	PCI_ADDR16(0x00000050)	/* Interrupt Cause Register           */
#define BCM3250_IRQ_MASK	PCI_ADDR16(0x00000052)	/* Interrupt Mask Register           */
#define		IB_IRQ_MASK		0x8000
#define		MSPI_IRQ_MASK	0x4000
#define		DMA6_IRQ_MASK	0x2000
#define		DMA5_IRQ_MASK	0x1000
#define		DMA4_IRQ_MASK	0x0800
#define		DMA3_IRQ_MASK	0x0400
#define		DMA2_IRQ_MASK	0x0200
#define		DMA1_IRQ_MASK	0x0100
#define		DMAC_IRQ_MASK	0x0080
#define		DS1_IRQ_MASK	0x0040
#define		DS2_IRQ_MASK	0x0020
#define		MAC_IRQ_MASK	0x0010
#define		OB_IRQ_MASK		0x0008
#define		UPG_IRQ_MASK	0x0004
#define		DP_IRQ_MASK		0x0002
#define		CPU_IRQ_MASK	0x0001

#define DIAG_GPO        0x00000054       /* Programmable GPO Register    */
#define TRC_CLKREF      0x00000080       /* TRC CLKREF Control Register  */
#define TRC_TRCCTL      0x00000081       /* TRCCTL Control Register      */
#define TRC_GLBCTL      0x00000082       /* TRC misc control register    */
#define TRC_TBD2        0x00000083       /* TRC resered register         */
#define MSPI_CTL0       0x000000A0       /* SPI master control 0         */
#define MSPI_CTL1       0x000000A1       /* SPI master control 1         */
#define MSPI_REGW_0     0x000000A2       /* SPI master write data 0      */
#define MSPI_REGW_1     0x000000A3       /* SPI master write data 1      */
#define MSPI_REGW_2     0x000000A4       /* SPI master write data 2      */
#define MSPI_REGW_3     0x000000A5       /* SPI master write data 3      */
#define MSPI_REGW_4     0x000000A6       /* SPI master write data 4      */
#define MSPI_REGW_5     0x000000A7       /* SPI master write data 5      */
#define MSPI_REGW_6     0x000000A8       /* SPI master write data 6      */
#define MSPI_REGW_7     0x000000A9       /* SPI master write data 7      */
#define MSPI_REGR_0     0x000000AA       /* SPI master read data 0       */
#define MSPI_REGR_1     0x000000AB       /* SPI master read data 1       */
#define MSPI_REGR_2     0x000000AC       /* SPI master read data 2       */
#define MSPI_REGR_3     0x000000AD       /* SPI master read data 3       */
#define MSPI_REGR_4     0x000000AE       /* SPI master read data 4       */
#define MSPI_REGR_5     0x000000AF       /* SPI master read data 5       */
#define MSPI_REGR_6     0x000000B0       /* SPI master read data 6       */
#define MSPI_REGR_7     0x000000B1       /* SPI master read data 7       */
#define MSPI_INT        0x000000B2       /* SPI master interrupt reg     */
#define CLK_EN          0x000000C0       /* Clock enable                 */
#define SOFT_RSTB       0x000000C2       /* Soft Reset                   */
	#define	 	MSPI_RESET	0x4000
	#define 	SSPI_RESET	0x2000
	#define 	SYS_RESET	0x1000
	#define 	CPU_RESET	0x0800
	#define 	ISB_RESET	0x0400
	#define 	LB_RESET	0x0200
	#define 	MAC_RESET	0x0100
	#define 	DMAC_RESET	0x0080
	#define 	DS1_RESET	0x0040
	#define 	DS2_RESET	0x0020
	#define 	US_RESET	0x0010
	#define 	OB_RESET	0x0008
	#define 	UPG_RESET	0x0004
	#define 	DP_RESET	0x0002
	#define 	PIDF_RESET	0x0001

#define SYSPLL_CTL      PCI_ADDR8(0x000000C5)       /* System PLL control           */
#define DIAG_EN_LO      0x000000C6       /* Diagnostic Port Selection LSB*/
#define DIAG_EN_HI      0x000000CA       /* Diagnostic Port Selection MSB*/
#define I2CSPI_CTL      0x000000CC       /* I2C/SPI control register     */
#define SYSCLK_SEL      0x000000CD       /* System clock select          */
#define DS1_CTL         0x000000CE       /* DS1 Control Register         */
#define TUNER_CTL       0x000000CF       /* Tuner Output Control         */
#define PIDF_GP         0x000000D0       /* PIDF general purpose reg.    */
#define PDIF_GBL        0x000000D3       /* PIDF global register         */
#define US_IN_SEL       0x000000D4       /* Upstream input select        */
#define EXT_SRAM        0x000000D5       /* Use the ext. deinter. RAM    */
#define DMAC_CTL        0x000000D6       /* Select DS input for DAVIC    */
#define US_BUSY         0x000000D8       /* Upstream busy status         */
#define PBUS_STATUS     0x000000F0       /* PBUS status                  */
#define PBUS_CTL        0x000000F4       /* PBUS control                 */
#define POST_ADDR       0x000000FA       /* Posted read address          */
#define POST_DATA       0x000000FC       /* Posted read data             */


#if 0
/* DOCSIS MAC Memory Map                        */
#define GLOBAL_CONTROL_STATUS   0x00000000       /* Global Control & Status */
#define IRQ_PENDING             0x00000002       /* IRQ Pending        */
#define IRQ_MASK                0x00000004       /* IRQ Mask           */
#define DOCSIS_DEVICE_ID        0x0000000A       /* Device ID          */
#define DOCSIS_DEVICE_REVISION  0x0000000C       /* Device Revision    */
#endif

/* Downstream Receiver Registers                */

#define BCM3250_DS_MB0          0x00000000       /* Multiple byte 0   */
#define BCM3250_DS_MB1          0x00000001       /* Multiple byte 1   */
#define BCM3250_DS_MB2          0x00000002       /* Multiple byte 2   */
#define BCM3250_DS_MB3          0x00000003       /* Multiple byte 3   */
#define BCM3250_DS_MBOP         0x00000004       /* Multiple byte operation  */

/* Upstream Receiver Registers                  */

/* Out Of Band Receiver Registers               */
#define BCM3250_OB_MB0          0x00000080       /* Multiple byte 0    */
#define BCM3250_OB_MB1          0x00000081       /* Multiple byte 1    */
#define BCM3250_OB_MB2          0x00000082       /* Multille byte 2    */
#define BCM3250_OB_MB3          0x00000083       /* Multiple byte 3    */
#define BCM3250_OB_MBOP         0x00000084       /* Multiple byte operation */

#endif /* BCM3250_HDR */
