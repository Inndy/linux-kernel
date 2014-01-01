/*
 * kgdb.h: Defines and declarations for serial line source level
 *         remote debugging of the Linux kernel using gdb.
 *
 * copied from include/asm-ppc, modified for ppc64
 *
 * PPC64 Mods (C) 2005 Frank Rowand (frowand@mvista.com)
 * PPC Mods (C) 2004 Tom Rini (trini@mvista.com)
 * PPC Mods (C) 2003 John Whitney (john.whitney@timesys.com)
 * PPC Mods (C) 1998 Michael Tesch (tesch@cs.wisc.edu)
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */
#ifdef __KERNEL__
#ifndef _PPC64_KGDB_H
#define _PPC64_KGDB_H

#ifndef __ASSEMBLY__

#define BREAK_INSTR_SIZE	4
#if 1
/*
 * Does not include vector registers and vector state registers.
 *
 * 64 bit (8 byte) registers:
 *   32 gpr, 32 fpr, nip, msr, link, ctr
 * 32 bit (4 byte) registers:
 *   ccr, xer, fpscr
 */
#define NUMREGBYTES		((68 * 8) + (3 * 4))
#else
/*
 * Includes vector registers and vector state registers.
 *
 * 128 bit (16 byte) registers:
 *   32 vr
 * 64 bit (8 byte) registers:
 *   32 gpr, 32 fpr, nip, msr, link, ctr
 * 32 bit (4 byte) registers:
 *   ccr, xer, fpscr, vscr, vrsave
 */
#define NUMREGBYTES		((128 * 16) + (68 * 8) + (5 * 4))
#endif
#define NUMCRITREGBYTES		184
#define BUFMAX			((NUMREGBYTES * 2) + 512)
#define OUTBUFMAX		((NUMREGBYTES * 2) + 512)
#define BREAKPOINT()		asm(".long 0x7d821008"); /* twge r2, r2 */
#define CHECK_EXCEPTION_STACK()	1
#define CACHE_FLUSH_IS_SAFE	1

#endif /* !(__ASSEMBLY__) */

#endif /* !(_PPC64_KGDB_H) */

#endif /* __KERNEL__ */
