/*
 * arch/mips/brcm/prom.c
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
 * 12-23-2005   RYH    Add bcmrac support
 * 05-22-2006   THT    Implement memory hole
 */
#include <linux/config.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h> // SYM EXPORT */
#include <asm/bootinfo.h>

/* RYH */
unsigned int par_val = 0x00;	/* for RAC Mode setting, 0x00-Disabled, 0xD4-I&D Enabled, 0x94-I Only */
unsigned int par_val2 = 0x00;	/* for RAC Cacheable Space setting */

/* Customized flash size in MB */
unsigned int gFlashSize = 0;	/* Default size on 97438 is 64 */
unsigned int gFlashCode = 0; 	/* Special reset codes, 1 for writing 0xF0 to offset 55 for Spansion flash */

/* Clear NAND BBT */
int gClearBBT = 0;
EXPORT_SYMBOL(gClearBBT);



/* The Chip Select [0..7] for the NAND chips from gNumNand above, only applicable to v1.0+ NAND controller */
#define NAND_MAX_CS    8
static int gNandCS_priv[NAND_MAX_CS+1]; // Num NAND stored in first entry
int* gNandCS;
EXPORT_SYMBOL(gNandCS);
/* Number of NAND chips, only applicable to v1.0+ NAND controller */
int gNumNand = 0;
EXPORT_SYMBOL(gNumNand);


/* 7401Cx revision to decide whether C0 workaround is needed or not */
int bcm7401Cx_rev = 0xFF;
EXPORT_SYMBOL(bcm7401Cx_rev);

/* 7118Ax revision to decide whether A0 workaround is needed or not */
int bcm7118Ax_rev = 0xFF;
EXPORT_SYMBOL(bcm7118Ax_rev);


/* 7403Ax revision to decide whether A0 workaround is needed or not */
int bcm7403Ax_rev = 0xFF;
EXPORT_SYMBOL(bcm7403Ax_rev);

#define MAX_HWADDR	16
#define HW_ADDR_LEN	6

int gNumHwAddrs = 0;
EXPORT_SYMBOL(gNumHwAddrs);

#ifdef CONFIG_MTD_BRCMNAND
static unsigned char privHwAddrs[MAX_HWADDR][HW_ADDR_LEN];
unsigned char* gHwAddrs[MAX_HWADDR];


#else
unsigned char** gHwAddrs = NULL;
#endif // CONFIG_MTD_BRCMNAND
EXPORT_SYMBOL(gHwAddrs);

//#define TEST_ROUNDROBIN_DISABLE

unsigned long get_RAM_size(void);
unsigned long g_board_RAM_size = 0;	//Updated by get_RAM_size();;

#if defined (CONFIG_MIPS_BCM7440) || defined (CONFIG_MIPS_BCM7403A0) || \
    defined (CONFIG_MIPS_BCM7452)
int console_uart = 0;
#endif



#if defined( CONFIG_MIPS_BCM7400 )
#define CONSOLE_KARGS " console=uart,mmio,0x10400b00,115200n8"

#elif defined( CONFIG_MIPS_BCM7440 )
//#define CONSOLE_KARGS " console=uart,mmio,0x10400b40,115200n8"
#define CONSOLE_KARGS " console=ttyS0,115200n8"


//#elif defined( CONFIG_MIPS_BCM7401B0 )
//#define DEFAULT_KARGS " rw console=ttyS0,115200 console=uart,mmio,0x10400b00,115200n8"

#else
#define CONSOLE_KARGS " console=ttyS0,115200"
#endif

#define RW_KARGS " rw"
#define DEFAULT_KARGS CONSOLE_KARGS RW_KARGS

static uint
bcm_atoi(const char *s)
{
	uint n;

	n = 0;

	while (isdigit(*s))
		n = (n * 10) + *s++ - '0';
	return (n);
}




#ifdef CONFIG_MIPS_BRCM97XXX
#include "asm/brcmstb/common/cfe_call.h"	/* CFE interface */

