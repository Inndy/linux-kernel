/*
 *  arch/mips/brcm/brcmpci.c -- Broadcom BCM93730 pci setup routines
 *
 *  Copyright (C) 2000 Troy Trammel <ttrammel@broadcom.com>
 *					Broadcom Corp.
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
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include "asm/brcm/bcm93730/ali.h"
#include "asm/brcm/bcm93730/pci.h"

static int brcm_pci_read_config_dword( struct pci_dev *dev,
				int where, u32 *val)
{
    int slot_num, func_num;
    u32 cfc,cf8;

    /*
     *  For starters let's do configuration cycle 0 only (one bus only)
     */
    if (dev->bus->number)
	return PCIBIOS_FUNC_NOT_SUPPORTED;

    slot_num = PCI_SLOT(dev->devfn);
    func_num = PCI_FUNC(dev->devfn);
	cf8 = PCI_ENABLE + PCI_IDSEL(slot_num) + PCI_FNCSEL(func_num) + where;

	*(volatile u32*)(BCM_7020_PCI_EXT_CF8_NOCACHE) = cf8;
	cfc = *(volatile u32*)(BCM_7020_PCI_EXT_CFC_NOCACHE);
    *val = cfc;
    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_write_config_dword(struct pci_dev *dev, int where,
					u32 val)
{
    int slot_num, func_num;
    u32 cfc;

    /*
     *  For starters let's do configuration cycle 0 only (one bus only)
     */
    if (dev->bus->number)
	return PCIBIOS_FUNC_NOT_SUPPORTED;

    slot_num = PCI_SLOT(dev->devfn);
    func_num = PCI_FUNC(dev->devfn);

	cfc = PCI_ENABLE + PCI_IDSEL(slot_num) + PCI_FNCSEL(func_num) + where;
	*(volatile u32*)(BCM_7020_PCI_EXT_CF8_NOCACHE) = cfc;
	*(volatile u32*)(BCM_7020_PCI_EXT_CFC_NOCACHE) = val;

    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_read_config_word(struct pci_dev *dev, int where, u16 *val)
{
    int status;
    u32 result;

    status = brcm_pci_read_config_dword(dev, where, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 2)
	result >>= 16;
    *val = result & 0xffff;
    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_read_config_byte(struct pci_dev *dev, int where, u8 *val)
{
    int status;
    u32 result;

    status = brcm_pci_read_config_dword(dev, where, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 1)
	result >>= 8;
    if (where & 2)
	result >>= 16;
    *val = result & 0xff;
    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_write_config_word(struct pci_dev *dev, int where, u16 val)
{
    int status, shift = 0;
    u32 result;

    status = brcm_pci_read_config_dword(dev, where, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 2)
	shift += 16;
    result &= ~(0xffff << shift);
    result |= val << shift;
    return brcm_pci_write_config_dword(dev, where, result);
}

static int brcm_pci_write_config_byte( struct pci_dev *dev, int where, u8 val)
{
    int status, shift = 0;
    u32 result;

    status = brcm_pci_read_config_dword(dev, where, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 2)
	shift += 16;
    if (where & 1)
	shift += 8;
    result &= ~(0xff << shift);
    result |= val << shift;
    return brcm_pci_write_config_dword(dev, where, result);
}

struct pci_ops brcm_pci_ops = {
    brcm_pci_read_config_byte,
    brcm_pci_read_config_word,
    brcm_pci_read_config_dword,
    brcm_pci_write_config_byte,
    brcm_pci_write_config_word,
    brcm_pci_write_config_dword
};

/*  This stuff is messed up, we will change it when I understand what it is. TDT */
struct {
    struct resource ram;
    struct resource flash;
    struct resource pci_io;
    struct resource pci_mem;
    struct resource bcm7020;
    struct resource boot;
} bcm93725_resources = {
    { "RAM", 0x00000000, 0x03ffffff,
      IORESOURCE_MEM | PCI_BASE_ADDRESS_MEM_TYPE_64 },
    { "Flash ROM", 0x1e000000, 0x1f000000 },
    { "PCI I/O", 0x06100000, 0x07ffffff },
    { "PCI mem", 0x09000000, 0x0fffffff, IORESOURCE_MEM },
    { "7020 ctrl", 0x1fa00000, 0x1fbfffff,
      IORESOURCE_MEM | PCI_BASE_ADDRESS_MEM_TYPE_64 },
    { "Boot ROM", 0x1fc00000, 0x1fffffff }
};

/*
 * We enable all devices during setup as the ALi must be setup in order
 * to access devices which use it's PIC.
 */
static void __init pcibios_enable_devices(void)
{
    struct pci_dev *dev;
    pci_for_each_dev(dev) {
		/* <TDT> Our enable_device sets up bios data for the bootrom does not */
		brcm_pcibios_enable_device(dev);

		/* <TDT> force a rescan in case we still need bios data */
		pci_setup_device(dev);
    }
}

/*
 * Our current boot rom does not set these properly, so we change
 * them during setup.
 */
static void __init pcibios_fixup_irqs(void)
{
	struct pci_dev *dev;
	int slot_num;

	pci_for_each_dev(dev) {
		slot_num = PCI_SLOT(dev->devfn);
		switch (slot_num) {
		case BCM3250_IDSEL:		/* BCM3250  */
			pcibios_update_irq(dev, 17);
			dev->irq = 17;
			break;
		case ALI_IDE_IDSEL:	/* IDE controller */
			pcibios_update_irq(dev, 14);
			dev->irq = 14;
			break;
#if 0
		case AMD_IDSEL:		/* On-board ethernet */
			pcibios_update_irq(dev, 6);
			dev->irq = 6;
			break;
#endif
		case BCM4412_IDSEL:	/* On-board ethernet */
			pcibios_update_irq(dev, 6);
			dev->irq = 6;
			break;
		case SLOTA_IDSEL:	/* On-board PCI slot */
			pcibios_update_irq(dev, 15);
			dev->irq = 15;
			break;
		}
	}
}

void __init pcibios_init(void)
{
	printk("PCI: Probing PCI hardware\n");
	pci_scan_bus(0, &brcm_pci_ops, NULL);
	pcibios_enable_devices();
	pcibios_fixup_irqs();
}


static char *brcm_pcibios_setup (char *str)
{
    return str;
}

void __init pcibios_update_irq(struct pci_dev *dev, int irq)
{
    u8 val;

	pci_write_config_byte(dev, PCI_INTERRUPT_LINE, irq);

	/* Test this as it does not seem to be working */
	pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &val);
}


int pcibios_enable_resources(struct pci_dev *dev)
{
	u16 cmd, old_cmd;
	int idx;
	struct resource *r;
	printk("PCI: Enabling device %x\n", dev->device);

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	old_cmd = cmd;
	for(idx=0; idx<6; idx++) {
		r = &dev->resource[idx];
		if (!r->start && r->end) {
			printk(KERN_ERR "PCI: Device %s not available because of resource collisions\n", dev->slot_name);
			printk("r->start=%lx, r->end=%lx\n",r->start,r->end);
			return -EINVAL;
		}
		if (r->flags & IORESOURCE_IO)
			cmd |= PCI_COMMAND_IO;
		if (r->flags & IORESOURCE_MEM)
			cmd |= PCI_COMMAND_MEMORY;
	}
	if (cmd != old_cmd) {
		printk("PCI: Enabling device %s (%04x -> %04x)\n", dev->slot_name, old_cmd, cmd);
		pci_write_config_word(dev, PCI_COMMAND, cmd);
	}
	return 0;
}

static int brcm_pcibios_enable_device(struct pci_dev *dev)
{
	u32 dev_vendor_id;
	u32 regData;

	/********************************************************
	 * Configure the Device
	 ********************************************************/
	dev_vendor_id = (u32)dev->device<<16;
	dev_vendor_id |= dev->vendor;
	switch (dev_vendor_id) {
	/********************************************************
	 * Using the ALi ISA Bridge
	 ********************************************************/
	case ALI_ISA_VID:
		printk("\tAli ISA\n\r");
		/************************************************
		 * ALi Register 0x40,0x41,0x42,0x43
		 ************************************************
		 *
		 * disable IRQ1 to KB: 0x41=0x47; enable: 0x41=0xc7
		 *
		 */
#define KBD_INT
   		regData = 
			  (0x30 << 0)	/*  0x40: PCI Interface Control  */
#ifdef	KBD_INT		/*  ap: using keyboard interrupts */
			| (0xc7 << 8)	/*  0x41: I/O Recovery Control */
#else
			| (0x47 << 8)	/*  0x41: I/O Recovery Control */
#endif
			| (0x08 << 16)	/*  0x42: ISA Bus Cycle Control I */
			| (0xea << 24);	/*  0x43: ISA Bus Cycle Control II */
		pci_write_config_dword(dev,0x40,regData);

		/************************************************
		 * ALi Register 0x44,0x45,0x46,0x47
		 ************************************************
		 *
		 * TDT - irq 14 for the ide
		 *
		 */
  		regData = 
			  (0x5D <<  0)	/*  0x44: IDE Native Mode Interrupt Routing I */
			| (0x00 <<  8)	/*  0x45: PCI Interrupt Polling Mode */
			| (0x00 << 16)	/*  0x46: Reserved */
			| (0x01 << 24);	/*  0x47: BIOS Chip Select Control */
		pci_write_config_dword(dev,0x44,regData);

		/************************************************
		 * ALi Register 0x48,0x49,0x4a,0x4b
		 *	0x48: PCI Interrupt to ISA Routing Table I
		 *		bit[7-4] : INTB to ISA Routing
		 *		bit[3-0] : INTA to ISA Routing
		 *	0x49: PCI Interrupt to ISA Routing Table I
		 *		bit[7-4] : INTD to ISA Routing
		 *		bit[3-0] : INTC to ISA Routing
		 *	0x4a: PCI Interrupt to ISA Routing Table I
		 *		bit[7-4] : INT6 (Poll) to ISA Routing
		 *		bit[3-0] : INT5 (Poll) to ISA Routing
		 *	0x4b: PCI Interrupt to ISA Routing Table I
		 *		bit[7-4] : INT8 (Poll) to ISA Routing
		 *		bit[3-0] : INT7 (Poll) to ISA Routing
		 ************************************************/
		regData = 
			  /* (ALI_IRQ_INTA <<  0)	// 0x48  */
			  (15 <<  0)	/*  0x48  */
			| (0x00 <<  8)	/*  0x49  */
			| (0x00 << 16)	/*  0x4a  */
			| (0x00 << 24);	/*  0x4b  */

		pci_write_config_dword(dev,0x48,regData);
		/************************************************
		 * ALi Register 0x4c,0x4d,0x4e,0x4f
		 *	0x4c: PCI Interrupt Level to Edge
		 *		bit[7-0] : INT8-INT1 0= Level to 8259
		 *		Level is the default setting
		 *	0x4d-0x4f: Reserved 
		 ************************************************/
		regData = 
			  (0xff <<  0)	/*  0x4c = Level to 8259  */
			| (0x00 <<  8)	/*  0x4d  */
			| (0x00 << 16)	/*  0x4e  */
			| (0x00 << 24);	/*  0x4f  */
		pci_write_config_dword(dev,0x48,regData);

		/************************************************
		 * ALi Register 0x50,0x51,0x52,0x53
		 *	0x50: I/O Cycle Posted Write 1st Port Def
		 *	0x52: I/O Cycle Posted Write 2nd Port Def
		 ************************************************
		 *
		 *leave as USB disabled 0x52=0x4000, enabled 0x52=0
		 */
		regData = 
			  (0x0000 <<  0)	/* 0x50 */
			| (0x0000 << 16);	/* 0x52 */
		pci_write_config_dword(dev,0x50,regData);

		/************************************************
		 * ALi Register 0x54,0x55,0x56,0x57
		 * 0x54: Hardware Setting Status Bits
		 * 0x55: Programmable Chip Select
		 ************************************************/
		regData = 
			  (0x00 <<  0)		/* 0x54 */
			| (0x000291 << 8);	/* 0x55 */
   	
		pci_write_config_dword(dev,0x54,regData);

		/************************************************
		 * ALi Register 0x58,0x59,0x5a,0x5b
		 ************************************************
		 *
		 * 0x58: bit6=1 Enable IDE controller
		 * 0x58: bit2=1 Enable IDE ATA Primary bus signals 
		 */
		regData = 
			  (0x44 << 0)	/* 0x58: IDE Interface Control */
			| (0x00 << 8)		/* 0x59: GPI Multiplexed Pin Select */
			| (0x0004 << 16);	/* 0x5a: GPO Multiplexed Pin Select */
		pci_write_config_dword(dev,0x58,regData);

		/************************************************
		 * ALi Register 0x70,0x71,0x72,0x73
		 ************************************************/
		regData =
			 (0x12 <<  0)	/* 0x70: Serial IRQ Control Register */
			|(0x00 <<  8)	/* 0x71: Distributed DMA Channel */
			|(0x00 << 16)	/* 0x72: USB IDSEL Mux Select */
			|(0x00 << 24);	/* 0x73: Distributed DMA Base Address */
		pci_write_config_dword(dev,0x70, regData);

		/************************************************
		 * ALi Register 0x74,0x75,0x76,0x77
		 ************************************************
		 *
		 * don't enable secondary IDE or SCI
		 */
		regData =
			 (0x59 <<  0)	/* 0x74: USB Interrupt Routing Table use IRQ[11] */
			|(0x00 <<  8)	/* 0x75: IDE Native Mode Interrupt Routing II */
			|(0x80 << 16) 	/* 0x76: PMU System Control Interrupt Routing */
			|(0x00 << 24);	/* 0x77: SMB Controller Event Interrupt Routing */
		pci_write_config_dword(dev,0x74,regData);

		regData = 0x00000000;
		pci_write_config_dword(dev,0x6c,regData);
		regData = 0x08000000;
		pci_write_config_dword(dev,0x6c,regData);
#if 0
		/* dump final configuration of the south bridge */
		{
		int i;
		printk("\t\tConfig Space Data");
		for (i=0;i<0x60;i++) {
		  	if ((i & 7) == 0)
			  	printk("\r\n\t\t");
		  	pci_read_config_byte(dev,i,(unsigned char *)&regData);
			printk("%02x ",(unsigned char) regData & 0xff);
		}
		printk("\r\n");
		}
#endif
		/***********************************************
	  	 * Configure ALI Parallel Port.
	 	 ***********************************************/
	outb(ALI_CONFIG_KEY_BEG0, oALI_CONFIG_PORT);
	outb(ALI_CONFIG_KEY_BEG1, oALI_CONFIG_PORT);
	outb(ALI_CONFIG_SEL_DEV, oALI_CONFIG_PORT);

		/***********************************************   
		 * Select Device 3 : Parallel Port 
		 * comments below include address(default)
		 ***********************************************/
	outb(0x03, oALI_DATA_PORT);

		/***********************************************   
		 * 0x30(0x00) : Enable parallel port
		 *      Bit7-1: Reserved
		 *      Bit0  : 0=disable, 1=enable
		 ***********************************************/
	outb(0x30, oALI_CONFIG_PORT);
	outb(0x01, oALI_DATA_PORT); 

		/***********************************************   
		 * Read parallel port address   
		 *      0x60(0x03) : hi addr of PP I/O base addr
		 *      0x61(0x78) : lo addr of PP I/O base addr
		 ***********************************************/
	outb(0x60, oALI_CONFIG_PORT);
	regData = inb(oALI_DATA_PORT);          

	outb(0x61, oALI_CONFIG_PORT);
	regData = inb(oALI_DATA_PORT);

		/***********************************************   
		 * 0x70(0x05) : Select IRQ channel used by PP
		 *      1 = IRQ[1]
		 *      3 = IRQ[3]
		 *      4 = IRQ[4]
		 *      5 = IRQ[5]
		 *      6 = IRQ[6]
		 *      7 = IRQ[7]
		 *      9 = IRQ[9]
		 *      10 = IRQ[10]
		 *      11 = IRQ[11]
		 *      12 = IRQ[12]
		 *      14 = IRQ[14]
		 *      15 = IRQ[15]
		 ***********************************************/
	outb(0x70, oALI_CONFIG_PORT);
	outb(ALI_IRQ_PARALLEL, oALI_DATA_PORT );


		/***********************************************   
		 * 0x74(0x04) : Select DMA channel used by PP
		 *      0 = DMA0
		 *      1 = DMA1
		 *      2 = DMA2
		 *      3 = DMA3
		 *      4 = none
		 ***********************************************/
	outb(0x74, oALI_CONFIG_PORT);
	outb(0x04, oALI_DATA_PORT); 

		/***********************************************   
		 * 0xF0(0x8c) : Select Mode PP
		 *      Bit7 : IRQ polarity, 1= active low
		 *      Bit6-3: ECP FIFO Threshold, default=1
		 *      Bit2-0: PP Mode
		 *              0 = PS2
		 *              1 = EPP 1.9
		 *              2 = ECP
		 *              3 = ECP+EPP 1.9
		 *              4 = SPP (default)
		 *              5 = EPP 1.7
		 *              6 = reserved
		 *              7 = ECP + EPP 1.7
		 *
		 ***********************************************/
	outb(0xf0, oALI_CONFIG_PORT);
	outb(0x88, oALI_DATA_PORT); 

		/***********************************************   
		 * 0xF1(0x85) : Select Mode PP
		 *      Bit7 : 1= force PP into extended mode 
		 *      Bit6-3: reserved 
		 *      Bit2 : PP clock, 0=24MHz, 1=12MHz 
		 *      Bit1 : EPP timeout interrupt, 1=enable
		 *      Bit0 : 1= Burst DMA mode in ECP
		 ***********************************************/
	outb(0xf1, oALI_CONFIG_PORT);
	outb(0x85, oALI_DATA_PORT); 

		/***********************************************   
		 *  Config Complete
		 ***********************************************/
	outb(ALI_CONFIG_KEY_END, oALI_CONFIG_PORT);

		/***********************************************   
	 	 *  Parallel Status
		 ***********************************************/
	regData = inb(oALI_PARALLEL_STATUS);

		/***********************************************
	 	 * Configure ALI keyboard.
		 ***********************************************/
	outb(ALI_CONFIG_KEY_BEG0, oALI_CONFIG_PORT);
	outb(ALI_CONFIG_KEY_BEG1, oALI_CONFIG_PORT);

		/************************************************** 
		 * Select Device 7              
		 **************************************************/
	outb(ALI_CONFIG_SEL_DEV, oALI_CONFIG_PORT );
	outb(0x07, oALI_DATA_PORT);          

			/****************************************** 
			 * Read keyboard type           
			 * Set the Keyboard to 12Meg clock
			 ******************************************/
	outb(0xf0, oALI_CONFIG_PORT);        
	outb(0x10, oALI_DATA_PORT);        

			/****************************************** 
			 * Read keyboard interrupt      
			 ******************************************/
	outb(0x70, oALI_CONFIG_PORT);
	regData = inb(oALI_DATA_PORT);
/* ap: not required, since keyboard intpt maps to IRQ[1] by default */
/*	outb(oALI_DATA_PORT, ALI_IRQ_KEYBOARD);		 */

		/****************************************** 
		 * Configuration Complete
		 ******************************************/
	outb(ALI_CONFIG_KEY_END, oALI_CONFIG_PORT);


		/*
		 * From M5042 : Masked Controller Spec
		 * COMMAND REGISTER BIT DEFINITION
		 *      Bit7 - Reserved
		 *      Bit6 - IBM PC Compatible Mode
		 *              Write 1 to get controller to 
		 *              convert scan code to IBMPC
		 *      Bit5 - IBMPC Mode
		 *              Write 1 to get control not to 
		 *              check parity or convert scan codes.
		 *      Bit4 - Disable Keyboard
		 *              Write 1 to disable keyboard IF.
		 *      Bit3 - Inhibit Override
		 *              Write 1 to disable the Keyboard Inhibit.
		 *      Bit2 - System Flag 
		 *              Sets Status reg system flag.
		 *      Bit1 - Reserved
		 *      Bit0 - Output Buffer Full Interrupt
		 *              Writing 1 causes an interrupt
		 *              to be generated when the
		 *              controller writes to output buf.
		 *
		 * STATUS REGISTER BIT DEFINITIONS
		 * 	Bit7 = Parity Error, 
		 *		0=last byte had odd parity 
		 *		1 =last byte had even parity
		 *	Bit6 = Receive TimeOut, 
		 *		1= keyboard started xmit but didn't finish
		 *	Bit5 = Transmit Time-Out,
		 *		1 = keyboard started xmit but was not complete
		 *	Bit4 = Inhibit Switch, 0=inhibit, 1=not inhibit
		 *	Bit3 = Command/Data Byte Select
		 *		0 = data byte, 1 = command byte
		 *	Bit2 = System Flag, 0 during power on
		 *	Bit1 = Input Buffer Full,
		 *		0 = keyboard controller input buffer empty
		 *		1 = data written into buffer
		 *	Bit0 = Output Buffer Full,
		 * 		0 = keyboard controller output buffer empty
		 *		1 = data placed in output buffer
		 *		Resets on Read of Status Register.
		 */
   	outb(0x20, 0x0064);
		/*
		 * Write to TX causes the Status Register
		 * Command/Data bit=0 and InputBufferFull bit=1
		 */
   	outb(0xAA, 0x0060);
	/**********************************************************
 	 *  Initialise ALI internal 8259 Interrupt Controller
	 **********************************************************
	 * bit[7-5] reserved
	 * bit[4] : must be 1
	 * bit[3] : 1 = level triggered. 
	 * bit[2] : reserved
	 * bit[1] : 0 = Cascade Controllers
	 * bit[0] : 1 = ICW4 needed
	 */
	outb(0x19, oICW1_M);	

		/* bit[7-3] : Interrupt Vector Address */
		/* bit[2-0] : reserved */
   	outb(0x08, oICW2_M);	

		/* Must be programmed to 0x04 indicating  */
		/* slave output is connected to master IRQ[2] */
   	outb(0x04, oICW3_M);	

		/* bit[7-5] : must be 0 */
		/* bit[4]   :  0 = not specially fully nested  */
		/* bit[3-2] : 0* = non buffered  */
		/* bit[1]   : 1  = auto EOI  */
		/* bit[0]   : must be 1 (80x86 mode) */
	//THT PR5279: AUtoEOI = False, was writing 0x03
	outb(0x01, oICW4_M);	


		/* bit[7-5] reserved */
		/* bit[4] : must be 1 */
		/* bit[3] : 1 = level triggered.  */
		/* bit[2] : reserved */
		/* bit[1] : 0 = Cascade Controllers */
		/* bit[0] : 1 = ICW4 needed */
	outb(0x19, oICW1_S);	

		/* bit[7-3] : Interrupt Vector Address */
		/* bit[2-0] : reserved */
   	outb(0x70, oICW2_S);	

		/* Must be programmed to 0x02 indicating  */
		/* slave output is connected to master IRQ[2] */
   	outb(0x02, oICW3_S);	

		/*  bit[7-5] : must be 0 */
		/*  bit[4]   : 0  = not specially fully nested  */
		/*  bit[3-2] : 0* = non buffered  */
		/*  bit[1]   : 1  = auto EOI  */
		/*  bit[0]   : must be 1 (80x86 mode) */
	//THT PR5279: AUtoEOI = False, was writing 0x03
	outb(0x01, oICW4_S);	

		/*  Disable all 8259 interrupts */
		/*  bit[7-0] : 1 = Set IRQ Mask */
	outb(0xff, oOCW1_S);	
	outb(0xff, oOCW1_M);	

		/*  Clear the 8259 ISR */
		/*  bit[7-5] : Non Specific EOI Command */
		/*  bit[4-3] : must be 0 */
		/*  bit[2-0] : Interrupt Level Select */
	outb(0x20, oOCW2_S);	
	outb(0x20, oOCW2_M);	

		/*  Set Defaults  */
		/*  bit[7] : reserved must be 0 */
		/*  bit[6-5] : No action */
		/*  bit[4-3] : Mubs be 01 */
		/*  bit[2] : 0=No Poll Command */
		/*  bit[1-0] : 0* = No Action */
   	outb(0x08, oOCW3_S);	
   	outb(0x08, oOCW3_M);	

		/*  ALi Keyboard is at default IRQ1 */
		/*  OAA USB uses edge interrupt */
/*    	outb(ELCR(ALI_IRQ_USB),ALI_IRQ_BIT(ALI_IRQ_USB));	 */
/*    	outb(ELCR(ALI_IRQ_INTA),ALI_IRQ_BIT(ALI_IRQ_INTA));	 */

   	outb(ALI_IRQ_BIT(ALI_IRQ_INTA),oELCR_S);	

	/* Go ahead and enable the 8259 Cascade interrupts now. */
	/*	NOTE: Interrupts should NOT be enabled here.  */
	/* This is done, later, at driver initialisation time. */
	outb(0xfb, oOCW1_M); 
	outb(0xfd, oOCW1_S);	
	break;
	/********************************************************
	 * Using the ALi PMU Controller
	 ********************************************************/
	case ALI_PMU_VID:
		printk("\tALi PMU\n\r");
		break;
	/********************************************************
	 * Using the ALi IDE Controller
	 ********************************************************/
	case ALI_IDE_VID:
		printk("\tAli IDE\n\r");
   		  /* turn on IDE Native Mode */
		pci_read_config_dword(dev,8, &regData);
		regData |= 0xFF00; /* enable native mode */
		pci_write_config_dword(dev,8,regData);

		  /* enable IDE (duplicate with 58 of SouthBridge) */
		pci_read_config_dword(dev,0x50, &regData);
		regData |= 1;
		pci_write_config_dword(dev,0x50,regData);

		  /* setup the Latency timer */
		pci_write_config_dword(dev,0x0c,0x00002000);

		  /* primary channel data read/write timing register */
		pci_write_config_dword(dev,0x58,0x00003142);

		  /* turn off FIFOs */
		pci_write_config_dword(dev,0x54,0x44441111);

#ifdef USE_ALI_IDE_NATIVE_offset
		  /* program the BARs */
		pci_write_config_dword(dev,0x10,0x7000); /* BAR0  away from ISA */
		pci_write_config_dword(dev,0x14,0x0000); /* BAR1 */
		pci_write_config_dword(dev,0x18,0x0000); /* BAR2 */
		pci_write_config_dword(dev,0x1C,0x0000); /* BAR3 */
		pci_write_config_dword(dev,0x20,0x0000); /* BAR4 */

#endif

		  /* lastly, turn on I/O access */
		pci_read_config_dword(dev,0x04,&regData);
		regData |= 5;
		pci_write_config_dword(dev,0x04,regData);

#if 0
		{
		unsigned long rd,i;
		printk("\t\tConfig Space Data");
		for (i=0;i<0x60;i++) {
			if ((i & 7) == 4)
				  printk("\t");
			if ((i & 7) == 0)
				  printk("\r\n\t\t");
			pci_read_config_byte(dev,i,(unsigned char *)&rd);
			printk("%02x ",(UCHAR)rd&0xff);
		}
		printk("\r\n");
		}
#endif

#if 0
		printk("Reading ALi IDE %x=%x\n\r",
			(QSR_PCI_IO_ADDR+0x1f1),
			*(UCHAR*)(QSR_PCI_IO_ADDR + 0x1f0)
			);	
#endif
//#endif

		break;
	/********************************************************
	 * Using the ALi USB Controller
	 ********************************************************/
	case ALI_USB_VID:
		/*
		 *   set memory window
		 *   BAR0  away from ISA
		 */ 
		pci_write_config_dword(dev,0x10,0xffffffff); 
		pci_write_config_dword(dev,0x10,ALI_USB_MEM_BASE); 

		/*
		 *   Set Interrupt
		 */  
		pci_read_config_dword(dev,0x3c,&regData);
		regData |= ALI_IRQ_USB;
		pci_write_config_dword(dev,0x3c,regData);

		/*
		 *  enable memory access
		 *  0x04: Command reg - enable Memory and I/O accesses
		 */  
		pci_read_config_dword(dev,0x04, &regData);
		regData |= (0x06     << 0);	
		pci_write_config_dword(dev,0x04,regData);

#ifdef DUMP_PCI_CONFIG
		printk("\tALi USB config space (IDSEL%d)"),PCI_SLOT(dev->devfn));
	  for (i=0;i<0x40;i++) {
		  if ((i & 3) == 0)
			  printk("\r\n\t\t");
		pci_read_config_byte(dev,i,(unsigned char *)&rd);
			  printk("[%.2x]=%.2x  "),i,rd & 0xFF);
		  }
		  printk("\n\r");
#endif

		break;

	/********************************************************
	 * Using the PCI Ide Card as controller
	 ********************************************************/
	case SIIG_IDE_VID:
#ifdef USE_SIIG_IDE
	printk("\tSIIG IDE\n\r");
		/* clear BAR0,1,3 & 4 */
		pci_write_config_dword(dev,0x10,0);
		pci_write_config_dword(dev,0x14,0);
		pci_write_config_dword(dev,0x1c,0);
		pci_write_config_dword(dev,0x20,0);

		/* program BAR2 to 7000 I/O */
		pci_write_config_dword(dev,0x18,0);

		/* now that it's all configured, turn on the Command Register */
		regData = pci_read_config_dword(dev,0x04);
		regData |= 0x145;
		pci_write_config_dword(dev,0x04,regData);
#if 0
		printk("\t\tConfig Space Data");
		for (i=0;i<0x40;i++) {
			if ((i & 7) == 4)
				  printk("\t");
			if ((i & 7) == 0)
				  printk("\r\n\t\t");
			pci_read_config_byte(dev,i,(unsigned char *)&rd);
			printk("%02x ",(UCHAR)rd&0xff);
		}
		printk("\r\n");
#endif
#endif	
		break;
	/********************************************************
	 * Epigram HPNA/Enet BCM4413 
 	 ********************************************************/
	case BCM4410_VID:
	case BCM4411_VID:
	case BCM4412_VID:
#if 1
   	{
    		int slot;
		int irq;
		u8 latency;

    		slot = PCI_SLOT(dev->devfn);

    		printk("PCI detected BCM4410/11/12 HPNA/Ethernet controller ID=%x, slot=%d\n",
			dev_vendor_id, slot);

		/*
		 *   program BAR0 to Memory
		 */


   		if (BCM4410_IDSEL == slot) { /* On board 4413: Disable it */
			printk(KERN_INFO "Onboard 4413 function=%d\n", PCI_FUNC(dev->devfn));
			regData = BCM4410_PCI_MEM_BASE;
			irq = ALI_IRQ_ENET;
			// Enable these 3 lines to disable the onboard 4413.
			//printk("PCI 4413 onboard disabled function=%d\n", PCI_FUNC(dev->devfn));
			//pci_write_config_dword(dev,0x04,0);
                        //break; /* Out of case */
   		}
		else if (SLOTA_IDSEL == slot) {
			printk(KERN_INFO "PCI 4413 function=%d\n", PCI_FUNC(dev->devfn));
			regData = BCM7041_MEM_BASE+BCM7041_MEM_SIZE;
			irq = ALI_IRQ_INTA;
   		}

		if (2 != PCI_FUNC(dev->devfn)) {
	    		printk(KERN_INFO "PCI 4413 disabled function=%d\n", PCI_FUNC(dev->devfn));
			pci_write_config_dword(dev,0x04,0);
                        break; /* Out of case */
   		}
		pci_write_config_dword(dev,0x10,regData);

#if 0
/* This seems to precipitate the "out-of-request" slot problem */
		/*
		 * See if we can program the latency byte
		 */
		if (PCIBIOS_SUCCESSFUL == brcm_pci_read_config_byte(dev,0x0d,&latency)) {
			printk(KERN_INFO "----Latency timer at 0x0D offset = %x\n", latency);

			/*
			 * if latency is 0, it is programmable, so
			 * write some latency to allow the turtle some more time
			 */
			if (0 == latency) {
				brcm_pci_write_config_byte(dev, 0x0d, 0x3f);
				brcm_pci_read_config_dword(dev,0x0C,&regData);
				printk(KERN_INFO "++++Latency timer at 0x0D offset after setting = %x\n", regData);
    			}
			//brcm_pci_read_config_dword(dev,0x0C,&regData);
			//printk("++++Latency timer at 0x0D offset after setting = %x\n", regData);
		}
#endif

		/*
		 *   program Interrupt to the ALi PIC
		 *   NDIS wants the offset from SYSINTR_FIRMWARE
		 *   The Driver reads this register for the
		 *   value to pass to NdisMRegisterInterrupt()
		 */
		  //regData=ALI_IRQ_HPNA;
		  regData= irq;
		  pci_write_config_dword(dev,0x3c,regData);

		/*
		 * now that it's all configured, 
		 * turn on the Command Register
		 */
		regData = pci_write_config_dword(dev,0x04,0);
		/*
		 * Epigram is not a bus master - THT not true with the 4413.
		 * Set to MEMORY access
		 */
		pci_read_config_dword(dev,0x04,&regData);
		regData |= 6; // Was 2 
		printk(KERN_INFO "++++ Writing command register[0x4] = %x\n", regData);
		regData = pci_write_config_dword(dev,0x04,regData);
        }

#else /* Original codes */
		/*
		 *   program BAR0 to Memory
		 */
		regData = BCM4410_PCI_MEM_BASE;
		pci_write_config_dword(dev,0x10,regData);
		/*
		 *   program Interrupt to the ALi PIC
		 *   NDIS wants the offset from SYSINTR_FIRMWARE
		 *   The Driver reads this register for the
		 *   value to pass to NdisMRegisterInterrupt()
		 */
		  //regData=ALI_IRQ_HPNA;
		  regData= ALI_IRQ_ENET;
		  pci_write_config_dword(dev,0x3c,regData);

		/*
		 * now that it's all configured, 
		 * turn on the Command Register
		 */
		regData = pci_write_config_dword(dev,0x04,0);
		/*
		 * Epigram is not a bus master
		 * Set to MEMORY access
		 */
		regData = 2; 
		regData = pci_write_config_dword(dev,0x04,regData);
                
#endif
#if 0
		{ int i;
		printk("\n\r\tEpigram Configuration");
		for(i=0;i<0x40;i+=4){
		if ((i & 15) == 0)
					  printk("\r\n");
		pci_read_config_dword(dev,i,&regData);
				printk("[0x%x]= %x\t",i,regData);

		}
		printk("\n");
		}
#endif 
		break;
 
	case BCM4212_VID:
		printk("PCI detected BCM4212 V.90 Softmodem\n");
		/* Write the 0x10 register to find the PCI memory size needed */
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, 0xFFFFffff);
		pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &regData);

		regData = BCM4212_PCI_MEM_BASE;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 ,regData);

		/* Enable response in I/O space */
		/* Enable response in Memory space */
		/* Enable bus mastering */
	        pci_read_config_dword(dev, PCI_COMMAND, &regData);
		regData |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
		
		/* PCI_COMMAND */
		pci_write_config_dword(dev, PCI_COMMAND, 0);
		pci_write_config_dword(dev, PCI_COMMAND, regData);

		/* Set the interrupt to 15 */
		regData=0;
		pci_read_config_dword(dev, PCI_INTERRUPT_LINE, &regData);
		regData = (regData & 0xFFFFff00) | 15;
		pci_write_config_dword(dev, PCI_INTERRUPT_LINE, regData);
		printk("Setting V.90 Softmodem to IRQ 15\n");

		/* JPF Temporary fix to enable the grant and request lines for the PCI slot */
		/* This should be done in the bootrom but doing it here avoids */
		/* needing updated bootroms */

		regData = *((volatile unsigned long *)0xb00000f0);
		printk("Current BGE_TEST_MODE setting = 0x%08lx\n", (unsigned long) regData);
		regData |= 0x08000000;
		*((volatile unsigned long *)0xb00000f0) = regData;
		regData = *((volatile unsigned long *)0xb00000f0);
		printk("Enable GNT,REQ for PCI Slot BGE_TEST_MODE setting = 0x%08lx\n", (unsigned long) regData);

	break;
 
	/********************************************************
	 * Using the Linksys Ethernet Pci card as controller
	 ********************************************************/
	case SMC_ETHERNET_VID:
	case LINKSYS_ETHERNET_VID:
		printk("\tPCI_ETHERNET\n\r");

		/*  program BAR0 to  NE2000_PCI_IO_BASE */
			/*  bus,slot,function,offset,value */
		regData = NE2000_PCI_IO_BASE;
		pci_write_config_dword(dev,0x10,regData);

		/* 
		 *  program Interrupt to ALi PIC
		 *  NDIS wants the offset from SYSINTR_FIRMWARE
		 *  The Driver reads this register for the
		 *  value to pass to NdisMRegisterInterrupt()
		 */ 
		regData= ALI_IRQ_INTA;
		pci_write_config_dword(dev,0x3c,regData);

		/*  now that it's all configured, turn on the Command Register */
		pci_read_config_dword(dev,0x04,&regData);
		regData |= 0x145;
		pci_write_config_dword(dev,0x04,regData);

