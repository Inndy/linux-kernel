/*
 *
 *  drivers/mtd/brcmnand/bcm7xxx-nand.c
 *
    Copyright (c) 2005-2006 Broadcom Corporation                 
    
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

    File: bcm7xxx-nand.c

    Description: 
    This is a device driver for the Broadcom NAND flash for bcm97xxx boards.
when	who what
-----	---	----
051011	tht	codings derived from OneNand generic.c implementation.
 */
 

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>

#include <linux/mtd/partitions.h>


#include <asm/io.h>
//#include <asm/mach/flash.h>


#include "brcmnand.h"

#define DRIVER_NAME	"bcm7xxx-nand"
#define DRIVER_INFO "Broadcom STB NAND controller version 0.9"

//#ifdef CONFIG_MTD_PARTITIONS
//static const char *part_probes[] = { "cmdlinepart", NULL,  };
//#endif

/* Size and offset are variable, depending on the size of the chip, but 
 * cfe_kernel always starts at 1FC0_0000 and is 4MB size.
 */
static struct mtd_partition bcm7XXX_nand_parts[] = 
{
	{ name: "rootfs",		offset: 0,			size: 12*1024*1024 },	
	{ name: "cfe_kernel",	offset: 0x00C00000,	size: 4*1024*1024 },
	//{ name: "vmlinux",	offset: 0x00C80000, size: 3582*1024 },
	//{ name: "config",	offset: 0x00FFF800,	size: 144 },
	//{ name: "nvram",	offset: 0x00FFF890,	size: 1904 },
};

struct brcmnand_info {
	struct mtd_info		mtd;
	struct mtd_partition*	parts;
	struct brcmnand_chip	brcmnand;
};



/* 
 * Size and offset are variable, depending on the size of the chip, but 
 * cfe_kernel always starts at 1FC0_0000 and is 4MB size.
 */
static void __devinit 
brcmnanddrv_setup_mtd_partitions(struct brcmnand_info* nandinfo)
{
	unsigned long size = nandinfo->brcmnand.chipSize;

	nandinfo->parts = bcm7XXX_nand_parts;
	nandinfo->parts[0].size = size - 4*1024*1024;
	nandinfo->parts[1].offset = nandinfo->parts[0].size; // Starts where the rootfs partition ends.
}

static int __devinit brcmnanddrv_probe(struct device *dev)
{
	struct brcmnand_info *info;
	struct platform_device *pdev = to_platform_device(dev);
	//struct flash_platform_data *pdata = pdev->dev.platform_data;
	//struct resource *res = pdev->resource;
	//unsigned long size = res->end - res->start + 1;
	int err = 0;

	info = kmalloc(sizeof(struct brcmnand_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, sizeof(struct brcmnand_info));

	info->brcmnand.numchips = 1; // For now, we only support 1 chip
	info->brcmnand.chip_shift = 0; // Only 1 chip
	info->brcmnand.regs = pdev->resource[0].start;

	//info->brcmnand.mmcontrol = NULL;  // THT: Sync Burst Read TBD.  pdata->mmcontrol;

	info->mtd.name = pdev->dev.bus_id;
	info->mtd.priv = &info->brcmnand;
	info->mtd.owner = THIS_MODULE;

	/* Enable the following for a flash based bad block table */
	info->brcmnand.options |= NAND_USE_FLASH_BBT;
	

printk("brcmnand_scan\n");
	if (brcmnand_scan(&info->mtd, 1)) {
		err = -ENXIO;
		goto out_free_info;
	}

printk("	brcmnanddrv_setup_mtd_partitions\n");
	brcmnanddrv_setup_mtd_partitions(info);
printk("	add_mtd_partitions\n");
	add_mtd_partitions(&info->mtd, info->parts, ARRAY_SIZE(bcm7XXX_nand_parts));
printk("	dev_set_drvdata\n");	
	dev_set_drvdata(&pdev->dev, info);
printk("<-- brcmnanddrv_probe\n");
	return 0;


out_free_info:
	kfree(info);

	return err;
}

static int __devexit brcmnanddrv_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct brcmnand_info *info = dev_get_drvdata(&pdev->dev);
	//struct resource *res = pdev->resource;
	//unsigned long size = res->end - res->start + 1;

	dev_set_drvdata(&pdev->dev, NULL);

	if (info) {
		if (info->parts)
			del_mtd_partitions(&info->mtd);
		//else
			//del_mtd_device(&info->mtd);

		brcmnand_release(&info->mtd);
		//release_mem_region(res->start, size);
		//iounmap(info->brcmnand.base);
		kfree(info);
	}

	return 0;
}

static struct device_driver bcm7xxx_nand_driver = {
	.name		= DRIVER_NAME,
	.bus		= &platform_bus_type,
	.probe		= brcmnanddrv_probe,
	.remove		= __devexit_p(brcmnanddrv_remove),
};

MODULE_ALIAS(DRIVER_NAME);




MODULE_ALIAS(DRIVER_NAME);

static int __init brcmnanddrv_init(void)
{
	int err = 0;
	struct resource devRes[1];
	struct platform_device* pdev;
	
	printk (DRIVER_INFO " (BrcmNand Controller)\n");

	memset(devRes, 0, sizeof(devRes));
	devRes[0].name = "brcmnand-base";
	devRes[0].start = BRCMNAND_CTRL_REGS;
	devRes[0].end = BRCMNAND_CTRL_REGS_END;
	devRes[0].flags = IORESOURCE_MEM;

	// Will need this whenever we use interrupt to look at the flash status
	//devRes[1].name ="brcmNand-irq";
	//devRes[1].start = tbd;
	//devRes[1].end = tbd;
	//devRes[1].flags = IORESOURCE_IRQ;

	// Before we register the driver, add a simple device matching our driver
	pdev = platform_device_register_simple(
		bcm7xxx_nand_driver.name,
		0, /* ID */
		devRes,
		ARRAY_SIZE(devRes));
	if (IS_ERR(pdev)) {
		printk("brcmnanddrv_init: device register failed, err=%d\n", err);
		return PTR_ERR(pdev);
	}

	// Set up dma_mask for our platform device
	// Overwrite whatever value it was set to.
	if (1 /*!pdev->dev.dma_mask*/) {
		extern phys_t upper_memory;

		//dma_set_mask(&pdev->dev, (u64) ((unsigned long) upper_memory - 1UL)); // default is 32MB 0x01ffffff;
		//dma_set_mask(&pdev->dev, 0x01ffffff);
		//pdev->dev.dma_mask = (u64*) 0x01ffffff;  
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask; 
		pdev->dev.coherent_dma_mask = (u64)  ( upper_memory - 1UL);

	}
		

	err = driver_register(&bcm7xxx_nand_driver);
	if (err) {
		printk("brcmnanddrv_init: driver_register failed, err=%d\n", err);
		return err;
	}
	return 0;
}

static void __exit brcmnanddrv_exit(void)
{
	driver_unregister(&bcm7xxx_nand_driver);
}

module_init(brcmnanddrv_init);
module_exit(brcmnanddrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ton Truong <ttruong@broadcom.com>");
MODULE_DESCRIPTION("Broadcom NAND flash driver");

