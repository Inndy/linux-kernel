/******************************************************************************
** Broadcom Corp. Confidential
** Copyright 1998-2000 Broadcom Corp.  All Rights Reserved.
**
** THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED 
** SOFTWARE LICENSE AGREEMENT  BETWEEN THE USER AND BROADCOM.  
** YOU HAVE NO RIGHT TO USE OR EXPLOIT THIS MATERIAL EXCEPT 
** SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
**
** File:         mpegaddress.h
** Description:  Register Definitions for Mpeg Video hardware block
** Author:       B.A. Ghanashyam ( shyam@broadcom.com )
** Version:      1.00                                                 
**
******************************************************************************/

#ifndef _MPEGADDRESS_H
#define _MPEGADDRESS_H

/* Base address values. */
 
#define RB_TMC_BASE					0x000 /* TMC   */
#define RB_TMC_MSA_DATA_BASE		0x010 /* RB_TMC_BASE + 0x10	*/
#define RB_TVR_BASE					0x028 /* TVR   */
#define RB_TTR_BASE					0x038 /* TTR   */
#define RB_TVP1_BASE				0x040 /* TVP1  */
#define RB_TVP2_BASE				0x048 /* TVP2  */
#define RB_TGH_BASE					0x050 /* TGH   */
#define RB_MISC_BASE				0x060 /* MISC  */
#define RB_TS1_BASE					0x080 /* TS    */
#define RB_TS2_BASE					0x0C0 /* TS    */
#define RB_MISC2_BASE				0x0F8 /* MISC2 */

/* Memory controller Registers */

#define RB_TMC_CNTRL_CONFIG_REG		(RB_TMC_BASE + 0x0)	
#define RB_TMC_MODE_CONTROL_REG		(RB_TMC_BASE + 0x1)
#define RB_TMC_TR1_REG				(RB_TMC_BASE + 0x2)
#define RB_TMC_TR2_REG				(RB_TMC_BASE + 0x3)
#define RB_TMC_TR3_REG				(RB_TMC_BASE + 0x4)
#define RB_TMC_CLIENT_INDEX_REG		(RB_TMC_BASE + 0x5)
#define RB_TMC_INDX_DATA_WR_REG		(RB_TMC_BASE + 0x6)
#define RB_TMC_INDX_DATA_RD_REG		(RB_TMC_BASE + 0x7)
#define RB_TMC_NMBX_REG				(RB_TMC_BASE + 0x8)
#define RB_TMC_DELAY_REG			(RB_TMC_BASE + 0x9)
#define RB_TMC_ADDRMAP_REG0			(RB_TMC_BASE + 0xA)
#define RB_TMC_ADDRMAP_REG1			(RB_TMC_BASE + 0xB)
#define RB_TMC_ADDRMAP_REG2			(RB_TMC_BASE + 0xC)
#define RB_TMC_ADDRMAP_REG3			(RB_TMC_BASE + 0xD)
#define RB_TMC_MSA_MODE_REG			(RB_TMC_BASE + 0xE)
#define RB_TMC_MSA_STATUS_REG		(RB_TMC_BASE + 0xF)

/* MSA Data Write Addresses */

#define RB_TMC_MSA_DATA0_REG		(RB_TMC_MSA_DATA_BASE + 0x0) 
#define RB_TMC_MSA_DATA1_REG		(RB_TMC_MSA_DATA_BASE + 0x1) 
#define RB_TMC_MSA_DATA2_REG		(RB_TMC_MSA_DATA_BASE + 0x2)
#define RB_TMC_MSA_DATA3_REG		(RB_TMC_MSA_DATA_BASE + 0x3) 
#define RB_TMC_MSA_DATA4_REG		(RB_TMC_MSA_DATA_BASE + 0x4) 
#define RB_TMC_MSA_DATA5_REG		(RB_TMC_MSA_DATA_BASE + 0x5) 
#define RB_TMC_MSA_DATA6_REG		(RB_TMC_MSA_DATA_BASE + 0x6) 
#define RB_TMC_MSA_DATA7_REG		(RB_TMC_MSA_DATA_BASE + 0x7) 

#define RB_TMC_MSA_DALL_REG			(RB_TMC_BASE + 0x18)
#define RB_TMC_MSA_READ_CNTR_REG	(RB_TMC_BASE + 0x19)
#define RB_TMC_MSA_WRITE_CNTR_REG	(RB_TMC_BASE + 0x1A)