#if 0
	{
		int i;
		printk("\t\tConfig Space Data");
		for (i=0;i<0x60;i++) {
			  if ((i & 7) == 0)
				  printk("\r\n\t\t");
			  pci_read_config_byte(dev,i,(unsigned char *)&regData);
			  printk("%02x ",(unsigned char)regData&0xff);
		}
		printk("\r\n");
	}
#endif
		break;
	/********************************************************
	 * Using the On-board AMD ethernet controller
	 ********************************************************/
#if 0
	case AMD_ETHERNET_VID:
		printk("\tAMD_ETHERNET\n\r");
		/* 
		 * dump the BAR
		 */

		/*  program BAR0 to  AMD_PCI_IO_BASE
		 *  bus,slot,function,offset,value
		 */
		regData = AMD_PCI_IO_BASE;
		pci_write_config_dword(dev,0x10,regData);
		/* 
		 *  program Interrupt to ALi PIC
		 *  NDIS wants the offset from SYSINTR_FIRMWARE
		 *  The Driver reads this register for the
		 *  value to pass to NdisMRegisterInterrupt()
		 */ 
		 regData= ALI_IRQ_ENET;
		 pci_write_config_dword(dev,0x3c,regData);

		/*  now that it's all configured, turn on the Command Register */
		pci_read_config_dword(dev,0x04, &regData);
		regData |= 0x145;
		pci_write_config_dword(dev,0x04,regData);

		/* OAA Program AMD's MAC address with data from Boot ROM. */
		printk("WARNING!!! This board uses temporary MAC address.\n\r");

		/* OAA Eanble APROM writes in BCR2 register */
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 0x12)) = 2;
		MacShort  = *(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 0x16));
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 0x16)) = (MacShort | 0x100);


		/* OAA Read temp. MAC addr from Boot ROM */
