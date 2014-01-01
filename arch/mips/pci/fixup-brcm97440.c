/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Bcm97440 Board specific pci fixups.
 *
 * Copyright 2004-2005 Broadcom Corp.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Revision Log
 * who      when    what
 * tht      083006  Created
 */
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci_ids.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <asm/io.h>


#include <asm/brcmstb/brcm97440a0/boardmap.h>
#include <asm/brcmstb/brcm97440a0/bchp_hif_cpu_intr1.h>
#include <asm/brcmstb/brcm97440a0/bcmintrnum.h>


//#define DEBUG 0
#undef DEBUG

//#define USE_SECONDARY_SATA  // TUrn on 2nd channel, and turn on DMA for 2nd channel
//#define USE_7038B0_WAR		// Will no longer needed with B1 rev.
//#define USE_TURN_OFF_SATA2_WAR  // when not defined, turn on 2nd channel only, no DMA


/* from PCI spec, Maybe we can put this in some include file. */
#define PCI_ENABLE              0x80000000
#define PCI_IDSEL(x)		(((x)&0x1f)<<11)
#define PCI_FNCSEL(x)		(((x)&0x7)<<8)

#define BCM7400_SATA_VID		0x02421166

#define BCM7400_1394_VID		0x8101104c



/* For now until the BSP defines them */
/* For SATA MMIO BAR5 */

#define PCI_SATA_PHYS_MEM_WIN5_SIZE 0x2000

// SATA mem is now on different window, so expansion starts where the OLD Bx 7041 was
#define PCI_EXPANSION_PHYS_MEM_WIN0_BASE 0xd1000000


#define PCI_EXPANSION_PHYS_IO_WIN0_BASE 0x400

#define CPU2PCI_PCI_SATA_PHYS_MEM_WIN0_BASE  0x10510000
#define CPU2PCI_PCI_SATA_PHYS_IO_WIN0_BASE   0x10520000

#define SATA_IO_BASE	KSEG1ADDR(CPU2PCI_PCI_SATA_PHYS_IO_WIN0_BASE)
#define SATA_MEM_BASE	KSEG1ADDR(CPU2PCI_PCI_SATA_PHYS_MEM_WIN0_BASE)



static char irq_tab_brcm97440a0[] __initdata = {
//[slot]  = IRQ
  [PCI_DEVICE_ID_SATA] = BCM_LINUX_SATA_IRQ,    /* SATA controller */

  [PCI_DEVICE_ID_EXT]  = BCM_LINUX_EXT_PCI_IRQ, /* On-board PCI slot */
  //[PCI_DEVICE_ID_MINI] = BCM_LINUX_MINI_PCI_IRQ,					    
  //[PCI_DEVICE_ID_1394] = BCM_LINUX_1394_IRQ,                 
};


int __init pcibios_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
#ifdef DEBUG    
    printk("pcibios_map_irq: slot %d pin %d IRQ %d\n", slot, pin, irq_tab_brcm97401a0[slot]);
#endif	
    return irq_tab_brcm97440a0[slot];
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}



// global to allow multiple slots
static u32 memAddr = PCI_EXPANSION_PHYS_MEM_WIN0_BASE;
static u32 ioAddr = PCI_EXPANSION_PHYS_IO_WIN0_BASE;

// Remembers which devices have already been assigned.
static  int slots[32];