extern void uart_init(unsigned long uClock);
void uart_puts(const char *s);
extern void InitEbi (void);

/* RYH */
extern void set_debug_traps(void);
extern void breakpoint(void);
//cmdEntry_t rootEntry;

char cfeBootParms[CFE_CMDLINE_BUFLEN]; 
#endif
//char arcs_cmdline[COMMAND_LINE_SIZE];



/*
  * Munges cmdArg, and append the console= string if its not there
  */
static void
appendConsoleArg(char* cmdArg)
{
	char* pConsole = NULL;
	char* pRwRo = NULL;
	char c = ' ', *from = cmdArg;
	int len = 0;

	for (;;) {
		if (isspace(c) && (!memcmp(from, "ro", 2) || !memcmp(from, "rw", 2))) {
			pRwRo = from+2;
			/* If found ro or rw, and not part of a string */
			if (*pRwRo == '\0' || isspace(*pRwRo)) 
				break;
			pRwRo = NULL;
		}
		c = *(from++);
		if (!c)
			break;
		if (CL_SIZE <= ++len)
			break;
	}
	if (!pRwRo) {
		if ((strlen(cmdArg) + strlen(RW_KARGS)) < COMMAND_LINE_SIZE) {
			strcat(cmdArg, RW_KARGS);
		}
		else {
			uart_puts("***** WARNINGS: No rw appended to kernel args: args is too long");
		}
	}
	
	pConsole = strstr(cmdArg, "console=");
	if (!pConsole) {
		if ((strlen(cmdArg) + strlen(CONSOLE_KARGS)) < COMMAND_LINE_SIZE) {
			strcat(cmdArg, CONSOLE_KARGS);
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

	char c = ' ', *from = cmdArg;
	int len = 0;

	for (;;) {
		if (c == ' ' && !memcmp(from, "root=", 5)) {
			return 1;
		}
		c = *(from++);
		if (!c)
			break;
		if (CL_SIZE <= ++len)
			break;
	}
	
	return 0;
}



void __init prom_init(void)
{

#ifdef CONFIG_MIPS_BRCM97XXX
	int hasCfeParms = 0;
	int res = -1;
	char msg[COMMAND_LINE_SIZE];
	extern void determineBootFromFlashOrRom(void);


#endif

        /* Kernel arguments */
#ifdef CONFIG_MIPS_BRCM97XXX
/* For the 97xxx series STB, process CFE boot parms */

  #ifdef CONFIG_MTD_BRCMNAND
        {
                int i;

                for (i=0; i<MAX_HWADDR; i++) {
                        gHwAddrs[i] = &privHwAddrs[i][0];
                }
        }
  #endif

        res = get_cfe_boot_parms(cfeBootParms, &gNumHwAddrs, gHwAddrs);
  //printk("get_cfe_boot_parms returns %d, strlen(cfeBootParms)=%d\n", res, strlen(cfeBootParms));
        hasCfeParms = ((res == 0 ) || (res == -2));
        // Make sure cfeBootParms is not empty or contains all white space
        if (hasCfeParms) {
                int i;

                hasCfeParms = 0;
                for (i=0; i < strlen(cfeBootParms); i++) {
                        if (isspace(cfeBootParms[i])) {
                                continue;
                        }
                        else if (cfeBootParms[i] == '\0') {
                                break; // and leave hasCfeParms false
                        }
                        else {
                                hasCfeParms = 1;
                                break;
                        }
                }
        }

#endif

#if defined (CONFIG_MIPS_BCM7440) || defined (CONFIG_MIPS_BCM7403A0) || \
    defined (CONFIG_MIPS_BCM7452)

        /*
         * Handle console= boot parameter, if any
         */

        {
          char *cp;

          /* Look for setting for console_uart (console= ) */
          cp = strstr( cfeBootParms, "console=" );
          if (cp)
          {
                cp += 8;

                if (strstr( cp, "ttyS"))
                        cp += 4;
#if defined (CONFIG_MIPS_BCM7440)
                if ( *cp >= '0' && *cp <= '3')
#else
                if ( *cp >= '0' && *cp <= '1')
#endif
                        console_uart = *cp - '0';

          }
          uart_init(27000000);
          sprintf(msg, "found console=  %d\n", console_uart);
          uart_puts(msg);



        }
#endif


#ifdef CONFIG_KGDB
	debugInit();
#else
#if !defined(CONFIG_MIPS_BCM7403A0) && !defined(CONFIG_MIPS_BCM7440) && \
    !defined(CONFIG_MIPS_BCM7452)
	uart_init(27000000);
#endif
#endif
	uart_puts("HI WORLD!!!\n");

#ifdef CONFIG_MIPS_BCM7315_BBX
	*(volatile unsigned long *)(0xfffe7008) = 0x1b800000;
	*(volatile unsigned long *)(0xfffe700c) = 0x00000c91;
#endif

#if defined(CONFIG_MIPS_BCM7115) || defined(CONFIG_MIPS_BCM7112) || \
     defined(CONFIG_MIPS_BCM7314) || defined( CONFIG_MIPS_BCM7315 ) || \
     defined(CONFIG_MIPS_BCM7317) || defined( CONFIG_MIPS_BCM7318 ) || \
     defined(CONFIG_MIPS_BCM7315_BBX) || defined(CONFIG_MIPS_BCM7110) ||\
     defined(CONFIG_MIPS_BCM7111) || defined(CONFIG_MIPS_BCM7312)

	*((volatile unsigned long *)0xfffe0900) = 0x7fff;

#ifndef CONFIG_MIPS_BCM7312

	*(volatile unsigned char *)(0xfffe8008) = 0x00;
	*(volatile unsigned char *)(0xfffe009d) = 0x00;
	*(volatile unsigned char *)(0xfffe0092) = 0x00;
	*(volatile unsigned char *)(0xfffe0093) = 0x55;
	*(volatile unsigned char *)(0xfffe0094) = 0x01;
	*(volatile unsigned char *)(0xfffe0095) = 0xaa;
	*(volatile unsigned char *)(0xfffe009a) = 0x80;
#endif

#if 0
/* THT 7/28/03 Now done in CFE */
#ifdef CONFIG_MIPS_BCM7110
	*(volatile unsigned long *)(0xffe00028) = 0x10fa;
#endif
#if defined( CONFIG_MIPS_BCM7315 ) || defined ( CONFIG_MIPS_BCM7315_BBX )
	*(volatile unsigned long *)(0xffe00028) = 0x30f8;
#endif
#if defined( CONFIG_MIPS_BCM7115 ) || defined ( CONFIG_MIPS_BCM7112 )
	*(volatile unsigned long *)(0xffe00028) = 0x11f5;
#endif
#endif

	//GPIO1 set to output and high to force the drive out of reset. 
	// we need to move this into some borad specific dirs.
	*((volatile unsigned char *)0xfffe0057) &= 0xfd;
	*((volatile unsigned char *)0xfffe0053) |= 0x02;
	// we need to move this into some borad specific dirs.

	//InitEbi();
	//uart_puts("init Ebi\n");
#endif

	/* Fill in platform information */
	mips_machgroup = MACH_GROUP_BRCM;
#ifdef CONFIG_MIPS_BCM3560	
	mips_machtype  = MACH_BRCM_3560;
#endif
#ifdef CONFIG_MIPS_BCM7115	
	mips_machtype  = MACH_BRCM_7115;
#endif
#ifdef CONFIG_MIPS_BCM7112	
	mips_machtype  = MACH_BRCM_7112;
#endif

#ifdef CONFIG_MIPS_BCM7110	
	mips_machtype  = MACH_BRCM_7110;
#endif
#ifdef CONFIG_MIPS_BCM7111
	mips_machtype  = MACH_BRCM_7111;
#endif
#ifdef CONFIG_MIPS_BCM7320	
	mips_machtype  = MACH_BRCM_7320;
/*
** Put this here until CFE does it,ISB bus timer
*/
        *((volatile unsigned long *)0xbafe0900) = 0x7fff;

#endif
#ifdef CONFIG_MIPS_BCM7314	
	mips_machtype  = MACH_BRCM_7314;
#endif
#ifdef CONFIG_MIPS_BCM7315	
	mips_machtype  = MACH_BRCM_7315;
#endif
#ifdef CONFIG_MIPS_BCM7317	
	mips_machtype  = MACH_BRCM_7317;
#endif
#ifdef CONFIG_MIPS_BCM7318	
	mips_machtype  = MACH_BRCM_7318;
#endif
#ifdef CONFIG_MIPS_BCM7319	
		mips_machtype  = MACH_BRCM_7319;
       *((volatile unsigned long *)0xbafe0900) = 0x7fff;
#endif
#ifdef CONFIG_MIPS_BCM7328	
		mips_machtype  = MACH_BRCM_7328;
       *((volatile unsigned long *)0xbafe0900) = 0x7fff;
#endif
#ifdef CONFIG_MIPS_BCM7038	
		mips_machtype  = MACH_BRCM_7038;

#ifdef TEST_ROUNDROBIN_DISABLE
// TBD: Change this to be "if chip revision is C0 or later"
		/*
		 * Testing 1,2,3: PR11804: Disabling RoundRobin in 7038 to see
		 * if it can withstand the transfer rate (bit 6)
		 */
		*((volatile unsigned long *)0xb01061b4) &= 0xffffffbf;
#endif
#endif
#ifdef CONFIG_MIPS_BCM7327	
	mips_machtype  = MACH_BRCM_7327;
       *((volatile unsigned long *)0xbafe0900) = 0x7fff;
#endif
#ifdef CONFIG_MIPS_BCM7329	
	mips_machtype  = MACH_BRCM_7329;
       *((volatile unsigned long *)0xbafe0900) = 0x7fff;
#endif
#ifdef CONFIG_MIPS_BCM7402	
		mips_machtype  = MACH_BRCM_7401;
#elif defined( CONFIG_MIPS_BCM7401 )	
		mips_machtype  = MACH_BRCM_7401;
#elif defined( CONFIG_MIPS_BCM7403 ) || defined( CONFIG_MIPS_BCM7452 )
                mips_machtype  = MACH_BRCM_7403;

#endif
#ifdef CONFIG_MIPS_BCM7400
		mips_machtype  = MACH_BRCM_7400;
#endif
#ifdef CONFIG_MIPS_BCM7118
		mips_machtype  = MACH_BRCM_7118;		
#endif
#ifdef CONFIG_MIPS_BCM7440
		mips_machtype  = MACH_BRCM_7440;
#endif

#if defined( CONFIG_MIPS_BCM7118) || defined( CONFIG_MIPS_BCM7401C0 )	\
     ||	defined( CONFIG_MIPS_BCM7402C0 ) || defined( CONFIG_MIPS_BCM3563 ) \
     || defined( CONFIG_MIPS_BCM7403A0 ) || defined( CONFIG_MIPS_BCM7452 )
// jipeng - need set bus to async mode before enabling the following	
	if(!(read_c0_diag4() & 0x400000))
	{
		int	val=read_c0_diag4();
		write_c0_diag4(val | 0x400000);
		sprintf(msg, "CP0 reg 22 sel 0 to 5: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", read_c0_diag(), read_c0_diag1(), read_c0_diag2(), read_c0_diag3(), read_c0_diag4(), read_c0_diag5());
		uart_puts(msg);
	}
	
	{
		volatile unsigned long* pMemMap = (volatile unsigned long*) 0xb000040c;
		volatile unsigned long* pSplit = (volatile unsigned long*) 0xb0000410;
		
		// Enable write gathering 
		*pMemMap = 0x264;
		// Enable Split Mode
		*pSplit = 1;
		sprintf(msg, "gathering mode: 0x%08x at 0x0xb000040c\nsplit mode: 0x%08x at 0xb0000410\n", *pMemMap, *pSplit);
		uart_puts(msg);
	}
#elif defined( CONFIG_MIPS_BCM7440A0 )
	if(!(read_c0_diag4() & 0x400000))
	{
		int	val=read_c0_diag4();
		write_c0_diag4(val | 0x400000);
		sprintf(msg, "CP0 reg 22 sel 0 to 5: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", read_c0_diag(), read_c0_diag1(), read_c0_diag2(), read_c0_diag3(), read_c0_diag4(), read_c0_diag5());
		uart_puts(msg);
	}
	
	{
		volatile unsigned long* pMemMap = (volatile unsigned long*) 0xb000040c;
		
		// Enable write gathering 
		*pMemMap = 0x2803;
		sprintf(msg, "gathering mode: 0x%08x at 0x0xb000040c\n", *pMemMap);
		uart_puts(msg);
	}
#endif

        /* Kernel arguments */
#ifdef CONFIG_MIPS_BRCM97XXX
/* For the 97xxx series STB, process CFE boot parms */


	/* RYH - RAC */
	{
	  char	str1[32], str2[32] = "0x";
	  char *cp, *sp;
          unsigned int rac_addr_range;

#ifdef CONFIG_MIPS_BCM7440A0
          /* 
           * the 7440A0 has discontiguous memory.
           * This value should be derived from the highest PFN
           */
          rac_addr_range = 0x2fff0000;
#else
          rac_addr_range = (get_RAM_size()-1) & 0xffff0000;
#endif



	  sprintf(msg, "cfeBootParms ===> %s\n", cfeBootParms);
	  uart_puts(msg);
	  cp = strstr( cfeBootParms, "bcmrac=" );
	  if( cp ) { 
		if ( strchr(cp, ',') ) {
			for( cp+=strlen("bcmrac="),sp=str1; *cp != ','; *sp++=*cp++ );
			*sp = '\0';
			strcat( str2, ++cp );
			sprintf(msg, "STR1 : %s    STR2 : %s\n", str1, str2);
		        uart_puts(msg);
			sscanf( str1, "%u", &par_val );
			sscanf( str2, "%x", &par_val2 );
			if (par_val2 == 0x00)
                                par_val2 = rac_addr_range;
		} else {
			sprintf(msg, "RAC Cacheable Space is set to default...\n");
		        uart_puts(msg);
			sscanf( cp+=strlen("bcmrac="), "%i", &par_val );
                        par_val2 = rac_addr_range;
		}
	  }
	  else {
#ifdef CONFIG_MIPS_BCM7440A0
                /*
                 * set default to I/D-RAC enabled,
                 * C_INV='1'b, PF_D='1'b, PF_I='1'b, RAC_D='1'b, RAC_I='1'b
                 */
                par_val = 11;
#elif CONFIG_MIPS_BCM7400B0
               par_val = 0xff;         /* default: leave RAC configuration alone */

#else
                par_val = 0x03;         /* set default to I/D RAC on */
#endif
                par_val2 = rac_addr_range;

	  }
	}

	/* flashsize=nnM */
	{
		char c = ' ', *from = cfeBootParms;
		int len = 0;

		for (;;) {
			if (c == ' ' && !memcmp(from, "flashsize=", 10)) {
				gFlashSize = memparse(from + 10, &from) >> 20;
				break;
			}
			c = *(from++);
			if (!c)
				break;
			if (CL_SIZE <= ++len)
				break;
		}
	}

	/* flashcode=1 : Write 0xF0 to reset Spansion flash */
	{
		char c = ' ', *from = cfeBootParms;
		int len = 0;

		for (;;) {
			if (c == ' ' && !memcmp(from, "flashcode=", 10)) {
				gFlashCode = memparse(from + 10, &from);
				break;
			}
			c = *(from++);
			if (!c)
				break;
			if (CL_SIZE <= ++len)
				break;
		}
	}

	/* brcmnand=
	 *	rescan: 	1. Rescan for bad blocks, and update existing BBT
	 *	showbbt:	2. Print out the contents of the BBT on boot up.
	 *
	 * The following commands are implemented but should be removed for production builds.  
	 * Use userspace flash_eraseall instead.
	 * These were intended for development debugging only.
	 * 	erase:	7. Erase entire flash, except CFE, and rescan for bad blocks 
	 *	eraseall:	8. Erase entire flash, and rescan for bad blocks
	 *	clearbbt:	9. Erase BBT and rescan for bad blocks.  (DANGEROUS, may lose Mfg's BIs).
	 */

	{
		char c = ' ', *from = cfeBootParms;
		int len = 0;

		for (;;) {
			if (c == ' ' && !memcmp(from, "brcmnand=", 9)) {
				if (!memcmp(from + 9, "rescan", 6)) {
					gClearBBT = 1; // Force BBT rescan, only BBT is erased and rebuilt
				}
				else if (!memcmp(from + 9, "showbbt", 7)) {
					gClearBBT = 2; // Force erase of entire NAND flash, before rescan
				}
#if 1
// Remove this in production build
				// Here we match the longer string first
				else if (!memcmp(from + 9, "eraseall", 8)) {
					gClearBBT = 8; // Force erase of entire NAND flash, before rescan
				}
				else if (!memcmp(from + 9, "erase", 5)) {
					gClearBBT = 7; // Force erase of NAND flash, except CFE, before rescan
				}

				else if (!memcmp(from + 9, "clearbbt", 8)) {
					gClearBBT = 9; // Force erase of BBT, DANGEROUS.
				}
#endif
				break;
			}
			c = *(from++);
			if (!c)
				break;
			if (CL_SIZE <= ++len)
				break;
		}
	}

       /* CS override for BrcmNAND controller */
       {
               char c = ' ', *from = cfeBootParms;
               int len = 0;
               int i;

               gNumNand = 0;
               gNandCS = NULL;
               for (i=0; i<ARRAY_SIZE(gNandCS_priv); i++) {
                       gNandCS_priv[i] = -1;
               }
               
               for (;;) {
                       if (c == ' ' && !memcmp(from, "nandcs=", 7)) {
                               get_options(from + 7, ARRAY_SIZE(gNandCS_priv), gNandCS_priv);
                               break;
                       }
                       c = *(from++);
                       if (!c)
                               break;
                       if (CL_SIZE <= ++len)
                               break;
               }
               if (gNandCS_priv[0] != 0 && gNandCS_priv[0] != -1) {
                       gNumNand = gNandCS_priv[0]; // Num NAND stored in first entry
                       gNandCS = &gNandCS_priv[1];     // Real array starts at element #1
               }

               printk("Number of Nand Chips = %d\n", gNumNand);
       }



	/* Just accept whatever specified in BOOT_FLAGS as kernel options, unless root= is NOT specified */
	if (hasCfeParms && isRootSpecified(cfeBootParms)) {
//sprintf(msg, "after get_cfe_boot_parms, res=0, BootParmAddr=%08x,bootParms=%s\n",
//&cfeBootParms[0],cfeBootParms);
//uart_puts(msg);

		strcpy(arcs_cmdline, cfeBootParms);
		appendConsoleArg(arcs_cmdline);
	}
	else /* The old ways of doing it, as root is not specified on the command line */
		
#endif
	{
	/* Gets here when one of the following is true
	  * - CFE is not the boot loader, or
	  * - CFE is the boot loader, but no option is specified or
	  * - CFE option is specified, but does not say root=
	  */

#ifdef CONFIG_BLK_DEV_INITRD
		/*
		 * tht: Things like init=xxxx wants to be first on command line,
		 * kernel arg parse logic depends on that
		 */
		if (hasCfeParms) {
			strcpy(arcs_cmdline, cfeBootParms);
			strcat(arcs_cmdline, DEFAULT_KARGS);
			hasCfeParms = 0; // Suppress further processing
		}
		else {
			strcpy(arcs_cmdline, DEFAULT_KARGS);
		}

#elif defined(CONFIG_CMDLINE)
		char* p;
		int appendConsoleNeeded = 1;

#ifdef CONFIG_MIPS_BRCM97XXX

		/*
		  * if root= is not on the command line, but user specified something else, 
		  * tag it on.  Some command like init=xxx wants to be first, as kernel
		  * arg parse logic depends on that.
		  */
		if (hasCfeParms && !isRootSpecified(cfeBootParms)) {
			strcpy(arcs_cmdline, cfeBootParms);
			strcat(arcs_cmdline, " ");
			strcat(arcs_cmdline, CONFIG_CMDLINE);
			appendConsoleNeeded = 0;
		}
		else {
			strcpy(arcs_cmdline, CONFIG_CMDLINE);
			appendConsoleNeeded = 0;
		}
#else
		strcpy(arcs_cmdline, CONFIG_CMDLINE);
		appendConsoleNeeded = 0;
#endif
		
		uart_puts("Default command line = \n");
		uart_puts(CONFIG_CMDLINE);
		uart_puts("\n");
		p = &arcs_cmdline[0];
		while (p != NULL && *p != '\0') {
			if (!isspace(*p))
				break;
			p++;
		}
		if (p == NULL || *p == '\0') {
			uart_puts("Defaulting to boot from HD\n");
			/* Default is to boot from HD */
			strcpy(arcs_cmdline,
				"root=/dev/hda1" DEFAULT_KARGS);
		}
		else if (appendConsoleNeeded) {
			/* Make sure that the boot params specify a console */
			appendConsoleArg(arcs_cmdline);
		}
		
#else /* No CONFIG_CMDLINE, and not Initrd */
	/* Default is to boot from HD */
		strcpy(arcs_cmdline,
			"root=/dev/hda1" DEFAULT_KARGS);
#endif /* No CONFIG_CMDLINE */



	} /* End else no root= option is specified */

	uart_puts("Kernel boot options: ");
	uart_puts(arcs_cmdline);
	uart_puts("\r\n");

	// THT: PR21410 Implement memory hole in init_bootmem, now just record the memory size.
	{
		unsigned int ramSizeMB;

		g_board_RAM_size = get_RAM_size();
		ramSizeMB = g_board_RAM_size >> 20;


		if (ramSizeMB <= 256) {
			add_memory_region(0, g_board_RAM_size, BOOT_MEM_RAM);
		}
		else {
			add_memory_region(0, 256<<20, BOOT_MEM_RAM);
#ifdef CONFIG_DISCONTIGMEM
/*
 * ADT                                    
 * BCM97440 has 3 physical memory blocks: 
 * 0x00000000 - 0x10000000  -  256MB      
 * 0x20000000 - 0x40000000  -  512MB      
 * 0x50000000 - 0x60000000  -  256MB 
 * If not fully populated, we support only 512 MB configuration for simplicity.
 *     
 */
#ifdef CONFIG_MIPS_BCM7440
                        if (ramSizeMB <= 768)
                        {
                                add_memory_region(512 << 20,
                                                  (ramSizeMB - 256) << 20,
                                                  BOOT_MEM_RAM);
                        }
                        else
                        {
                                add_memory_region(512 << 20,
                                                  512 << 20,
                                                  BOOT_MEM_RAM);
#ifdef NEVER
                                add_memory_region(1280 << 20,
                                                  256  << 20,
                                                  BOOT_MEM_RAM);
#endif

                        }

#elif defined (CONFIG_MIPS_BCM97438)

			add_memory_region(512<<20, (ramSizeMB - 256)<<20, BOOT_MEM_RAM);
#endif

#else
			uart_puts("Extra RAM beyond 256MB ignored.  Please use a kernel that supports NUMA\n");
#endif
		}
	} 
	

#if defined (CONFIG_MIPS_BRCM97XXX) 
	(void) determineBootFromFlashOrRom();
#endif /* if BCM97xxx boards */
//	uart_puts("<--prom_init\r\n");

#ifndef CONFIG_MIPS_BCM7440B0

// jipeng - if it is not in sync mode, set it here
	switch (current_cpu_data.cputype) {
               	case CPU_BMIPS3300:
                    // RYH - BHTD patch 11/15/06
                    {
                        int cp022;

                        cp022 = __read_32bit_c0_register($22, 5);
                        sprintf(msg, "Initial CP0 22 value : 0x%08x\n", cp022);   
                        uart_puts(msg);

			// clear BHTD (branch history disable)
                        if ( cp022 & (1 << 16) ) {     // RYH - cp0 reg 22, sel 5, bit[16]
                                cp022 &= ~(1 << 16);
                                __write_32bit_c0_register($22, 5, cp022);

                                cp022 = __read_32bit_c0_register($22, 5);
                                sprintf(msg, "Updated CP0 22 value : 0x%08x\n", cp022);
                                uart_puts(msg);
                        }
                    }
		    // clear ASC (disable async mode for BIU)
                    if(read_c0_diag4() & (1 << 22))
                    {
                        int     val=read_c0_diag4();
                        write_c0_diag4(val & ~(1 << 22));
                        sprintf(msg, "CP0 reg 22 sel 0 to 5: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
                                        read_c0_diag(),
                                        read_c0_diag1(),
                                        read_c0_diag2(),
                                        read_c0_diag3(),
                                        read_c0_diag4(),
                                        read_c0_diag5());
                        uart_puts(msg);
                    }

                    break;


	       	case CPU_BMIPS4350:
	       	case CPU_BMIPS4380:

                    // RYH - BHTD patch 01/08/07
                    {
                        int cp022;

                        cp022 = __read_32bit_c0_register($22, 0);
                        sprintf(msg, "Initial CP0 22 value : 0x%08x\n", cp022);   
                        uart_puts(msg);

			// clear BHTD (branch history disable)
			cp022 &= ~(1 << 21);
			// set CLF (critical line first, to optimize RAC performance)
			cp022 |= (1 << 20);
			__write_32bit_c0_register($22, 0, cp022);

			cp022 = __read_32bit_c0_register($22, 0);
			sprintf(msg, "Updated CP0 22 value : 0x%08x\n", cp022);
			uart_puts(msg);
                    }

		    // clear ASC (disable async mode for BIU)
		    if(read_c0_diag5() & (1 << 28))
		    {
			int	val=read_c0_diag5();
			write_c0_diag5(val & ~(1 << 28));
			sprintf(msg, "CP0 reg 22 sel 0 to 5: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
					read_c0_diag(), 
					read_c0_diag1(), 
					read_c0_diag2(), 
					read_c0_diag3(), 
					read_c0_diag4(), 
					read_c0_diag5());
			uart_puts(msg);
		    }
               	    break;

		default:
		    break;
	}	

	if(bcm7401Cx_rev == 0xFF)
	{
		volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xb0404000;
        	unsigned long chipId = (*pSundryRev) >> 16;		
		
		if(chipId == 0x7401)
		{
			bcm7401Cx_rev = (*pSundryRev) & 0xFF;
			sprintf(msg, "Sundry 0x%08x, chipId 0x%08x, bcm7401Cx 0x%02x\n",  pSundryRev, chipId, bcm7401Cx_rev);
			uart_puts(msg);
		}
		else if(chipId == 0x7118)
		{
			bcm7118Ax_rev = (*pSundryRev) & 0xFF;
			sprintf(msg, "Sundry 0x%08x, chipId 0x%08x, bcm7118Ax 0x%02x\n",  pSundryRev, chipId, bcm7118Ax_rev);
			uart_puts(msg);
		}
		else if(chipId == 0x7403)
		{
			bcm7403Ax_rev = (*pSundryRev) & 0xFF;
			sprintf(msg, "Sundry 0x%08x, chipId 0x%08x, bcm7403Ax %02x\n",  pSundryRev, chipId, bcm7403Ax_rev);
			uart_puts(msg);
		}

	}

#endif  /* of #ifndef CONFIG_MIPS_BCM7440B0   */

}

const char *get_system_type(void)
{
        return "BCM97xxx Settop Platform";
}

void __init prom_free_prom_memory(void) {}