#if 1
		MacShort = *((unsigned short *)ROM_MAC_ADDR0);
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE)) = MacShort;
		MacShort = *((unsigned short *)(ROM_MAC_ADDR0+2));
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 2)) = MacShort;
		MacShort = *((unsigned short *)(ROM_MAC_ADDR0+4));
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 4)) = MacShort;
#else
		MacShort = 0xc0;
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE)) = MacShort;
		MacShort = 0x2b2b;
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 2)) = MacShort;
		MacShort = 0xad1b;
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 4)) = MacShort;
#endif

		/* OAA Disable APROM writes */
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 0x12)) = 2;
		MacShort  = *(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 0x16));
		*(volatile unsigned short*)(MIPS_IO_ADDR(AMD_PCI_IO_BASE + 0x16)) = (MacShort & (~0x100));

#if 0
	{
		int i;
		printk("\t\tConfig Space Data");
		for (i=0;i<0x60;i++) {
			  if ((i & 7) == 0)
				  printk("\r\n\t\t");
			  pci_read_config_byte(dev,i,(unsigned char *)&regData);
			  printk("%02x ",(unsigned char)regData&0xff);
		}
		printk("\r\n");
	}
#endif
		break;
#endif
	/********************************************************
	 * Config the BCM3250 chip
	 ********************************************************/
	case BCM3250_VID:
		printk("\tBCM3250\n\r");
		/* 
		 * dump the BAR
		 */

		break;

	/********************************************************
	 * Config the BCM7040 chip
	 ********************************************************/
	case BCM7040_VID:
		printk("\tBMC7040 MPEG2 Encoder\n\r");
		regData = BCM7040_MEM_BASE;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_1, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_2, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_3, regData);
 
		pci_read_config_dword(dev, PCI_COMMAND, &regData);
		regData |= PCI_COMMAND_MEMORY;
		pci_write_config_dword(dev, PCI_COMMAND, regData);
		break;                                                                                       
	/********************************************************
	 * Config the BCM7041 chip
	 ********************************************************/
	case BCM7041_VID:
		printk("\tBMC7041 Dual Channel MPEG2 Encoder\n\r");
		regData = BCM7041_MEM_BASE;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_1, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_2, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_3, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_4, regData);
		regData += 0x20000;
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_5, regData);
 
		pci_read_config_dword(dev, PCI_COMMAND, &regData);
		regData |= PCI_COMMAND_MEMORY;
		pci_write_config_dword(dev, PCI_COMMAND, regData);
		break;

	default:
		break;
	}