/* Do IO/memory allocation for expansion slot here */
static void brcm_pcibios_fixup_plugnplay(struct pci_dev *dev) 
{
	int i,j,k,l;
	volatile unsigned int iosize, msize;
	//int useIOAddr = 0;
	u32 savRegData;
	u32 size;
	u32 mask;
	int slot;
	u32 regData;
		

	/*
	* Make sure that it is in the PCI slot, otherwise we punt.
	*/

	if (dev->bus->number != 0)
		return;


	slot = PCI_SLOT(dev->devfn);

	#if 0
	/* Only handle devices in the PCI slot */
	if ( !(PCI_DEVICE_ID_EXT == slot || PCI_DEVICE_ID_EXT2 == slot)) {
		printk("\tDev ID %04x:%04x ignored\n", dev->vendor, dev->device);
		return;
	}
	

	if (slots[slot]) {
		printk("Skip resource assignment for dev ID %04x:%04x already allocated\n", dev->vendor, dev->device);
		return;
	}

	slots[slot] = 1;
#endif

	printk("\tPCI DEV in slot %x, ID=%04x:%04x\n\r", slot, dev->vendor, dev->device);
	

	/* Write FFFFFFFFH to BAR registers to probe for IO or Mem */
	iosize = msize = 0;
	for (i=0,j=0; i<6; i++,j+=4) {
		pci_write_config_dword(dev,PCI_BASE_ADDRESS_0+j,0xffffffff);
		pci_read_config_dword(dev,PCI_BASE_ADDRESS_0+j,&regData);
		if (regData) {
			printk("PCI PnP: PCI_BAR[%d] = %x\n", i, regData);


			savRegData = regData;
			regData &= 0xfffffff0; /* Get rid of Bit 0-3 0=Mem, 2:1 32/64, 3=
Prefetchable */
			/*
			* determine io/mem size here by looking at the BAR
			* register, skipping the first 4 bits
			*/
			for (k=4; k<32; k++) {
				if (regData & (1<<k)) {
					break;
				}
			}
			if (k < 32) {
				printk("PCI PnP: size requested is %x\n", 1<<k);
				size = 1<<k;
				mask = 0xffffffff;
				for (l=0; l<k; l++) {
					mask &= ~(1<<l);
				}
			}
			else
				break;

			if (savRegData & 0x1) {	/* IO address */
				/* Calculate the next address that satisfies the boundary condition */
				regData = (ioAddr + size - 1) & mask;
				dev->resource[j].start = regData;
				dev->resource[j].end = regData + size - 1;
				if (insert_resource(&ioport_resource, &dev->resource[j])) {
					printk("PnP: Cannot allocate IO resource[%d]=(%04x,%04x)\n", j, regData, regData + size - 1);
				}
				pci_write_config_dword(dev, PCI_BASE_ADDRESS_0+j, regData);
				printk("PnP: Writing PCI_IO_BAR[%d]=%x, size=%d, mask=%x\n", i,
					regData, 1<<k, mask);
				iosize = regData + size;
				ioAddr = regData + size; // Advance it
			}
			else { /* Mem address, tag on to the last one, the 7041 mem area */
				/* Calculate the next address that satisfies the boundary condition */
				regData = (memAddr + size - 1) & ~(size -1);
				dev->resource[j].start = regData;
				dev->resource[j].end = regData + size - 1;
				if (insert_resource(&iomem_resource, &dev->resource[j])) {
					printk("PnP: Cannot allocate MEM resource[%d]=(%08x,%08x)\n", j, regData, regData + size - 1);
				}
				pci_write_config_dword(dev, PCI_BASE_ADDRESS_0+j, regData);
				printk("PnP: Writing PCI_MEM_BAR[%d]=%x, size=%d, mask=%x\n", i,
					regData, 1<<k, mask);
				msize = regData + size;
				memAddr = regData+size; // Advance it
			}
		}
	}
	/*  now that it's all configured, turn on the Command Register */
	pci_read_config_dword(dev,PCI_COMMAND,&regData);
	regData |= PCI_COMMAND_SERR|PCI_COMMAND_PARITY |PCI_COMMAND_MASTER;
	if (iosize > 0)
		regData |= PCI_COMMAND_IO;
	if (msize > 0)
		regData |= PCI_COMMAND_MEMORY;

	printk("\tPnP: PCI_DEV %04x:%04x, command=%x, msize=%08x, iosize=%08x\n", 
		dev->vendor, dev->device, regData, msize, iosize);
	pci_write_config_dword(dev,PCI_COMMAND,regData);

	
}

/*
** Statics used for MDIO register access
*/
#define WRITE_CMD           1
#define READ_CMD            2
#define CMD_DONE            (1 << 15)
#define MDIO_PORT_SELECT    0x07
#define MDIO_PORT_RX_LO     0x08    /* Contains Signal Detect Treshold bits 11:13
                                       and Signal Detect Enable bit 10 */
#define MDIO_PORT_TX_CTL    0x0a    /* Contains Transmit Driver current bits 12:15
                                       and Pre-emphasis percentage bits 6:8
                                       and Transmit pre-emphasis mode bit 4 */

/*
** 1. port is SATA port ( 0 or 1)
** 2. reg is the address of the MDIO register ( see spec )
** 3. Access via SATA_MEM_BASE
*/
static u16 mdio_read_reg( int port, int reg)
{
	volatile unsigned long *mdio = (volatile unsigned long  *)((SATA_MEM_BASE + 0x8c));
	volatile unsigned long pSel  = 1 << port;
	uint32_t cmd  = WRITE_CMD;

	if( reg > 0x13 )
		return( -1 );

 	/*Select Port*/
	*mdio = pSel<<16 | (cmd << 13) | MDIO_PORT_SELECT;
	while( !(*mdio & CMD_DONE) )
		udelay(1);

	*mdio = (READ_CMD << 13) + reg;
	while( !(*mdio & CMD_DONE) )
		udelay(1);

	return( *mdio >> 16 );
}