#define RB_TMC_SNP_MAX_TOL_REG		(RB_MISC2_BASE+0x0)
#define RB_TMC_SNP_CNTR_REG			(RB_MISC2_BASE+0x1)
#define RB_TMC_SNP_DL_MISS_CNTR_REG	(RB_MISC2_BASE+0x2)
#define RB_TMC_SNP_CNTL_REG			(RB_MISC2_BASE+0x3)

/*--------------------------- ------------------------------- */

/*BCM7020A Video RISC control processor registers */

#define RB_TVRCP_INDEX_ADRS_REG		(RB_TVR_BASE + 0x0)
#define RB_TVRCP_INDEX_DATA_REG		(RB_TVR_BASE + 0x1)
#define RB_TVRCP_INDEX_CNTL_REG		(RB_TVR_BASE + 0x2)
#define RB_SOFT_RESET_APPLY_REG1	(RB_TVR_BASE + 0x3)  
#define RB_SOFT_RESET_WITHDRAW_REG1	(RB_TVR_BASE + 0x4) 
#define RB_SOFT_RESET_APPLY_REG2	(RB_TVR_BASE + 0x5) 
#define RB_SOFT_RESET_WITHDRAW_REG2	(RB_TVR_BASE + 0x6) 

/* BCM7020A Transport RISC  processor registers */

#define RB_TTRCP_INDEX_ADRS_REG		(RB_TTR_BASE + 0x0)
#define RB_TTRCP_INDEX_DATA_REG		(RB_TTR_BASE + 0x1)
#define RB_TTRCP_INDEX_CNTL_REG		(RB_TTR_BASE + 0x2)
#define RB_LPCR_COUNT_LOW_REG		(RB_TTR_BASE + 0x3)
#define RB_LPCR_COUNT_HIGH_REG		(RB_TTR_BASE + 0x4)

/* TVPCORE index register offsets
#define RB_TVPC_INDEX_ADRS_OFS		3'b000
#define RB_TVPC_INDEX_DATAL_OFS		3'b001
#define RB_TVPC_INDEX_CNTL_OFS		3'b010
#define RB_TVPC_INDEX_DATAH_OFS		3'b011
*/

/* BCM7020A Row decoder_1 registers */

#define RB_TVPC1_INDEX_ADRS_REG		(RB_TVP1_BASE + 0x0) 	
#define RB_TVPC1_INDEX_DATAL_REG	(RB_TVP1_BASE + 0x1)
#define RB_TVPC1_INDEX_CNTL_REG		(RB_TVP1_BASE + 0x2) 	
#define RB_TVPC1_INDEX_DATAH_REG	(RB_TVP1_BASE + 0x3) 	

/* BCM7020A Row decoder_2 registers */

#define RB_TVPC2_INDEX_ADRS_REG		(RB_TVP2_BASE + 0x0) 
#define RB_TVPC2_INDEX_DATAL_REG	(RB_TVP2_BASE + 0x1)
#define RB_TVPC2_INDEX_CNTL_REG		(RB_TVP2_BASE + 0x2)
#define RB_TVPC2_INDEX_DATAH_REG	(RB_TVP2_BASE + 0x3)

/* BCM7020A generic host interface registers */

#define RB_TGH_INTR_STATUS_REG		(RB_TGH_BASE + 0x0) /* RO */
#define RB_TGH_INTR_MASK_REG		(RB_TGH_BASE + 0x1) /* RW */
#define RB_TGH_PENDING_INTR_REG		(RB_TGH_BASE + 0x2) /* RO */
#define RB_TGH_CLR_INTR_REG			(RB_TGH_BASE + 0x0) /* WO */
#define RB_TGH_API_REG				(RB_TGH_BASE + 0x2) /* WO */
#define RB_TGH_TPFEED_DATA_REG      (RB_TGH_BASE + 0x3) /* RW */
#define RB_TGH_BYTESWAP_REG      	(RB_TGH_BASE + 0x4) /* RW */

/* These registers are there in TVRISC */

#define RB_TGH_BT_CTR_REG			(RB_TGH_BASE + 0x7)
#define RB_TGH_BT_ADRS_REG			(RB_TGH_BASE + 0x8)


/* BCM7020A MISCLANIOUS REGISTER BUS register defines */

