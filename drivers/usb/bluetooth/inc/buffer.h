#ifndef __BUFFER_H__
#define __BUFFER_H__
/****************************************************************************

                (c) CSR plc 2007

                All rights reserved

FILE:           buffer.h  -  message buffers

REVISION:       $Revision: #2 $

DESCRIPTION:

************************************************************************/

#include "sched/sched.h"
#include "sched/types.h"
#include "bluetooth.h"
#include "hci.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A buffer handle. 1 or 2 of these are attached to a buffer. */
typedef uint16_t bufhandle;

/* bufhandle zero is never used. The value zero signals "no handle". */
#define NO_BUFHANDLE (0)

/* Opaque structures */
typedef struct MSGSET MSGSET;

/****************************************************************************
MSGSTART  -  a message delimiter
*/

typedef struct msgstart {
    struct msgstart    *next;    /* next in linked list */
    uint16_t start;
} MSGSTART;

/****************************************************************************
BUFFER - a unidirectional data buffer.
*/

typedef    struct BUFFER {
    bufhandle       wrh;        /* write */
    bufhandle       rdh;        /* read */
    uint16_t        index;        /* valid data high mark */
    uint16_t        outdex;        /* where to read from next */
    uint16_t        tail;        /* buffer deleted under here */
    MSGSET          *msgset;    /* message delimiters */
    uint8_t        *theBuffer;    /* pmalloc'ed memory */
    uint16_t        bufSize;    /* the allocated buffer size */
} BUFFER;

/****************************************************************************
MSGFRAG  -  a message fragment
*/

typedef struct MSGFRAG {
    BUFFER          *b;        /* The buffer that holds the frag. */
    uint16_t        start;        /* Buffer index of frag start. */
    uint16_t        len;        /* Length of frag. */
    bool_t          msg_start;    /* Is frag first in a message? */
} MSGFRAG;

/****************************************************************************
NAME
    buf_get_size  -  how many bytes are stored in a buffer

RETURNS
    An estimate of the number of bytes currently stored in the buffer
    "b", or zero if b is invalid.
*/

extern uint16_t buf_get_size( BUFFER *b );

/****************************************************************************
NAME
    buf_map_msgfrag  -  make msgfrag legible from cpu addr space

RETURNS
    Pointer to start of the fragment in 8 bit wide memory or NULL if
    something went wrong.
*/

extern uint8_t *buf_map_msgfrag(MSGFRAG *msgfrag);

/****************************************************************************
NAME
    buf_clear_msgfrag_out_of_order  -  clear msgfrag, not necc the head
*/

extern void buf_clear_msgfrag_out_of_order( MSGFRAG *msgfrag ) ;

#include "hostio.h"

#ifdef __cplusplus
}
#endif

#endif

