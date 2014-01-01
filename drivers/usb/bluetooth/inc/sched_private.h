#ifndef SCHED_PRIVATE_H__
#define SCHED_PRIVATE_H__
/*******************************************************************************

                (c) Cambridge Silicon Radio Ltd, 2006
 	 				
				All rights reserved

FILE:			sched_private.h 

DESCRIPTION:    Private scheduler defines
  
REVISION:       $Revision: #1 $

*******************************************************************************/

#include "sched/sched.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MessageQueueEntryTag
{
    struct MessageQueueEntryTag	* next;
    uint16_t		          event;
    taskid                    sender;
    void			* message;
    msgid			id;
} MessageQueueEntryType;


typedef struct
{
    void	          * instanceDataPointer;
    MessageQueueEntryType * messageQueueFirst;
    MessageQueueEntryType * messageQueueLast;
    uint16_t                ext_qid;
} TaskInformationType;
#ifdef __cplusplus
}
#endif

#endif