#define RB_TVRCP_SEMA21_REG			(RB_MISC_BASE + 0x0)	/* RSWC */
#define RB_TVRCP_SEMA22_REG			(RB_MISC_BASE + 0x1)	/* RSWC */
#define RB_TVRCP_SEMA23_REG			(RB_MISC_BASE + 0x2)	/* RSWC */
#define RB_TVRCP_SEMA24_REG			(RB_MISC_BASE + 0x3)	/* RSWC */
#define RB_TVRCP_SEMA_SET_REG		(RB_MISC_BASE + 0x4)	/* RW */
#define RB_TVRCP_SEMA_CLR_REG		(RB_MISC_BASE + 0x5)	/* RW */
#define RB_TVRCP_RBLOCK_SET_REG		(RB_MISC_BASE + 0x6)	/* RW */
#define RB_TVRCP_RBLOCK_CLR_REG		(RB_MISC_BASE + 0x7)	/* RW */
#define RB_HSTSLV_DATAH_REG			(RB_MISC_BASE + 0x8)	/* WO */
#define RB_HSTSLV_DATAL_REG			(RB_MISC_BASE + 0x9)	/* WO */
#define RB_HSTSLV_CNTL_REG			(RB_MISC_BASE + 0xA)	/* WO */
#define RB_TESTPORT_CNTL_REG		(RB_MISC_BASE + 0xB)	/* RW */

/* Scanout Channel 1 registers  */

#define RB_TS1_Y0_ADDR_ORH_REG		(RB_TS1_BASE + 0x00)
#define RB_TS1_Y0_ADDR_ORL_REG		(RB_TS1_BASE + 0x01)
#define RB_TS1_Y1_ADDR_ORH_REG		(RB_TS1_BASE + 0x02) 
#define RB_TS1_Y1_ADDR_ORL_REG		(RB_TS1_BASE + 0x03)
#define RB_TS1_Y2_ADDR_ORH_REG		(RB_TS1_BASE + 0x04)
#define RB_TS1_Y2_ADDR_ORL_REG		(RB_TS1_BASE + 0x05)
#define RB_TS1_Y3_ADDR_ORH_REG		(RB_TS1_BASE + 0x06)
#define RB_TS1_Y3_ADDR_ORL_REG		(RB_TS1_BASE + 0x07)
#define RB_TS1_C0_ADDR_ORH_REG		(RB_TS1_BASE + 0x08)
#define RB_TS1_C0_ADDR_ORL_REG		(RB_TS1_BASE + 0x09) 
#define RB_TS1_C1_ADDR_ORH_REG		(RB_TS1_BASE + 0x0a)
#define RB_TS1_C1_ADDR_ORL_REG		(RB_TS1_BASE + 0x0b) 
#define RB_TS1_C2_ADDR_ORH_REG		(RB_TS1_BASE + 0x0c)
#define RB_TS1_C2_ADDR_ORL_REG		(RB_TS1_BASE + 0x0d) 
#define RB_TS1_C3_ADDR_ORH_REG		(RB_TS1_BASE + 0x0e)
#define RB_TS1_C3_ADDR_ORL_REG		(RB_TS1_BASE + 0x0f) 
#define RB_TS1_BASE0_REG	        (RB_TS1_BASE + 0x10)
#define RB_TS1_BASE1_REG	        (RB_TS1_BASE + 0x11) 
#define RB_TS1_BASE2_REG	        (RB_TS1_BASE + 0x12)
#define RB_TS1_BASE3_REG	        (RB_TS1_BASE + 0x13) 
#define RB_TS1_BUFID_OR_REG	      	(RB_TS1_BASE + 0x14)
#define RB_TS1_X_TX_MAX_REG	      	(RB_TS1_BASE + 0x15) 
#define RB_TS1_TX_TILE_OFS_REG	  	(RB_TS1_BASE + 0x16)
#define RB_TS1_C_COEF_TOP_OR_REG    (RB_TS1_BASE + 0x17) 
#define RB_TS1_C_COEF_BOT_OR_REG    (RB_TS1_BASE + 0x18)
#define RB_TS1_C_FILT_CONFIG_OR_REG (RB_TS1_BASE + 0x19)
#define	RB_TS1_H_V_PANSCAN_OFS_REG  (RB_TS1_BASE + 0x1a) 
#define	RB_TS1_DISPLAY_STATUS       (RB_TS1_BASE + 0x1b)
#define	RB_TS1_LINE_COUNT			(RB_TS1_BASE + 0x1c) 
#define	RB_TS_TX1_TX2_MAX		    (RB_TS1_BASE + 0x1d)
#define	RB_TS_TX3_TX4_MAX		    (RB_TS1_BASE + 0x1e)

/* Scanout Channel 2 registers */