static void mdio_write_reg(int port, int reg, u16 val )
{
	volatile unsigned long *mdio = (volatile unsigned long  *)((SATA_MEM_BASE + 0x8c));
	volatile unsigned long pSel  = 1 << port;
	uint32_t cmd  = WRITE_CMD;
    
	if( reg > 0x13 )
		return;

 	/*Select Port*/
	*mdio = pSel<<16 | (cmd << 13) | MDIO_PORT_SELECT;
	while( !(*mdio & CMD_DONE) )
		udelay(1);

	*mdio = (val << 16) + (WRITE_CMD << 13) + reg;
	while( !(*mdio & CMD_DONE) )
		udelay(1);
}


	/********************************************************
	 * SATA Controller Fix-ups
	 ********************************************************/
#define CHIPREV_7038B0 0x00000510

static void brcm_pcibios_fixup_SATA(struct pci_dev *dev)
{
	u32 dev_vendor_id;
	u32 regData;
	u32 size;

			
	regData = 0x200;
	dev->resource[0].start = regData;
	dev->resource[0].end = regData + 0x3f;
	//dev->resource[0].flags = IORESOURCE_IO;
	if (insert_resource(&ioport_resource, &dev->resource[0])) {
		//printk("fixup: Cannot allocate resource 0\n");
	}
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, regData);	
	
	regData = 0x240;
	dev->resource[1].start = regData;
	dev->resource[1].end = regData + 0x3f;
	//dev->resource[1].flags = IORESOURCE_IO;
	if (insert_resource(&ioport_resource, &dev->resource[1])) {
		printk("fixup: Cannot allocate resource 1\n");
	}
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_1, regData);
	
	regData = 0x280;
	dev->resource[2].start = regData;
	dev->resource[2].end = regData + 0x3f;
	//dev->resource[2].flags = IORESOURCE_IO;
	if (insert_resource(&ioport_resource, &dev->resource[2])) {
		printk("fixup: Cannot allocate resource 2\n");
	}
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_2, regData);
	
	regData = 0x2c0;
	dev->resource[3].start = regData;
	dev->resource[3].end = regData + 0x3f;
	//dev->resource[3].flags = IORESOURCE_IO;
	if (insert_resource(&ioport_resource, &dev->resource[3])) {
		printk("fixup: Cannot allocate resource 3\n");
	}
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_3, regData);
	
	regData = 0x300;
	dev->resource[4].start = regData;
	dev->resource[4].end = regData + 0x3f;
	//dev->resource[4].flags = IORESOURCE_IO;	
	if (insert_resource(&ioport_resource, &dev->resource[4])) {
		printk("fixup: Cannot allocate resource 4\n");
	}
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_4, regData);

	/* On 2.4.25, we have to assign some mem region for MMIO even though we
	 * have no intention of using it
	 */
	size = PCI_SATA_PHYS_MEM_WIN5_SIZE;
	//regData = (CPU2PCI_PCI_SATA_PHYS_MEM_WIN0_BASE + size -1) & ~(size-1); 
	regData = KSEG1ADDR((CPU2PCI_PCI_SATA_PHYS_MEM_WIN0_BASE + size -1) & ~(size-1));
	dev->resource[5].start = regData;
	dev->resource[5].end = regData +size-1;
	//dev->resource[5].flags = IORESOURCE_MEM;
	if (insert_resource(&iomem_resource, &dev->resource[5])) {
		//printk("fixup: Cannot allocate resource 5\n");
	}
	pci_write_config_dword(dev, PCI_BASE_ADDRESS_5, regData);
		
	regData = 0x0f;
	pci_write_config_byte(dev, 0x3e, regData); /* Minimum Grant, to avoid Latency being reset to lower value */
	
	regData = 0xff;
	pci_write_config_byte(dev, 0xd, regData); /* SATA_CFG_MASTER_LATENCY_TIMER. */


	pci_read_config_dword(dev, PCI_COMMAND, &regData);
	regData |= (PCI_COMMAND_SERR|PCI_COMMAND_PARITY|PCI_COMMAND_MASTER|PCI_COMMAND_IO| PCI_COMMAND_MEMORY);
	pci_write_config_dword(dev, PCI_COMMAND, regData);


	/* Primary */
	pci_read_config_dword(dev, PCI_BASE_ADDRESS_4, &regData);
	regData &= 0xfffffc; // Mask off  Reserved & Res Type bits.
	printk("SATA: Primary Bus Master Status Register offset = %08lx + %08x = %08lx\n", 
			SATA_IO_BASE, regData, SATA_IO_BASE+regData);

	regData += 2; // Offset 302H for primary
	printk("SATA: before init Primary Bus Master Status reg = 0x%08x.\n", *((volatile unsigned char *)(SATA_IO_BASE+regData)));
	*((volatile unsigned char *)(SATA_IO_BASE+regData)) |= 0x20; // Both Prim and Sec DMA Capable
	printk("SATA: after init Primary Bus Master Status reg = 0x%08x.\n", *((volatile unsigned char *)(SATA_IO_BASE+regData)));


