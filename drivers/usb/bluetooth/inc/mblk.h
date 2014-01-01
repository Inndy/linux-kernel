#ifndef __MBLK_H__ /* Once is enough */
#define __MBLK_H__
/*******************************************************************************

					© Cambridge Silicon Radio Ltd, 2005

					All rights reserved

FILE:		        mblk.h

DESCRIPTION:		Defines the MBLK message block structure

REVISION:			$Revision: #4 $

*******************************************************************************/

/*============================================================================*
Public Defines
*============================================================================*/

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(STR_LOG_DEBUG)

#define STRLOGMBLK( multiple_arg )  str_mblk_log multiple_arg

#else

#define STRLOGMBLK( ignore )

#endif


/*============================================================================*
Public Data Types
*============================================================================*/
typedef uint32_t    cs_pool_t;  /* pool identifier */


/*------------------------------------------------------*\
    This structure is used to pass data up and down
    the stack.

    It is defined with three extra fields...
    - pNext
    - bufLength
    - pool
    ...which are only used by host-side users of the
    MBLK structure.  These fields are not used in
    the on-chip code.
\*------------------------------------------------------*/

typedef struct  msgTag
{
     uint8_t isNotReallyAnMblk; /* Used by BNEP -- DO NOT MOVE */
     uint8_t isRFCOMM;  /* is this an RFCOMM msg ? */
     uint8_t isMsgFrag;    /* is this really a msgfrag ? */
     uint8_t isCopy;   /* is this actually a copy of a msgfrag ? */
     uint8_t isIN;     /* is this an Internet header (packed, big-endian)? */
     uint8_t isINspecial; /* if isIN, does first word only contain an octet? */
	 uint8_t reserved1;
     uint8_t*        pData;      /* pointer to start of data */
     void*           pRelease;   /* pointer to address to give to Free() if
	 buffer is to be released */
	 uint8_t		*pDataOrig;
	 bool_t			useOffset;
	uint16_t		lengthOrig;

     struct MSGFRAG * msgfrag ; /* TODO - this should really be a union but then
				   I'd have to change too many files */

     uint16_t offset ; /*offset from start of msgfrag before data */
     uint16_t        length;     /* length of data (in bytes) */
     struct  msgTag* pCont;      /* pointer to next mblk in a message chain */

#if !defined(BTCHIP)
    struct  msgTag* pNext;      /* pointer to next mblk in a queue */
#endif

} MBLK_T;


/*============================================================================*
Public Data
*============================================================================*/
/* None */


/*============================================================================*
Public Functions
*============================================================================*/
/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_copy
 *
 *  DESCRIPTION
 *  Make a deep copy of message block chain
 *
 *  RETURNS/MODIFIES
 *  The copy (the input mblk is left unchanged) or NULL
 *
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_copy(const MBLK_T *mblk);

/*----------------------------------------------------------------------------*
 *  NAME
 *  void mblk_free
 *
 *  DESCRIPTION
 *  Free a message block chain
 *
 *  RETURNS/MODIFIES
 *
 *
 *----------------------------------------------------------------------------*/
