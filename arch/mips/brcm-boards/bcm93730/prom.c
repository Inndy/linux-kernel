/*
 * arch/mips/brcm-boards/bcm93730/prom.c
 *
 * Copyright (C) 2001 Broadcom Corporation
 *                    Steven J. Hill <shill@broadcom.com>
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
 * Set up kernel arguments and memory. Currently 32MB is reserved
 * for the 70XX and other Broadcom chip drivers.
 *
 * 11-29-2001   SJH    Created
 * 01-03-2002   SJH    Changes for new command line, new method
 *                     for /proc filesystem.
 * 02-07-2002   SJH    Changed machine group/type stuff
 * 04-04-2003   THT    Added support for kernel command line
 * 04-24-2004	THT	   Added support for mem=xx kernel arg
 */
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/bootinfo.h>
#include <asm/io.h>

unsigned long get_RAM_size(void);
unsigned long g_board_RAM_size = 0;	//Updated by get_RAM_size();;


//#define DEBUG_PROM


char arcs_cmdline[CL_SIZE];


const char *get_system_type(void)
{
	return "BCM93730 Settop Platform";
}

#ifndef LOAD_ADDRESS
#define LOAD_ADDRESS		0x80001000
#define BOOT_OPT_SIZE		512
#define BOOT_OPT_SIG		"BM937XX"
#define BOOT_OPT_LEN_OFFSET	8   /* Must be on boundary or we will get an unpleasant SigBus */
#endif



static uint
bcm_atoi(const char *s)
{
	uint n;

	n = 0;

	while (isdigit(*s))
		n = (n * 10) + *s++ - '0';
	return (n);
}



#ifdef DEBUG_PROM

static void printBootArgRegion(char* p)
{
	unsigned int* savedp = (unsigned int*) p-128;
	int i, j;
	char msg[256];

	for (i=0; i<512; i+=4) {
		char* cp = (char*) &savedp[i];
		sprintf(msg,"Read %08x %08x  %08x %08x at %08lx\t",
			savedp[i], savedp[i+1], savedp[i+2], savedp[i+3], (unsigned long) &savedp[i]);
			
		for (j=0; j<16; j++) {
		       char mm[2];
			if (j == 8) printk(" ");
			if (isprint(cp[j]))
				sprintf(mm,"%c", cp[j]);
			else
				strcpy(mm,".");
			strcat(msg,mm);
		}
		strcat(msg, "\n");
		uart_puts(msg);
	}
}

#else
#define printBootArgRegion(p)
#endif


/*
  * Munges cmdArg, and append the console= string if its not there
  */
static void
appendConsoleArg(char* cmdArg)
{
	char* pConsole = NULL;

	pConsole = strstr(cmdArg, "console=");
	if (!pConsole) {
		if ((strlen(cmdArg) + strlen(" console=tty0 console=ttyS0,115200")) <COMMAND_LINE_SIZE) {
		strcat(cmdArg, " console=tty0 console=ttyS0,115200");
	}
		else {
			uart_puts("***** WARNINGS: No console= appended to kernel args: args is too long");
		}
	}		
	return;
}

static int
isRootSpecified(char* cmdArg)
{
	char* pRoot = NULL;

	if (NULL != cmdArg && cmdArg[0] != '\0') {
		pRoot = strstr(cmdArg, "root=");
	}
	return (NULL != pRoot);
}