#define RB_TS2_Y0_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x00) 
#define RB_TS2_Y0_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x01)
#define RB_TS2_Y1_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x02)
#define RB_TS2_Y1_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x03)
#define RB_TS2_Y2_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x04)
#define RB_TS2_Y2_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x05)
#define RB_TS2_Y3_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x06)
#define RB_TS2_Y3_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x07)
#define RB_TS2_C0_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x08)
#define RB_TS2_C0_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x09)
#define RB_TS2_C1_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x0a)
#define RB_TS2_C1_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x0b)
#define RB_TS2_C2_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x0c) 
#define RB_TS2_C2_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x0d)
#define RB_TS2_C3_ADDR_ORH_REG	  	(RB_TS2_BASE + 0x0e) 
#define RB_TS2_C3_ADDR_ORL_REG	  	(RB_TS2_BASE + 0x0f)
#define RB_TS2_BASE0_REG	        (RB_TS2_BASE + 0x10) 
#define RB_TS2_BASE1_REG	        (RB_TS2_BASE + 0x11)
#define RB_TS2_BASE2_REG	        (RB_TS2_BASE + 0x12) 
#define RB_TS2_BASE3_REG	        (RB_TS2_BASE + 0x13)
#define RB_TS2_BUFID_OR_REG	      	(RB_TS2_BASE + 0x14) 
#define RB_TS2_X_TX_MAX_REG	      	(RB_TS2_BASE + 0x15)
#define RB_TS2_TX_TILE_OFS_REG	  	(RB_TS2_BASE + 0x16) 
#define RB_TS2_C_COEF_TOP_OR_REG    (RB_TS2_BASE + 0x17)
#define RB_TS2_C_COEF_BOT_OR_REG    (RB_TS2_BASE + 0x18)
#define RB_TS2_C_FILT_CONFIG_OR_REG (RB_TS2_BASE + 0x19)
#define	RB_TS2_H_V_PANSCAN_OFS_REG  (RB_TS2_BASE + 0x1a)
#define	RB_TS2_DISPLAY_STATUS       (RB_TS2_BASE + 0x1b)
#define	RB_TS2_LINE_COUNT	  		(RB_TS2_BASE + 0x1c)

/* Interrupt position defines */
 
#define RB_Tout_Int_POS			0x0
#define TVR_Int_POS           	0x1
#define TVR_Host_API1_Int_POS 	0x2
#define TVR_Host_API2_Int_POS 	0x3
#define TS_SOF1_Int_POS       	0x4
#define TS_SOL1_Int_POS       	0x5
#define TS_SOSL1_Int_POS      	0x6
#define TS_SOF2_Int_POS       	0x7
#define TS_SOL2_Int_POS       	0x8
#define TS_SOSL2_Int_POS      	0x9
#define TTR_Host_API1_Int_POS 	0xa
#define TTR_Host_API2_Int_POS 	0xb
#define TTR_Int_POS           	0xc
#define HOST_FEED_Int_POS       0xd
#define TMB_Err_Int_POS       	0xd
#define Spare1_Int_POS        	0xe
#define Spare2_Int_POS        	0xf

/* Reset Position defines */

#define TVRperi_Rst_POS         0
#define TTRperi_Rst_POS         1
#define TPDperi_Rst_POS         2
#define TTR_TPF1_Rst_POS        3
#define TTR_TPF2_Rst_POS        4
#define TTR_TPF3_Rst_POS        5
#define TTR_27MH_Rst_POS        6
#define RE1_TVPC_HMQ_Rst_POS    7
#define RE2_TVPC_HMQ_Rst_POS    8
#define RE1_TVPCperi_Rst_POS    9
#define RE2_TVPCperi_Rst_POS    10
#define TMC_Rst_POS             11
#define TMC_SDRM_Rst_POS        12
#define TS_Rst_POS              13
#define TS1_Rst_POS             13
#define TRB_Rst_POS             14
#define Master_Rst_POS          15
 
#define TVRcore_Rst_POS         16
#define TTRcore_Rst_POS         17
#define RE1_TVPcore_Rst_POS     18
#define RE2_TVPcore_Rst_POS     19
#define TS2_Rst_POS             20

/* Host MSA Client Id for Memory Controller accesses */
#define MEM_SWC_TMC_MSA_ID		0x1E

/* Processor Ram Sizes */
#define	 TVR_CODE_MEM_SIZE		8192
#define	 TVR_DATA_MEM_SIZE		2048

#define	 TTR_CODE_MEM_SIZE		4096
#define	 TTR_DATA_MEM_SIZE		1024

#define	 TVP_CODE_MEM_SIZE		2048
#define	 TVP_DATA_MEM_SIZE		512

#endif /* _MPEGADDRESS_H */