#ifdef DUMP_PCI_CONFIG
	{
	int i;
	printk("\tPCI Config Space IDSEL%d"),PCI_SLOT(dev->devfn));
	for (i=0;i<0x40;i++) {
		if ((i & 3) == 0) printk("\r\n\t\t");
		pci_read_config_byte(dev,i,(unsigned char *)&regData);
		printk("[%.2x]=%.2x  "),i,regData & 0xFF);
	}
	printk("\n\r");
	}
#endif
	return 0;
}

static void brcm_pcibios_update_resource(struct pci_dev *dev, struct resource *root,
			     struct resource *res, int resource)
{
#if 0
	u32 new, check;
	int reg;

	new = res->start | (res->flags & PCI_REGION_FLAG_MASK);
	if (resource < 6) {
		reg = PCI_BASE_ADDRESS_0 + 4*resource;
	} else if (resource == PCI_ROM_RESOURCE) {
		res->flags |= PCI_ROM_ADDRESS_ENABLE;
		reg = dev->rom_base_reg;
	} else {
		/* Somebody might have asked allocation of a non-standard resource */
		return;
	}
	
	pci_write_config_dword(dev, reg, new);
	pci_read_config_dword(dev, reg, &check);
	if ((new ^ check) & ((new & PCI_BASE_ADDRESS_SPACE_IO) ? PCI_BASE_ADDRESS_IO_MASK : PCI_BASE_ADDRESS_MEM_MASK)) {
		printk(KERN_ERR "PCI: Error while updating region "
		       "%s/%d (%08x != %08x)\n", dev->slot_name, resource,
		       new, check);
	}
#endif
}

void __init pcibios_fixup_bus(struct pci_bus *bus)
{
}

static void brcm_pcibios_align_resource(void *data, struct resource *res,
			    unsigned long size)
{
#if 0
	struct pci_dev *dev = data;

	if (res->flags & IORESOURCE_IO) {
		unsigned long start = res->start;

		/* We need to avoid collisions with `mirrored' VGA ports
		   and other strange ISA hardware, so we always want the
		   addresses kilobyte aligned.  */
		if (size > 0x100) {
			printk(KERN_ERR "PCI: I/O Region %s/%d too large"
			       " (%ld bytes)\n", dev->slot_name,
			       dev->resource - res, size);
		}

		start = (start + 1024 - 1) & ~(1024 - 1);
		res->start = start;
	}
#endif
}

unsigned __init int pcibios_assign_all_busses(void)
{
	return 1;
}

struct pci_fixup pcibios_fixups[] = {};
