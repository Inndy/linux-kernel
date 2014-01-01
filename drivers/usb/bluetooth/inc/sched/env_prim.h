#ifndef _ENV_PRIM_H     /* Once is enough */
#define _ENV_PRIM_H
/****************************************************************************

				© Cambridge Silicon Radio Ltd, 2004
 	 			
				All rights reserved

FILE:			env_prim.h

DESCRIPTION:	The Primitives used for accessing communicating with the 
                environment

REVISION:		$Revision: #1 $ 

****************************************************************************/

/*============================================================================*
                    Header files on which this one depends
 *============================================================================*/
#include "sched/types.h" 

#ifdef __cplusplus
extern "C" {
#endif


/*============================================================================*
                       Interface #defined parameters
 *============================================================================*/
/* Primitive definitions                                       */
/*-------------------------------------------------------------*/

typedef uint16_t  EnvPrim_t;


#define ENV_PRIM_BASE        0x0000  

/* Define the primitive numbers                                */
enum
{
	ENUM_ENV_CLEANUP_IND = ENV_PRIM_BASE
};

/* Define the primitives here using the enums                  */
#define ENV_CLEANUP_IND	    ((EnvPrim_t)ENUM_ENV_CLEANUP_IND)

typedef struct
{
    /* primitive/message identity */
	EnvPrim_t				type;
    
    /* Application handle that needs to be cleaned up after */
	uint16_t			phandle;
} ENV_CLEANUP_IND_T;


#ifdef __cplusplus
}
#endif


#endif /* _SPP_PRIM_H  */
