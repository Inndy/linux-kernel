/***************************************************************
**
**  Broadcom Corp. Confidential
**  Copyright 2000 Broadcom Corp.  All Rights Reserved.
**
**  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED 
**  SOFTWARE LICENSE AGREEMENT  BETWEEN THE USER AND BROADCOM.  
**  YOU HAVE NO RIGHT TO USE OR EXPLOIT THIS MATERIAL EXCEPT 
**  SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
**
**  File:         bcmaccess.h
**  Description:  Hardware Interface access macros
**  Created:      
**
**  REVISION:
**
**  $Log: $
**
**
****************************************************************/
#ifndef _BCMACCESS_H_
#define _BCMACCESS_H_

/****************************************************************
 * macros to mangle the 3250 address 
 * Because the 3250 is a big endian device, you must mangle the
 * addresses before using them when running a little endian cpu.
 ****************************************************************/
#define PCI_ADDR8(x)	(x^3)	/* fix addresses for char access  */
#define PCI_ADDR16(x)	(x^2)	/* fix addresses for short access */

#define PciIoWriteU8(o,d)   (*(volatile unsigned char*)(MIPS_IO_ADDR(o)) = d)
#define PciIoReadU8(o)      (*(volatile unsigned char*)(MIPS_IO_ADDR(o)))

#define PciMemWriteU8(o,d)  (*(volatile unsigned char*)(MIPS_MEM_ADDR(o)) = d)
#define PciMemReadU8(o)     (*(volatile unsigned char*)(MIPS_MEM_ADDR(o)))

#endif /* _BCMACCESS_H_*/