extern void mblk_free(MBLK_T * mblk);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_xnew_pmalloc
 *
 *  DESCRIPTION
 *  Create a new message block with a pmalloc of a given size.
 *  All flags are cleared, pCont is set to NULL, pData and pRelease
 *  are set to the pmalloced block (or NULL, if the length is zero).
 *  msgfrag and offset are undefined, as are the contents of the pmalloc.
 *
 *  RETURNS/MODIFIES
 *  Pointer to a new MBLK or NULL if failed.
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_xnew_pmalloc (uint16_t data_len);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_new_pmalloc
 *
 *  DESCRIPTION
 *  Same as mblk_xnew_pmalloc except that it panics on failure.
 *
 *  RETURNS/MODIFIES
 *  Pointer to a new MBLK.
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_new_pmalloc (uint16_t data_len);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_xnew_in
 *
 *  DESCRIPTION
 *  Create a new Internet message block with a pmalloc of a given size.
 *  All flags other than isIN and isINspecial are cleared, pCont is set
 *  to NULL, pData and pRelease are set to the pmalloced block (or NULL,
 *  if the length is zero).
 *  msgfrag and offset are undefined, as are the contents of the pmalloc.
 *  If special is TRUE then isINspecial is set; the length should be the
 *  logical length in octets (e.g. a length of 5 for 1+2*2 octets).
 *
 *  RETURNS/MODIFIES
 *  Pointer to a new MBLK or NULL if failed.
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_xnew_in (uint16_t data_len, bool_t special);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_new_in
 *
 *  DESCRIPTION
 *  Same as mblk_xnew_in except that it panics on failure.
 *
 *  RETURNS/MODIFIES
 *  Pointer to a new MBLK.
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_new_in (uint16_t data_len, bool_t special);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_xnew_msgfrag
 *
 *  DESCRIPTION
 *  Create a new message block with a given msgfrag.
 *  All flags other than isMsgFrag are cleared, pCont is set to NULL,
 *  the offset is set to zero and the length is set to the length
 *  of the msgfrag (or zero if no msgfrag is passed).
 *  pData and pRelease are undefined.
 *
 *  RETURNS/MODIFIES
 *  Pointer to a new MBLK or NULL if failed.
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_xnew_msgfrag (MSGFRAG *msgfrag);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_new_msgfrag
 *
 *  DESCRIPTION
 *  Same as mblk_xnew_msgfrag except that it panics on failure.
 *
 *  RETURNS/MODIFIES
 *  Pointer to a new MBLK.
 *----------------------------------------------------------------------------*/
extern MBLK_T *mblk_new_msgfrag (MSGFRAG *msgfrag);

/*----------------------------------------------------------------------------*
 *  NAME
 *      mblk_append_mblk
 *
 *  DESCRIPTION
 *      Appends the new MBLK 'p_mblk' to the end of the MBLK chain '*pp_mblk'.
 *      If the chain is empty then the new MBLK becomes the head of the chain
 *      and '*pp_mblk' is modified to point to the new MBLK.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void mblk_append_mblk(
    MBLK_T **pp_chain,
    MBLK_T *p_mblk
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      mblk_prepend_mblk
 *
 *  DESCRIPTION
 *      Prepends the MBLK 'p_first' to the start of the MBLK chain 'p_rest'.
 *      If the chain is empty then the new MBLK contains just p_first.
 *
 *  RETURNS
 *      The new MBLK (chain).
 *
 *----------------------------------------------------------------------------*/
