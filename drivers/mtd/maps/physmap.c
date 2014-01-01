/*
 * $Id: physmap.c,v 1.39 2005/11/29 14:49:36 gleixner Exp $
 *
 * Normal mappings of chips in physical memory
 *
 * Copyright (C) 2003 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * 031022 - [jsun] add run-time configure and partition setup
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>

static struct mtd_info *mymtd;

#ifdef CONFIG_HUMAX_CONTROL_VPP
#include <linux/delay.h>
void phys_map_set_vpp(struct map_info *map, int iVal)
{
/*
 * It is supposed that VPP pin is connected to GPIO_31.
 *
 * 081223 dcchung
 */
	
#define GIO_DATA_LO		0x10400704
#define GIO_IODIR_LO	0x10400708
#define PIN_MUX_CTRL_9	0x104040b8
	
	volatile unsigned int* pulIoData=GIO_DATA_LO;
	volatile unsigned int* pulIoDir=GIO_IODIR_LO;
	volatile unsigned int* pulPinmuxCtrl9=PIN_MUX_CTRL_9;

	// set GPIO_31
	*(volatile unsigned int *)KSEG1ADDR(pulPinmuxCtrl9)&=~0x000001c0;
	// SET GPIO_31 to OUTPUT
	*(volatile unsigned int *)KSEG1ADDR(pulIoDir)&=~0x80000000;
	if (iVal==1)
	{
		*(volatile unsigned int *)KSEG1ADDR(pulIoData)|=0x80000000;
		// It takes time for VPP(GPIO_31) to go up to HIGH level.		
		// For this reason, put "delay" here.
		udelay(10);
		//printk("VPP Enabled:%08lx\n", *(volatile unsigned int *)KSEG1ADDR(pulIoData));
	}
	else
	{
		*(volatile unsigned int *)KSEG1ADDR(pulIoData)&=~0x80000000;
		//printk("VPP Disabled:%08lx\n", *(volatile unsigned int *)KSEG1ADDR(pulIoData));
	}

#undef GIO_DATA_LO		
#undef GIO_IODIR_LO	
#undef PIN_MUX_CTRL_9	
}
#endif


struct map_info physmap_map = {
	.name = "phys_mapped_flash",

#ifdef CONFIG_HUMAX_CONTROL_VPP
	set_vpp: phys_map_set_vpp,
#endif

	.phys = CONFIG_MTD_PHYSMAP_START,
	.size = CONFIG_MTD_PHYSMAP_LEN,
	.bankwidth = CONFIG_MTD_PHYSMAP_BANKWIDTH,
};

#ifdef CONFIG_MTD_PARTITIONS
static struct mtd_partition *mtd_parts;
static int                   mtd_parts_nb;

static int num_physmap_partitions;
static struct mtd_partition *physmap_partitions;

static const char *part_probes[] __initdata = {"cmdlinepart", "RedBoot", NULL};

void physmap_set_partitions(struct mtd_partition *parts, int num_parts)
{
	physmap_partitions=parts;
	num_physmap_partitions=num_parts;
}
#endif /* CONFIG_MTD_PARTITIONS */

static int __init init_physmap(void)
{
	static const char *rom_probe_types[] = { "cfi_probe", "jedec_probe", "map_rom", NULL };
	const char **type;

       	printk(KERN_NOTICE "physmap flash device: %lx at %lx\n", physmap_map.size, physmap_map.phys);
	physmap_map.virt = ioremap(physmap_map.phys, physmap_map.size);

	if (!physmap_map.virt) {
		printk("Failed to ioremap\n");
		return -EIO;
	}

	simple_map_init(&physmap_map);

	mymtd = NULL;
	type = rom_probe_types;
	for(; !mymtd && *type; type++) {
		mymtd = do_map_probe(*type, &physmap_map);
	}
	if (mymtd) {
		mymtd->owner = THIS_MODULE;

#ifdef CONFIG_MTD_PARTITIONS
		mtd_parts_nb = parse_mtd_partitions(mymtd, part_probes,
						    &mtd_parts, 0);

		if (mtd_parts_nb > 0)
		{
			add_mtd_partitions (mymtd, mtd_parts, mtd_parts_nb);
			return 0;
		}

		if (num_physmap_partitions != 0)
		{
			printk(KERN_NOTICE
			       "Using physmap partition definition\n");
			add_mtd_partitions (mymtd, physmap_partitions, num_physmap_partitions);
			return 0;
		}

#endif
		add_mtd_device(mymtd);

		return 0;
	}

	iounmap(physmap_map.virt);
	return -ENXIO;
}

static void __exit cleanup_physmap(void)
{
#ifdef CONFIG_MTD_PARTITIONS
	if (mtd_parts_nb) {
		del_mtd_partitions(mymtd);
		kfree(mtd_parts);
	} else if (num_physmap_partitions) {
		del_mtd_partitions(mymtd);
	} else {
		del_mtd_device(mymtd);
	}
#else
	del_mtd_device(mymtd);
#endif
	map_destroy(mymtd);

	iounmap(physmap_map.virt);
	physmap_map.virt = NULL;
}

module_init(init_physmap);
module_exit(cleanup_physmap);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("Generic configurable MTD map driver");