//	if (use_secondary_sata) 
	{
		/* Secondary */
		pci_read_config_dword(dev, PCI_BASE_ADDRESS_4, &regData);
		regData &= 0xfffffc; // Mask off  Reserved & Res Type bits.
		printk("SATA: Secondary Bus Master Status Register offset = %08lx + %08x = %08x\n", 
				SATA_IO_BASE, regData, SATA_IO_BASE+regData);

		regData += 0xa; // Offset 30AH for secondary
		printk("SATA: before init Secondary Bus Master Status reg = 0x%08x.\n", *((volatile unsigned char *)(SATA_IO_BASE+regData)));
		*((volatile unsigned char *)(SATA_IO_BASE+regData)) |= 0x60; // Both Prim and Sec DMA Capable
		printk("SATA: after init Secondary Bus Master Status reg = 0x%08x.\n", *((volatile unsigned char *)(SATA_IO_BASE+regData)));			
	}



#if 1
	/*
	** Andover: as per Ajay: Good for all existing and future controllers
	** Enable ECO fix to force SYNCs on the SATA interface
	** between PIO data transfer of the packet CDB and DMA mode transfer.
	**
	** THT: However, this is NOT the place to turn this on, as doing it here
	** would hang the SATA controller when we try to disable the PHY and reset
	** the drive with MDIO later on.
	*/
	*((volatile unsigned long *)(SATA_MEM_BASE+0x88))  |= 0x20000000;
	*((volatile unsigned long *)(SATA_MEM_BASE+0x188)) |= 0x20000000;
	printk("SATA: SIMR = 0x%08lx/0x%08lx after ECO fix enable\n",
		*((volatile unsigned long *)(SATA_MEM_BASE+0x88)),
		*((volatile unsigned long *)(SATA_MEM_BASE+0x188)));

	/*
	** Check MDIO Settings
	*/
	{
		u16 rxlo[2];
		u16 txctl[2];

		rxlo[0]  = mdio_read_reg(0, MDIO_PORT_RX_LO);
		rxlo[1]  = mdio_read_reg(1, MDIO_PORT_RX_LO);
		txctl[0] = mdio_read_reg(0, MDIO_PORT_TX_CTL);
		txctl[1] = mdio_read_reg(1, MDIO_PORT_TX_CTL);

		/*
		** Set Rx Signal Detect threshold
		** Bits <12:11>:
		**  00b - 80 mV  (0x0000)
		**  01b - 100 mV (0x0800)
		**  10b - 120 mV (0x1000)
		**  11b - 140 mV (0x1800)
		*/
		if ((rxlo[0] & 0x1800) != 0x1800 || (rxlo[1] & 0x1800) != 0x1800) {
			/* Set RxLo Signal Detect Threshold to 140 mV */
			printk("SATA: Set Port 0/1 RxLo Signal Detect to 140 mV\n");
			rxlo[0] |= 0x1800;
			rxlo[1] |= 0x1800;
			mdio_write_reg(0, MDIO_PORT_RX_LO, rxlo[0]);
			mdio_write_reg(1, MDIO_PORT_RX_LO, rxlo[1]);
		}

		/*
		** Set Transmit Driver current to 500mV and
		** pre-emphasis to 10%.
		**
		** Bits <15:12> are Transmit Driver Current:
		**  0000b - 140mV (0x0000)
		**  0001b - 190mV (0x1000)
		**  0010b - 230mV (0x2000)
		**  0011b - 270mV (0x3000)
		**  0100b - 320mV (0x4000)
		**  0101b - 360mV (0x5000)
		**  0110b - 400mV (0x6000)
		**  0111b - 430mV (0x7000)
		**  1000b - 460mV (0x8000)
		**  1001b - 500mV (0x9000 - default)
		**  1010b - 540mV (0xa000)
		**  1011b - 580mV (0xb000)
		**  1100b - 620mV (0xc000)
		**  1101b - 650mV (0xd000)
		**  1110b - 690mV (0xe000)
		**  1111b - 720mV (0xf000)
		**
		** Bits <8:6> are Pre-Emphasis percentage:
		**  000b - 0%  (0x0000 - default)
		**  001b - 5%  (0x0040)
		**  010b - 10% (0x0080)
		**  011b - 15% (0x00c0)
		**  100b - 20% (0x0100)
		**  101b - 25% (0x0140)
		**  110b - 30% (0x0180)
		**  111b - 35% (0x01c0)
		*/
		if ((txctl[0] & 0x9080) != 0x9080 || (txctl[1] & 0x9080) != 0x9080) {
			/* Set RxLo Signal Detect Threshold to 140 mV */
			printk("SATA: Set Port 0/1 TxCtl to 500 mV/10%s pre-emphasis\n", "%");
			txctl[0] |= 0x9080;
			txctl[1] |= 0x9080;
			mdio_write_reg(0, MDIO_PORT_TX_CTL, txctl[0]);
			mdio_write_reg(1, MDIO_PORT_TX_CTL, txctl[1]);
		}

		rxlo[0]  = mdio_read_reg(0, MDIO_PORT_RX_LO);
		rxlo[1]  = mdio_read_reg(1, MDIO_PORT_RX_LO);
		txctl[0] = mdio_read_reg(0, MDIO_PORT_TX_CTL);
		txctl[1] = mdio_read_reg(1, MDIO_PORT_TX_CTL);

		printk("SATA: MDIO_PORT_RX_LO  = 0x%04x/0x%04x\n", rxlo[0],  rxlo[1]);
		printk("SATA: MDIO_PORT_TX_CTL = 0x%04x/0x%04x\n", txctl[0], txctl[1]);

    }