MBLK_T *mblk_prepend_mblk(
    MBLK_T *p_first,
    MBLK_T *p_rest
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *  void mblk_getlen
 *
 *  DESCRIPTION
 *  Return total payload length of a message block chain
 *
 *  RETURNS/MODIFIES
 *
 *  Total length
 *----------------------------------------------------------------------------*/
extern uint16_t mblk_getlen(MBLK_T * mblk);

/*----------------------------------------------------------------------------*
 *  NAME
 *      mblk_get
 *
 *  DESCRIPTION
 *      Attempts to read 'bytes_to_read' bytes from the MBLK chain 'mblk',
 *      starting from position 'offset' bytes from the start of the chain,
 *      storing the bytes into the buffer 'p_buffer'.
 *
 *      This function does not modify the MBLK chain in any way.
 *
 *  RETURNS
 *      uint32_t - The number of bytes read
 *
 *----------------------------------------------------------------------------*/

uint32_t mblk_get(
    const MBLK_T *mblk,
    uint32_t offset,
    uint8_t *p_buffer,
    uint32_t bytes_to_read
    );
/*----------------------------------------------------------------------------*
 *  NAME
 *      mblk_read
 *
 *  DESCRIPTION
 *      Attempts to read 'bytes_to_read' bytes from the MBLK chain '*pp_mblk',
 *      storing the bytes into the buffer 'p_buffer'. As data is copied into the
 *      buffer, used MBLKs are released from the chain, and the 'pData' pointer
 *      in the current MBLK is updated to point to the next bye of unread data.
 *      The supplied MBLK pointer is modified to point to the new head of the
 *      chain.
 *
 *  RETURNS
 *      uint32_t - The number of bytes read.
 *
 *----------------------------------------------------------------------------*/

uint32_t mblk_read(
    MBLK_T **pp_mblk,
    uint8_t *p_buffer,
    uint32_t bytes_to_read
    );
/*----------------------------------------------------------------------------*
 *  NAME
 *      mblk_set_read_position
 *
 *  DESCRIPTION
 *      Attempts to advance the read position of the MBLK chain '*pp_mblk' by
 *      'read_position' bytes. As the read position advances, any discarded
 *      MBLKs are released from the chain, and the 'pData' pointer in the
 *      current MBLK is updated to point to the next byte of unread data. The
 *      supplied MBLK pointer is mofified to point to the new head of the chain.
 *
 *  RETURNS
 *      uint32_t - The number of bytes by which the read position was advanced.
 *
 *----------------------------------------------------------------------------*/

uint32_t mblk_set_read_position(
    MBLK_T **pp_mblk,
    uint32_t read_position
    );

#if defined(STR_LOG_DEBUG)
/*----------------------------------------------------------------------------*
 * NAME
 *     str_mblk_log
 *
 * DESCRIPTION
 *      Log the data contained within a message block chain to terminal device.
 *      This is ONLY called from the STRLOG macro. See above for details.
 *
 *      log_to_file - If TRUE output logging to file
 *      subsys      - Identifies subsystem calling STRLOG
 *      level       - Defines the type, or level, of logging performed
 *      nossprefix  - If TRUE suppresses output when SL_SHOWSS defined
 *      mblk        - pointer to the message block chain
 *
 *----------------------------------------------------------------------------*/

extern void str_mblk_log(
    bool_t log_to_file,
    uint32_t subsys,
    LoggingLevel_T level,
    bool_t nossprefix,
    char *text,
    MBLK_T *mblk
    );
#endif /* STR_LOG_DEBUG */



/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_convert_msgfrag_to_pmalloc
 *
 *  DESCRIPTION
 *  Grabs data from the buffer and puts it into pmalloc space instead.
 *
 *
 *  RETURNS/MODIFIES
 *
 *  pointer to pmalloced space.
 *  If discard is set, the mblk structure is discarded, if not the pData field is
 *  updated to refer to the pmalloced memory.
 *----------------------------------------------------------------------------*/
extern uint8_t* mblk_convert_msgfrag_to_pmalloc(MBLK_T * mblk,bool_t discard) ;

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_convert_msgfrags
 *
 *  DESCRIPTION
 *  Convert all the msgfrags in an entire mblk chain to pmalloced blocks
 *
 *
 *  RETURNS/MODIFIES
 *
 *----------------------------------------------------------------------------*/

extern void mblk_convert_msgfrags(MBLK_T * mblk);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_copy_msgfrag_to_buffer_at
 *
 *  DESCRIPTION
 *  Write a single mblk which is known to be a msgfrag into a buffer at pos
 *  bytes ahead of index. If you're not sure whether the mblk is a msgfrag,
 *  or if you wnat to write a chain of mblks then use mblk_flatten.
 *
 *  RETURNS/MODIFIES
 *  FALSE iff failed
 *
 *----------------------------------------------------------------------------*/

extern bool_t mblk_copy_msgfrag_to_buffer_at(BUFFER *b, uint16_t pos,
					   const MBLK_T *mblk);


/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_flatten
 *
 *  DESCRIPTION
 *  Write out a message block chain into a buffer
 *  (the buffer indices are left untouched)
 *
 *  RETURNS/MODIFIES
 *  FALSE iff failed
 *
 *----------------------------------------------------------------------------*/
extern bool_t mblk_flatten(const MBLK_T *mblk, BUFFER *b, uint16_t pos);

/*----------------------------------------------------------------------------*
 *  NAME
 *  mblk_map_msgfrag
 *
 *  DESCRIPTION
 *  Returns a pointer to a (temporary) address space where the contents of
 *  the mblk data can be inspected.
 *
 *
 *  RETURNS/MODIFIES
 *
 *----------------------------------------------------------------------------*/

extern uint8_t *  mblk_map_msgfrag(const MBLK_T * mblk) ;

#ifdef __cplusplus
}
#endif

#endif /* ndef __MBLK_H__ */
