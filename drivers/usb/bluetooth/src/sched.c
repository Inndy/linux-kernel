/******************************************************************************

                 (c) Cambridge Silicon Radio Ltd, 2006

                 All rights reserved

FILE:            sched.c

DESCRIPTION:     Scheduler implementation

REVISION:        $Revision: #2 $

******************************************************************************/

#include <linux/kernel.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/semaphore.h>

#include "../inc/sched/panic.h"
#include "../inc/sched/pmalloc.h"
#include "../inc/sched/sched.h"
#include "../inc/sched_private.h"

#ifdef STRING_LOG__
#include "log.h"
#include "log_private.h"
#endif

#define WRAP (4289999000)

typedef struct timedEventType
{
    struct timedEventType    *next;
    TIME                     when;
    uint32_t                 wrapCount;
    void                     (*eventFunction)(uint16_t, void *);
    uint16_t                 fniarg;
    void                     *fnvarg;
    tid                      id;
} timedEventType;

static bool_t (*external_send)(qid q,uint16_t mi,void *mv) = NULL;
static void deliver_timed_message(uint16_t notUsed, void *v);

static TaskInformationType *sched_tasks;
static timedEventType * timedEvents = NULL;

qid currentTaskId;
static tid currentTid;
static uint16_t    currentNumberOfPendingMessages;

static spinlock_t scheduler_access = SPIN_LOCK_UNLOCKED;
DECLARE_WAIT_QUEUE_HEAD(notIdleCondition);

static uint32_t currentWrapCount = 0;
static TIME     lastNow = 0;
extern TIME     realNow;

#define MAX_SCHED_ID        ((scheduler_identifier)(~0x0UL))
#define WRAPPED_SCHED_ID_START    (MAX_SCHED_ID / 4)

static scheduler_identifier next_scheduler_identifier = 1;

static uint16_t sched_running;
static DECLARE_COMPLETION(schedd_exited);

extern TaskDefinitionType *sched_setup_tasks(uint16_t *no);
uint16_t number_of_tasks;
TaskDefinitionType *Tasks;


/* ------------------------ Background Interrupts -------------------------- */
#define NUMBER_OF_BG_INTS    7
#define BG_INT_1            0x00
#define BG_INT_2            0x01
#define BG_INT_3            0x02
#define BG_INT_4            0x03
#define BG_INT_5            0x04
#define BG_INT_6            0x05
#define BG_INT_7            0x06

typedef void (* BgIntFuncType)(void);
static BgIntFuncType    bgIntFunctions[NUMBER_OF_BG_INTS];
static unsigned long    bgInts;

bool_t register_bg_int(int vector, BgIntFuncType bgIntFunction)
{
    bool_t retval;
    unsigned long flags;

    if ((vector < 1) || (vector > NUMBER_OF_BG_INTS))
    {
        printk("<1>SCHED: Invalid vector in register_bg_int(%d)!\n", vector);
        retval = FALSE;
    }
    else
    {
        spin_lock_irqsave(&scheduler_access,flags);
        bgIntFunctions[vector - 1] = bgIntFunction;
        spin_unlock_irqrestore(&scheduler_access,flags);
        retval = TRUE;
    }

    return(retval);
}