void __init prom_init(int argc, const char **arg)
{
        int	hasBootParms = 0;
        char msg[500];
	 char pmon_kargs[CL_SIZE];
		
	uart_init(27000000);
//uart_puts("Inside prom_init\r\n");

	/* Fill in platform information */
	mips_machgroup = MACH_GROUP_BRCM;
	mips_machtype  = MACH_BCM93730;

	/* Boot arguments can be put here */
	/* Browse memory region to see if the boot rom deposits an argument there */
	{
		unsigned long sigAddr = KSEG1ADDR(LOAD_ADDRESS - BOOT_OPT_SIZE);
		unsigned long mapAddr = (sigAddr & 0xFFFFF000);
		unsigned long offset = sigAddr - mapAddr;
		char* pMap;
		char*	p;   /* Start address of bootParms including signature */

		char*	pBootParms; /* Actual start address of bootParms */
		int len = 0;
		int* plen;

//uart_puts( "Before ioremap\n");
		//pMap = ioremap_nocache(mapAddr, 4096);
		pMap = (char*) mapAddr;
		p = &pMap[offset];
//uart_puts("After ioremap\n");
#ifdef DEBUG_PROM
		sprintf(msg,"prom_init, mapped address %08lx at %08lx, pSig1=%08lx, sigAddr=%08lx\n",
			mapAddr, (unsigned long) pMap, (unsigned long) p, sigAddr);
		uart_puts(msg);
		printBootArgRegion(p);
#endif
		if (0 == memcmp(p, BOOT_OPT_SIG, 1+strlen(BOOT_OPT_SIG))) {
			len += BOOT_OPT_LEN_OFFSET;

			/* Rely on BOOT_OPT_LEN_OFFSET to place it at boundary */
			plen = (int*) &p[len];
			pBootParms = &p[len + sizeof(int)];
			/* Making sure that we don't run over any bounds as there is no guarantee
			 * that COMMAND_LINE_SIZE <= BOOT_OPT_SIZE minus the sig
			 */
			if (*plen <= COMMAND_LINE_SIZE && *plen <= (BOOT_OPT_SIZE - 2*BOOT_OPT_LEN_OFFSET)) {
				len += *plen + sizeof(int);
				if (0 == memcmp(&p[len], BOOT_OPT_SIG, 1+strlen(BOOT_OPT_SIG))) {
					hasBootParms = 1;

					printBootArgRegion(p); /* Force it to include the next line in the log */
					sprintf(msg, "Found bootargs=<%s>,%d bytes\n", pBootParms, *plen);
					uart_puts(msg);
					strncpy(pmon_kargs, pBootParms, *plen);

					/* Wipe out boot parameter, so that subsequent reboot would not have it */
					memset(p, 0, 1+strlen(BOOT_OPT_SIG) + *plen);
				}
				else {
					sprintf(msg, "2nd Signature do not match @%08x, ignoring kernel boot parameters\n", &p[len]);
					uart_puts(msg);
					printBootArgRegion(p);
				}
			}
			else {

				sprintf(msg, "Length %d of kernel parameter is larger than allowed min(%d,%d)\n",
					*plen, COMMAND_LINE_SIZE, (BOOT_OPT_SIZE - 2*BOOT_OPT_LEN_OFFSET));
				uart_puts(msg);	

				printBootArgRegion(p);
     		      }
		}
		else {

			printk(KERN_INFO "1st Signature do not match, ignoring kernel boot parameters\n");
			printBootArgRegion(p);

		}
    }
	/* Use default if we don't see a valid boot prom */
	/* We may use a more sophisticated scheme in filling out the parameters later, but for now
	 * just require that the boot loader procedure must provide the full command.  Ugly but
	 * better than nothing.
	 */
	if (hasBootParms && isRootSpecified(pmon_kargs)) {
		strcpy(arcs_cmdline, pmon_kargs);
	   	appendConsoleArg(arcs_cmdline);
	}
	else {
	/* Kernel default arguments */

#ifdef CONFIG_BLK_DEV_INITRD
	         strcpy(arcs_cmdline, "rw console=ttyS0,115200");

#elif defined(CONFIG_CMDLINE)
		char* p;
		char msg[256];

		strcpy(arcs_cmdline, CONFIG_CMDLINE);
		sprintf(msg, "Default command line = \'%s\'\n", CONFIG_CMDLINE);
		uart_puts(msg);
		p = &arcs_cmdline[0];
		while (p != NULL && *p != '\0') {
			if (!isspace(*p))
				break;
			p++;
		}
		if (p == NULL || *p == '\0') {
		sprintf(msg, "Defaulting to boot from HD\n", CONFIG_CMDLINE);
		uart_puts(msg);
			/* Default is to boot from HD */
			strcpy(arcs_cmdline,
				"root=/dev/hda1 rw console=tty0 console=ttyS0,115200");
		}
		else {
			/* Make sure that the boot params specify a console */
			appendConsoleArg(arcs_cmdline);
		}
#else /* No CONFIG_CMDLINE, and not Initrd */
		/* Default is to boot from HD */
		strcpy(arcs_cmdline,
			"root=/dev/hda1 rw console=tty0 console=ttyS0,115200");
#endif /* No CONFIG_CMDLINE */

		/*
		 * if root= is not on the command line, but user specified something else, tag it on
		 */
		if (hasBootParms && !isRootSpecified(pmon_kargs)) {
			strcat(arcs_cmdline, " ");
			strcat(arcs_cmdline, pmon_kargs);
        	}
	} /* End else no root= option is specified */
	uart_puts("Kernel boot options: ");
	uart_puts(arcs_cmdline);
	uart_puts("\r\n");

	{
		/*
		  * Support  mem=nn[KMG] on command line
		  */
		const char* p = (const char*) arcs_cmdline;
		const char* q = NULL;
		const char* sizep = NULL;
		int i, foundKeyword = 0, foundNumber = 0, foundUnit = 0, done = 0;
		unsigned int size = 0, unitShift = 0;
		unsigned int ramSizeMB;

		for (i = 0; i < strlen(p) - 6 && !done; i++) {
//sprintf(msg, "i=%d\n", i);
//uart_puts(msg);
			if (0 == strncmp(&p[i], "mem=", 4)) {
				/* Found key, now read in value */
				foundKeyword = 1;

//uart_puts("while\n");
				for (sizep = q = &p[i+4];*q != '\0' && !done; q++) {
					if (isdigit(*q)) {
						foundNumber = 1;
						continue;
					}

					if (foundNumber) {
//uart_puts("found number\n");

						switch (*q) {
						case 'k':
						case 'K':
							unitShift = 10; /* KB shift value*/
							foundUnit = 1;
							done = 1;
							break;
						case 'm':
						case 'M':
							unitShift = 20; /* MB shift value */
							foundUnit = 1;
							done = 1;
//uart_puts("found unit M\n");
//sprintf(msg, "q=%x\n", q);
//uart_puts(msg);

							break;
						case 'g':
						case 'G':
							/* Probably too big */
							unitShift = 30; /* GB shift value */
							foundUnit = 1;
							done = 1;
							break;
						default:
							done = 1;
							break;
						} 
					}
				}
			} 
		} 

		if (foundNumber) {
			if (foundUnit) {
//uart_puts("Size=");
//uart_puts(sizep);
//uart_puts("\n");
				size = bcm_atoi(sizep);
//sprintf(msg, "q=%x\n", q);
//uart_puts(msg);

				sprintf(msg, "Using %d %cB for memory\n", size, *(q-1));
				uart_puts(msg);
			}
			else {
				uart_puts("Syntax: mem=nn[KMG] Option ignored : No unit specified\n");
			}
		}
		else if (foundKeyword) {
			uart_puts("Syntax: mem=nn[KMG] Option ignored : No size specified\n");
		}

		g_board_RAM_size = get_RAM_size();
		ramSizeMB = g_board_RAM_size >> 20;

		{
			/* 
			  * Kernels on STBs with larger than 32MB, we only use 32MB RAM for the kernel
			  */

	  		if (foundNumber && foundUnit) {
				if (size <= ramSizeMB && size > 0) {
				/* Already output size above */
				} 
				else {
					uart_puts("Invalid size ignored, using default value of 32MB\n");
					size = 32;
					unitShift = 20;
				}
			}
			/* No mem=xxU specified, give the kernel 32MB of memory */
			else {
				uart_puts("Using 32MB for memory, overwrite by passing mem=xx\n");
				size = 32;
				unitShift = 20;
			}
		}
		
		/* Assert size and unit not 0 */
		add_memory_region(0, size << unitShift, BOOT_MEM_RAM);

		/* Register the reserved upper memory, in order to allow kernel to cache them */
		if (size < ramSizeMB) {
			add_memory_region(size << unitShift, (ramSizeMB-size) << unitShift, BOOT_MEM_RAM);
		}
	
	}
}

void __init prom_free_prom_memory(void) {}
