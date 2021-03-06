#ifndef _SCHED_TYPES_H    /* Once is enough */
#define _SCHED_TYPES_H

/*******************************************************************************

                ? Cambridge Silicon Radio Ltd, 2006
 	 			
				All rights reserved
				
FILE:			types.h

DESCRIPTION:	This file contains definition of the base types that the 
                scheduer and tasks running in the scheduler need.

NOTE:           This file contains _TWO_ parts: first part for system dependent
                stuff and second part which contain the BCHS special 
                definitions. The two parts are DELIMITED by typdef of 
                delimiter_for_start_of_bchs_types_t. This is used for 
                scripting this file.

REVISION:       $Revision: #1 $ 

*******************************************************************************/

#if defined(__linux__)
#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/string.h>
#else
#include <stdint.h>
#include <sys/types.h>
#endif
#endif

#include <stdarg.h>

/*============================================================================*
Public Defines
*============================================================================*/
#undef  FALSE
#define FALSE   (0)

#undef  TRUE
#define TRUE    (1)

#ifndef NULL
#define NULL   (0)
#endif

#undef  bool
#define bool            unsigned

#undef	fast
#define fast            register

#undef	forever
#define forever         for(;;)

#undef	max
#define max(a,b)        (((a) > (b)) ? (a) : (b))

#undef	min
#define min(a,b)	(((a) < (b)) ? (a) : (b))

/* To shut lint up. */
#undef	unreferenced
#define	unreferenced(x)	(void)(x)

#undef	unused
#define	unused(x)	(void)(x)

#define PARAM_UNUSED(x) ((void)(x))

#define INVALID_PHANDLE     0xFFFF       /* Invalid protocol handle setting */

/*============================================================================*
Public Data Types
*============================================================================*/

#if !defined(_SYS_TYPES_H) && !defined(_SYS_INT_TYPES_H)
#if !defined(__linux__)

#if !defined(__CYGWIN_) 
typedef unsigned		int		uint;
#endif

typedef unsigned        char    uint8_t;
typedef unsigned short  int	uint16_t;
typedef unsigned long   int     uint32_t;

#if !defined(__ARM__) 
typedef long int                int32_t;
#if !defined(__SUNOS__)
typedef unsigned int            uint;
#endif
#endif
#endif
#endif

#if !defined(XAP) || !defined(__GNUC__) || !defined(_SYS_TYPES_H)
/*
 * Gcc already defines these types in sys/types.h.
 */

#if !defined(__linux__) && !defined(_SYS_INT_TYPES_H)
typedef signed   char           int8_t;
typedef signed   short  int     int16_t;
#endif

#if !defined(__GNUC__)
typedef signed   long   int     int32_t;
#endif
#endif

#if !defined(_SYS_INT_TYPES_H)
typedef unsigned char			char_t;
typedef uint16_t				uchar_t;
#endif

typedef va_list					VaList;
#define VaStart					va_start
#define VaArg(ap, at)           (at)va_arg(ap, uint32_t)
#define VaEnd					va_end

typedef int                     delimiter_for_start_of_bchs_types_t;
typedef unsigned        char    bool_t;

#if !defined(__cplusplus) && !defined(__XAP__)
#undef bool
#define bool bool_t
#endif

typedef unsigned char           string_t;
typedef unsigned char           ucs2string_t;
typedef unsigned long   int     uint24_t;
typedef uint32_t                tid_t;
typedef uint16_t                phandle_t;
typedef unsigned        char    uint8;
typedef unsigned short  int		uint16;
typedef unsigned long   int     uint32;

/*============================================================================*
Public Data
*============================================================================*/
/* None */

/*============================================================================*
Public Functions
*============================================================================*/
/* None */


#endif /* ifndef _TYPES_H */
/*============================================================================*
End Of File
*============================================================================*/