void bg_int1(void)
{
    set_bit(BG_INT_1,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}

void bg_int2(void)
{
    set_bit(BG_INT_2,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}

void bg_int3(void)
{
    set_bit(BG_INT_3,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}

void bg_int4(void)
{
    set_bit(BG_INT_4,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}

void bg_int5(void)
{
    set_bit(BG_INT_5,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}

void bg_int6(void)
{
    set_bit(BG_INT_6,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}

void bg_int7(void)
{
    set_bit(BG_INT_7,&bgInts);
    wake_up_interruptible(&notIdleCondition);
}
#if 0 /* by ilee start */
static void serviceBgInts(void)
{
    int i;

    if (bgInts)
    {
        for(i=0;((i<NUMBER_OF_BG_INTS) && (bgInts));i++)
        {
            if (test_bit(i,&bgInts))
            {
                clear_bit(i,&bgInts);

                if (bgIntFunctions[i])
                {
                    bgIntFunctions[i]();
                }
                else
                {
                    /* do nothing */
                }
            }
            else
            {
                /* Do nothing */
            }
        }
    }
    else
    {
        /* Do nothing */
    }
}

/* --------------------- Internal Scheduler functions ---------------------- */
static void defaultHandlerFunction(void **gash)
{
    sched_panic(PANIC_NULL_TASK_HANDLER);
}

scheduler_identifier get_scheduler_identifier(void)
{
    scheduler_identifier i;
    unsigned long flags;

    spin_lock_irqsave(&scheduler_access,flags);

    i = next_scheduler_identifier;
    if (++next_scheduler_identifier == MAX_SCHED_ID)
    {
        next_scheduler_identifier = WRAPPED_SCHED_ID_START;
    }
    spin_unlock_irqrestore(&scheduler_access,flags);

    return i;
}


void init_sched(void)
{
    uint16_t taskIndex;

    printk("<1>SCHED: init_sched\n");
    Tasks = sched_setup_tasks(&number_of_tasks);

#ifdef STRING_LOG__
    logInitSched(ENVIRONMENT_TASK_ID);
#endif

    /* Initialize sched_tasks and set default values */
    sched_tasks = kmalloc(number_of_tasks * sizeof(TaskInformationType),
                          GFP_KERNEL);
    if(!sched_tasks)
    {
        sched_panic(PANIC_HEAP_EXHAUSTION);
        return;
    }

    for (taskIndex = 0; taskIndex < number_of_tasks; taskIndex++)
    {
        sched_tasks[taskIndex].messageQueueFirst   = NULL;
        sched_tasks[taskIndex].messageQueueLast    = NULL;
        sched_tasks[taskIndex].instanceDataPointer = NULL;
        sched_tasks[taskIndex].ext_qid             = 0xffff;
    }

    currentNumberOfPendingMessages = 0;
    currentTaskId = 0;
}

static int do_sched(void *param)
{
    qid                taskIndex;
    timedEventType    *timedEvent = NULL;
    TIME           timeUntilNextTimedEvent;
    TIME               nextTimedEventTime;
    TIME               now;
    unsigned long      flags;

    printk("<1>SCHED: running scheduler...\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    daemonize("schedd");
#else
    daemonize();
#endif

#ifdef STRING_LOG__
    logSched();
#endif

    for (taskIndex = 0; taskIndex < number_of_tasks; taskIndex++)
    {
#ifdef SCHED_DEBUG_LEVEL_1
        printk("<1>: %d: 0x%p, 0x%p, 0x%p\n",
               taskIndex,
               Tasks[taskIndex].initFunction,
               Tasks[taskIndex].handlerFunction,
               Tasks[taskIndex].deinitFunction);
#endif
        if (Tasks[taskIndex].initFunction)
        {
            currentTaskId = taskIndex;

#ifdef STRING_LOG__
            logInitTask(taskIndex);
#endif

            Tasks[taskIndex].initFunction(&sched_tasks[taskIndex].instanceDataPointer);
            currentTaskId = ENVIRONMENT_TASK_ID;
        }
    }

    sched_running = 1;

    while (sched_running)
    {
        for (taskIndex = 0; taskIndex < number_of_tasks; taskIndex++)
        {
            while (timedEvents)
            {
                serviceBgInts();
                spin_lock_irqsave(&scheduler_access,flags);

                if (!timedEvents)
                {
                    spin_unlock_irqrestore(&scheduler_access,flags);
                    break;
                }

                                /* Now, back to timed events. */
                now = get_time();

                if (now<lastNow)
                {
                    currentWrapCount++;
                }

                lastNow = now;

                if (timedEvents->wrapCount > currentWrapCount)
                {
                    spin_unlock_irqrestore(&scheduler_access,flags);
                    break;
                }
                else
                {
                    if ((timedEvents->when>now) &&
                        (timedEvents->wrapCount == currentWrapCount))
                    {
                        spin_unlock_irqrestore(&scheduler_access,flags);
                        break;
                    }
                    else
                    {
                        timedEvent = timedEvents;
                        timedEvents = timedEvent->next;
                        spin_unlock_irqrestore(&scheduler_access,flags);

                        currentTid = timedEvent->id;
#ifdef STRING_LOG__
                        if(timedEvent->eventFunction != deliver_timed_message)
                        {
                            logTimedEventFire(timedEvent->id,
                                              timedEvent->fniarg,
                                              psizeof(timedEvent->fnvarg),
                                              timedEvent->fnvarg);
                        }
#endif
                        timedEvent->eventFunction(timedEvent->fniarg,
                                                  timedEvent->fnvarg);
                        pfree(timedEvent);
                    }
                }
            }
            serviceBgInts();
            spin_lock_irqsave(&scheduler_access,flags);

            if (sched_tasks[taskIndex].messageQueueFirst)
            {
                spin_unlock_irqrestore(&scheduler_access,flags);

                currentTaskId = taskIndex;

#ifdef STRING_LOG__
                logActivate(taskIndex);
#endif

                Tasks[taskIndex].handlerFunction(&sched_tasks[taskIndex].instanceDataPointer);

#ifdef STRING_LOG__
                logDeactivate(taskIndex);
#endif

                currentTaskId = ENVIRONMENT_TASK_ID;
            }
            else
            {
                spin_unlock_irqrestore(&scheduler_access,flags);
            }
        }

        spin_lock_irqsave(&scheduler_access,flags);

        if ((currentNumberOfPendingMessages == 0) && (bgInts == 0))
        {
            if (timedEvents)
            {
                now = get_time();

                if(timedEvents->wrapCount > currentWrapCount)
                {
                    timeUntilNextTimedEvent = (TIME)(WRAP - now +
                                                     timedEvents->when);
                }
                else
                {
                    if (now<timedEvents->when)
                    {
                        timeUntilNextTimedEvent = timedEvents->when - now;
                    }
                    else
                    {
                        timeUntilNextTimedEvent = 0;
                    }
                }

                if (timeUntilNextTimedEvent>0)
                {
                    spin_unlock_irqrestore(&scheduler_access,flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                    wait_event_interruptible_timeout(notIdleCondition,
                                                     ((bgInts > 0) ||
                                                      (currentNumberOfPendingMessages > 0) ||
                                                      (sched_running==0)),
                                                     (HZ*timeUntilNextTimedEvent/1000000));
#else
                    interruptible_sleep_on_timeout(&notIdleCondition,
                                                   (HZ*timeUntilNextTimedEvent/1000000));
#endif
                }
                else
                {
                    spin_unlock_irqrestore(&scheduler_access,flags);
                }
            }
            else
            {
                spin_unlock_irqrestore(&scheduler_access,flags);

                wait_event_interruptible(notIdleCondition,
                                         ((bgInts > 0) ||
                                          (currentNumberOfPendingMessages>0) ||
                                          (sched_running==0)));
            }
        }
        else
        {
            spin_unlock_irqrestore(&scheduler_access,flags);
        }
    }

    complete_and_exit(&schedd_exited,0);
}

TIME sched(void)
{
    kernel_thread(do_sched,NULL,0);

    return(0);
}

static msgid do_put_message(qid q, uint16_t mi, void *mv, msgid id)
{
    MessageQueueEntryType * message;
    msgid message_id;
    unsigned long flags;

    /* Is this an external queue */
    if (q & EXTERNAL_TASK_MASK)
    {
        if (external_send)
        {
            external_send(q,mi,mv);
        }
        else
        {
            /* Do nothing */
        }

        message_id = 0;
    }
    else
    {
        if (q >= number_of_tasks)
        {
            sched_panic(PANIC_UNKNOWN_TASK);
        }

        message = pmalloc(sizeof(MessageQueueEntryType));
        message->next = NULL;
        message->event = mi;
        message->sender = currentTaskId;
        message->message = mv;
        message->id = id;

        message_id = id;

        spin_lock_irqsave(&scheduler_access,flags);

        if (sched_tasks[q].messageQueueFirst == NULL)
        {
            sched_tasks[q].messageQueueFirst = sched_tasks[q].messageQueueLast = message;
        }
        else
        {
            sched_tasks[q].messageQueueLast->next = message;
            sched_tasks[q].messageQueueLast = sched_tasks[q].messageQueueLast->next;
        }

        currentNumberOfPendingMessages++;

        spin_unlock_irqrestore(&scheduler_access,flags);

        wake_up_interruptible(&notIdleCondition);
    }

#ifdef STRING_LOG__
    logPutMessage(currentTaskId, q, id, mi, psizeof(mv), mv);
#endif

    return message_id;
}

static void deliver_timed_message(uint16_t notUsed, void *v)
{
    TimedMessageType * timedMessage = (TimedMessageType *) v;

    do_put_message(timedMessage->queue, timedMessage->event, timedMessage->message, currentTid);

    pfree(timedMessage);
}


/* ---------------------- Public Scheduler functions ----------------------- */
bool_t get_message(qid q, uint16_t *pmi, void **pmv)
{
    MessageQueueEntryType * message;
    unsigned long flags;

    if (q >= number_of_tasks)
    {
        sched_panic(PANIC_UNKNOWN_TASK);
    }

    if (q != currentTaskId)
    {
        sched_panic(PANIC_QUEUE_ACCESS_VIOLATION);
    }

    spin_lock_irqsave(&scheduler_access,flags);

    message = sched_tasks[q].messageQueueFirst;

    if (message)
    {
        sched_tasks[q].messageQueueFirst = message->next;
        currentNumberOfPendingMessages--;

        spin_unlock_irqrestore(&scheduler_access,flags);

        if (pmi)
        {
            *pmi = message->event;
        }
        if (pmv)
        {
            *pmv = message->message;
        }

#ifdef STRING_LOG__
        logGetMessage(message->sender,q,message->id,*pmi,sizeof(*pmv),*pmv,TRUE);
#endif

        pfree(message);

        return TRUE;
    }
    spin_unlock_irqrestore(&scheduler_access,flags);

    *pmi = 0;
    *pmv = NULL;
    return FALSE;
}

#ifdef STRING_LOG__
static bool_t  enableCancelMessageLog = TRUE;
#endif

bool_t cancel_message(qid q, msgid mid, uint16_t *pmi, void **pmv)
{
    MessageQueueEntryType * searchPointer;
    MessageQueueEntryType * previousPointer = NULL;
    unsigned long flags;
    taskid senderOfMsgToCancel = 0;

    if (q >= number_of_tasks)
    {
        sched_panic(PANIC_UNKNOWN_TASK);
    }

    spin_lock_irqsave(&scheduler_access,flags);
    for (searchPointer = sched_tasks[q].messageQueueFirst; searchPointer != NULL; searchPointer = searchPointer->next)
    {
        if (mid == searchPointer->id)
        {
            if (previousPointer == NULL)
            {
                sched_tasks[q].messageQueueFirst = searchPointer->next;
            }
            else
            {
                previousPointer->next = searchPointer->next;
                if (searchPointer == sched_tasks[q].messageQueueLast)
                {
                    sched_tasks[q].messageQueueLast = previousPointer;
                }
            }
            currentNumberOfPendingMessages--;
            spin_unlock_irqrestore(&scheduler_access,flags);

            if (pmi)
            {
                *pmi = searchPointer->event;
                senderOfMsgToCancel = searchPointer->sender;
            }
            if (pmv)
            {
                *pmv = searchPointer->message;
            }

            pfree(searchPointer);

#ifdef STRING_LOG__
            logCancelMessage(senderOfMsgToCancel,q, mid, *pmi, psizeof(*pmv),*pmv, TRUE);
#endif

            return TRUE;
        }

        previousPointer = searchPointer;
    }
    spin_unlock_irqrestore(&scheduler_access,flags);

#ifdef STRING_LOG__
    logCancelMessage(currentTaskId,q, (msgid) 0, 0, 0, NULL, FALSE);
#endif

    return FALSE;
}

#ifdef STRING_LOG__
static bool_t  enableTimedEventAtLog = FALSE;
#endif
tid timed_event_at(TIME when, void (*fn) (uint16_t, void *), uint16_t fniarg, void *fnvarg)
{
    timedEventType * timedEvent;
    unsigned long flags;
    timedEventType *ptr;
    timedEventType *prev;

    if (when > WRAP)
    {
        when %= WRAP;
    }
    else
    {
        if (when < realNow)
        {
            when += (0xffffffff - WRAP);
        }
        else
        {
            /* Do nothing */
        }
    }

    timedEvent = pmalloc(sizeof(timedEventType));
    timedEvent->when = when;
    timedEvent->wrapCount = currentWrapCount;
    timedEvent->eventFunction = fn;
    timedEvent->fniarg = fniarg;
    timedEvent->fnvarg = fnvarg;
    timedEvent->id = (tid) get_scheduler_identifier();

    /* Has time wrapped */
    if (when<realNow)
    {
        timedEvent->wrapCount++;
    }
    else
    {
        /* Do nothing */
    }

    spin_lock_irqsave(&scheduler_access,flags);
    if (!timedEvents)
    {
        timedEvent->next = NULL;
        timedEvents = timedEvent;
    }
    else
    {
        ptr = timedEvents;
        prev = NULL;

        while(ptr)
        {
            if(((ptr->when>=timedEvent->when) &&
                (ptr->wrapCount >= timedEvent->wrapCount)) ||
               (ptr->wrapCount>timedEvent->wrapCount))

            {
                break;
            }
            else
            {
                prev = ptr;
                ptr=ptr->next;
            }
        }

        if (prev == NULL)
        {
            timedEvent->next = timedEvents;
            timedEvents= timedEvent;
        }
        else
        {
            prev->next = timedEvent;
            timedEvent->next = ptr;
        }
    }
    spin_unlock_irqrestore(&scheduler_access,flags);

    wake_up_interruptible(&notIdleCondition);

#ifdef STRING_LOG__
    logTimedEventAt(currentTaskId, timedEvent->id, when, fniarg, psizeof(fnvarg), fnvarg);
#endif

    return timedEvent->id;
}

tid timed_event_in(TIME delay, void (*fn) (uint16_t, void *), uint16_t fniarg, void *fnvarg)
{
    tid tmpTid;

#ifdef STRING_LOG__
    enableTimedEventAtLog = FALSE;
#endif

    tmpTid = timed_event_at(time_add(get_time(), delay), fn, fniarg, fnvarg);
#ifdef STRING_LOG__
    enableTimedEventAtLog = TRUE;
    logTimedEventIn(currentTaskId, tmpTid, delay, fniarg, psizeof(fnvarg), fnvarg);
#endif

    return tmpTid;
}

#ifdef STRING_LOG__
static bool_t enableCancelTimedEventLog = TRUE;
#endif

bool_t cancel_timed_event(tid eventid, uint16_t *pmi, void **pmv)
{
    timedEventType * searchPointer;
    timedEventType * previousPointer = NULL;
    unsigned long flags;

    spin_lock_irqsave(&scheduler_access,flags);

    for (searchPointer = timedEvents; searchPointer != NULL; searchPointer = searchPointer->next)
    {
        if (eventid == searchPointer->id)
        {
            if (previousPointer == NULL)
            {
                timedEvents = searchPointer->next;
            }
            else
            {
                previousPointer->next = searchPointer->next;
            }

            spin_unlock_irqrestore(&scheduler_access,flags);

            if (pmi)
            {
                *pmi = searchPointer->fniarg;
            }
            if (pmv)
            {
                *pmv = searchPointer->fnvarg;
            }

#ifdef STRING_LOG__
            if (enableCancelTimedEventLog)
            {
                logCancelTimedEvent(currentTaskId, eventid, searchPointer->fniarg,
                                    psizeof(searchPointer->fnvarg), searchPointer->fnvarg, TRUE);
            }
#endif

            pfree(searchPointer);

            return TRUE;
        }

        previousPointer = searchPointer;
    }

    spin_unlock_irqrestore(&scheduler_access,flags);

#ifdef STRING_LOG__
    if (enableCancelTimedEventLog)
    {
        logCancelTimedEvent(currentTaskId, eventid, 0, 0, NULL, FALSE);
    }
#endif

    return FALSE;
}

void broadcast_message(uint16_t mi, void*(*msg_build_func)(void* ptr),
                       void *msg_build_ptr)
{
    void *msg;
    qid t;

    if (msg_build_func)
    {
        for(t=0;t<number_of_tasks;t++)
        {
            if (Tasks[t].handlerFunction != defaultHandlerFunction)
            {
                msg = msg_build_func(msg_build_ptr);

                do_put_message(t,mi,msg,(msgid) get_scheduler_identifier());
            }
            else
            {
                /* Do nothing */
            }
        }
    }
    else
    {
        /* Do nothing */
    }
}

msgid put_message(qid q, uint16_t mi, void *mv)
{
    return do_put_message(q, mi, mv, (msgid) get_scheduler_identifier());
}

#ifdef STRING_LOG__
static bool_t enablePutMessageAtLog = TRUE;
#endif

msgid put_message_at(TIME when, qid q, uint16_t mi, void *mv)
{
    TimedMessageType * timedMessage = pmalloc(sizeof(TimedMessageType));
    msgid tmpMsgId;

    timedMessage->queue = q;
    timedMessage->event = mi;
    timedMessage->message = mv;

    /* Deliver message via a timed event. */
    tmpMsgId = (msgid) timed_event_at(when, deliver_timed_message, 0, (void *)(timedMessage));

#ifdef STRING_LOG__
    if (enablePutMessageAtLog)
    {
        logPutMessageAt(currentTaskId, q, tmpMsgId, when, mi, psizeof(mv), mv);
    }
#endif

    return (tmpMsgId);
}

msgid put_message_in(TIME delay, qid q, uint16_t mi, void *mv)
{
    msgid tmpMsgId;

#ifdef STRING_LOG__
    enablePutMessageAtLog = FALSE;
#endif

    tmpMsgId = put_message_at( time_add( get_time(), delay ), q, mi, mv );

#ifdef STRING_LOG__
    enablePutMessageAtLog = TRUE;
    logPutMessageIn(currentTaskId, q, 0, delay, mi, psizeof(mv), mv);
#endif

    return (tmpMsgId);
}

bool_t cancel_timed_message(qid q, msgid mid, uint16_t *pmi, void **pmv)
{
    bool_t    tmpResult;
    TimedMessageType * timedMessage;

#ifdef STRING_LOG__
    enableCancelTimedEventLog = FALSE;
#endif

    tmpResult = cancel_timed_event((tid) mid,  NULL, (void **) &timedMessage);

#ifdef STRING_LOG__
    enableCancelTimedEventLog = TRUE;
#endif

    if (tmpResult)
    {
        if (pmi != NULL)
        {
            *pmi = timedMessage->event;
        }
        if (pmv != NULL)
        {
            *pmv = timedMessage->message;
        }
        pfree(timedMessage);
#ifdef STRING_LOG__
        logCancelTimedMessage(currentTaskId, q, mid, *pmi, psizeof(*pmv), *pmv, TRUE);
#endif

        return TRUE;
    }

#ifdef STRING_LOG__
    enableCancelMessageLog = FALSE;
#endif

    tmpResult = cancel_message(q, mid, pmi, pmv);

#ifdef STRING_LOG__
    enableCancelMessageLog = TRUE;
#endif

    if (tmpResult)
    {
#ifdef STRING_LOG__
        logCancelTimedMessage(currentTaskId,q,mid, *pmi, psizeof(*pmv), *pmv, TRUE);
#endif

        return TRUE;
    }

#ifdef STRING_LOG__
    logCancelTimedMessage(currentTaskId,q,mid,0,0,NULL,FALSE);
#endif

    return(FALSE);
}

uint16_t get_task_id(void)
{
    return(currentTaskId);
}

uint16_t map_queue(uint16_t ext_qid)
{
    int i;
    uint16_t int_qid;

    int_qid = 0xffff;

    for(i=0;i<number_of_tasks;i++)
    {
        if (sched_tasks[i].ext_qid == ext_qid)
        {
            int_qid = i;
            break;
        }
        else
        {
            /* Do nothing */
        }
    }

    return(int_qid);
}

void set_external_tid(uint16_t tid, uint16_t ext_tid)
{
    sched_tasks[tid].ext_qid = ext_tid;
}

uint16_t get_external_tid(void)
{
    return(sched_tasks[currentTaskId].ext_qid);
}

void deinit_sched(void)
{
#ifdef STRING_LOG__
    logStop();
#endif

    kfree(sched_tasks);
}


#ifdef STRING_LOG__
uint16_t sched_number_of_tasks(void)
{
    return number_of_tasks;
}

const char * sched_task_name(uint16_t taskid)
{
	if (taskid < number_of_tasks)
	{
		return Tasks[taskid].taskName;
	}
	else
	{
		return "UNKNOWN";
	}
        return "UNKNOWN";
}
#endif /* STRING_LOG__ */



void sched_init_tasks(int no_tasks, TaskDefinitionType *t)
{
    uint16_t i;

    for(i=0;i<no_tasks;i++)
    {
        t[i].initFunction=NULL;
        t[i].handlerFunction=defaultHandlerFunction;
        t[i].deinitFunction=NULL;
#ifdef STRING_LOG__
        t[i].taskName[0] = '\0';
#endif
    }
}

void register_external_send(bool_t (*f)(qid ,uint16_t,void *))
{
    external_send = f;
}

void sched_stop(void)
{
    printk("<1>SCHED: sched_stop called\n");

    if (sched_running)
    {
        /* Stopping scheduler */
        printk("<1>SCHED: Stopping scheduler...: %d\n",sched_running);
        sched_running = 0;
        wake_up_interruptible(&notIdleCondition);
        wait_for_completion(&schedd_exited);
        printk("<1>SCHED: Scheduler stopped.\n");
    }
    else
    {
        printk("<1>SCHED: Scheduler not running\n");
    }
}

void sched_task_deinit(void)
{
    int taskIndex;
    timedEventType *timedEvent;

#ifdef STRING_LOG__
    logText(0xffff, "-------------- CLEAN UP --------------");
    logText(0xffff, "Call task deinit function");
#endif

    printk("<1>SCHED: Call deinit for all tasks\n");
    /* Let all tasks, cleanup what they know to clean up */
    for (taskIndex = 0; taskIndex < number_of_tasks; taskIndex++)
    {
        if (Tasks[taskIndex].deinitFunction != NULL)
        {
            currentTaskId = taskIndex;

#ifdef STRING_LOG__
            logDeinitTask(taskIndex);
#endif

            Tasks[taskIndex].deinitFunction(&sched_tasks[taskIndex].instanceDataPointer);
        }
        else
        {
            /* Do nothing */
        }
    }

#ifdef STRING_LOG__
    logText(0xffff, "-------------- CLEAN UP --------------");
    logText(0xffff, "Free any remaining timed events");
#endif

    while(timedEvents)
    {
        timedEvent = timedEvents;
        timedEvents = timedEvent->next;
        pfree(timedEvent);
    }

#ifdef STRING_LOG__
    logText(0xffff, "-------- CLEANED UP (Sched exit) --------");
#endif
}

qid sched_get_task_queue(void)
{
    return(currentTaskId);
}
#endif /* by ilee end */