#endif

#if 0
	printk("Turning on SIMR and SERR\n");
	// Turn on SATA error checking, for development debugging only.  Turn this off in production codes.
	*((volatile unsigned long *)(SATA_MEM_BASE+0x88)) |=  0xffffffff & ~(0x94050000);
	printk("SATA: SIMR = 0x%08lx.\n", *((volatile unsigned long *)(SATA_MEM_BASE+0x88)));
	printk("SATA: SCR1 = 0x%08lx.\n", *((volatile unsigned long *)(SATA_MEM_BASE+0x44)));
#endif
}




//#define PCI_VENDOR_ID_BCM7041 (BCM7041_VID&0xffff)
//#define PCI_DEVICE_ID_BCM7041 (BCM7041_VID>>16)

/* 
 * SATA is on the secondary bus, and requires special handling.
 */

DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_SERVERWORKS, PCI_DEVICE_ID_SERVERWORKS_BCM7038,
	 	brcm_pcibios_fixup_SATA);


DECLARE_PCI_FIXUP_HEADER(PCI_ANY_ID, PCI_ANY_ID,
	 	brcm_pcibios_fixup_plugnplay);


//#endif CONFIG_PCI_AUTO



/* --------------------------------------------------------------------------
    Name: DumpRegs
Abstract: Dump the IDE registers
 -------------------------------------------------------------------------- */

void
dump_sata(void)
{

# if 0
	struct pci_dev *dev = find_pci_device();	
	volatile unsigned long mmioBase = pci_resource_start(dev, 5);
	u16 cmd, status;
	u32 sataIntStatus;

//	extern int gDbgWhere;
    

//	printk("ide_do_request: %d\n", gDbgWhere);
	printk("7038B0 SATA: Status Reg = %08x\n", *((volatile unsigned long *) (mmioBase+0x1C)));
	printk("7038B0 SATA: Port0 Master Status = %08x\n", *((volatile unsigned long *) (mmioBase+0x30)));
	printk("7038B0 SATA: Port0 SATA Status = %08x\n", *((volatile unsigned long *)  (mmioBase+0x40)));
	printk("7038B0 SATA: Port0 SATA Error = %08x\n", *((volatile unsigned long *)  (mmioBase+0x44)));
#if 0
       pci_find_device(BCM7038_SATA_VID>>16, BCM7038_SATA_VID&0xffff) {

	
	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	pci_read_config_word(dev, PCI_STATUS, &status);
	pci_read_config_dword(dev, 0x80, &sataIntStatus);
#endif
#endif

}
EXPORT_SYMBOL(dump_sata);




