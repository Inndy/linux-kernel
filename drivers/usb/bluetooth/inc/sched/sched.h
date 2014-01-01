#ifndef __SCHED_H__
#define __SCHED_H__
/****************************************************************************

                (c) Cambridge Silicon Radio Ltd, 2006

                All rights reserved

FILE:           sched.h

DESCRIPTION:    The system scheduler gives structure to the bulk of the code in the
                system.   It organises the code as a set of tasks, and provides means
                for communication between the tasks.

                This is a non-preemptive system - there is no higher powered entity
                that forces one task to suspend so as to allow another to run.  Any
                time-critical code should be driven by system interrupts, which are
                of no concern to this scheduler.   This scheduler only supports code
                in the system's background, i.e.  it doesn't support the interrupts.

                Each task has two main elements: a set of message queues and a
                message handler function.   The message queues accumulate messages to
                be consumed by the message handler.   The message handler consumes
                messages from its queues and acts upon them.

                Each message has two elements: a uint16 and a void* pointer.   Much
                of the time the uint16 carries some form of event message and the
                void* pointer is null.   Sometimes it is convenient to have extra
                data in the void*.   The scheduler does not examine or alter
                either of the elements of each message.

                The core of the scheduler inspects each task's set of message queues
                in turn and calls the task's handler if there is any message in any
                of the task's queues.

                Key point: the message handler function must run to completion and
                return control to the scheduler.   Thus, to keep the system flowing,
                message handlers must hog the cpu for only a limited time.   In this
                respect the scheduling is similar to calling a STREAMS service
                procedure - described in:

                    cm.bell-labs.com/cm/cs/who/dmr/st.html

                The main gain of this approach, rather than a more conventional
                co-processing, non-preemptive scheduler, is that the system only
                requires one stack.  This should help constrain the ram consumption
                of this embedded application.

                This structure also appears appropriate for the current application,
                which is largely the handling of command messages which provoke
                a report or a change of configuration, or cause other elements of
                the local network of devices to do the same, thus the tasks are
                not cpu-intensive.

                A task with a heavy cpu load can timeslice its work by breaking the
                work into consecutive chunks, then performing each chunk in separate
                handler invocations.   The task might send itself a sequence of
                messages to provoke the sequence of invocations.

                A task's handler function isn't required to consume any of the
                messages pending for it.   It is also free to consume as many pending
                messages as it wishes.   Thus, if a message handler consumes no
                messages then the scheduler will call it again at the next time it
                inspects its queues.

                This scheduler uses a static list of tasks and a static set of
                message queues, so (unlike the first version of this scheduler) there
                is no need for scheduler primitives to create and destroy tasks.

                The scheduler stores a void* pointer for the internal use of each
                task.   The scheduler makes no use of the pointer except to pass it
                as an argument to each invocation of the task's message handler.
                This allows the system to maintain data between invocations of the
                handler rather than in static data.  This is expected to be of use
                only where there are multiple tasks which share a single message
                handler.

                Most tasks are expected to be structured as message-driven state
                machines.

                A task's message handler can post to any tasks' queues, but it can
                consume messages only from its own queues.

                Each task can have an initialisation function which is called exactly
                once before any task's main function is called.   The initialisation
                function is passed the same void* pointer that is passed to the main
                task function.

                The scheduling of tasks is naive: round-robin.

                All tasks run in a single address space; this isn't a real processor.

                It is intended that this scheduler's functions will be called only
                from the processor's background - in particular, communication with
                the foreground must not be via the scheduler's message queues.  (The
                scheduler code is not thread-safe - it uses no resource locks.)

                An expeditious bodge: the main body of the scheduler frequently
                examines one or more flags.   If such a flag is set the flag is
                cleared and a hard-wired function is called.   This is to allow
                the foreground to "interrupt" the background to tell it that it
                has data available to the background.   This avoids the need
                for the background to poll the foreground's data buffers.

****************************************************************************/

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
    Public Defines
 *============================================================================*/

/* TIME constants. */
#define MILLISECOND ((TIME)(1000))
#define SECOND      ((TIME)(1000 * MILLISECOND))
#define MINUTE      ((TIME)(60 * SECOND))

/*============================================================================*
    Public Data Types
 *============================================================================*/

/* System time, in microseconds. */
typedef uint32_t                TIME;

/* An identifier issued by the scheduler. */
typedef uint32_t                scheduler_identifier;

/* A task identifier. */
typedef uint16_t                taskid;

/* A queue identifier */
typedef uint16_t                qid;

/* A message identifier */
typedef scheduler_identifier    msgid;

/* A timer event identifier */
typedef scheduler_identifier    tid;

#define MAX_TASK_NAME_LENGTH    32

typedef struct
{
    void                        (*initFunction)(void **);
    void                        (*deinitFunction)(void **);
    void                        (*handlerFunction)(void **);
#if defined(STRING_LOG__) || defined(ENABLE_MIPS_MEASURE)
    char   taskName[MAX_TASK_NAME_LENGTH];
#endif
} TaskDefinitionType;

/* PRIM that identifies the environment */
#define ENVIRONMENT_TASK_ID     0xFFFF
#define ENV_PRIM                (ENVIRONMENT_TASK_ID)

/* NOT 8000h otherwise it will clash with BCSP off-chip bit */
#define EXTERNAL_TASK_MASK      0x4000
#define EXCLUDED_MODULE_QUEUE   0xFFFF

#if defined(STRING_LOG__) || defined(ENABLE_MIPS_MEASURE)
#define FUNCTION_TO_STRING(function) #function
#define DEFINE_TASK(queue, init, deinit, task) \
    if(queue < (sizeof(Tasks)/sizeof(TaskDefinitionType))) \
    { \
        Tasks[queue].initFunction = init; \
        Tasks[queue].deinitFunction = deinit; \
        Tasks[queue].handlerFunction = task; \
        strncpy(Tasks[queue].taskName, #task, MAX_TASK_NAME_LENGTH); \
    }
#else
#define DEFINE_TASK(queue, init, deinit, task) \
    if(queue < (sizeof(Tasks)/sizeof(TaskDefinitionType))) \
    { \
        Tasks[queue].initFunction = init; \
        Tasks[queue].deinitFunction = deinit; \
        Tasks[queue].handlerFunction = task; \
    }
#endif

typedef struct TimedEventTypeTag
{
    struct TimedEventTypeTag    *next;
    TIME            when;
    void            (*eventFunction)(uint16_t, void *);
    uint16_t        fniarg;
    void            *fnvarg;
    tid             id;
} TimedEventType;

typedef struct
{
    uint16_t        event;
    void            *message;
    qid             queue;
    tid             id;
} TimedMessageType;


/*===========================================================================*
  Public Functions
  *==========================================================================*/
extern TaskDefinitionType *sched_setup_tasks(uint16_t * numberOfTasks);

/*----------------------------------------------------------------------------*
 *  NAME
 *      register_bg_int
 *
 *  DESCRIPTION
 *      Register a background interrupt handler function with the scheduler.
 *        When bg_int*() is then called from the foreground (e.g. an interrupt
 *        routine) the registered function is called.
 *
 *        If "fn" is null then the interrupt is effectively disabled. If a
 *        function is already registered this function returns FALSE.
 *
 *        bgint_id identifies the bg_int* function to be registered (1-8).
 *
 *  RETURNS
 *      TRUE if function registered.
 *
 *----------------------------------------------------------------------------*/

extern bool_t register_bg_int(int bgint_id, void (* function)(void));

/*----------------------------------------------------------------------------*
 *  NAME
 *      bg_intn
 *
 *  DESCRIPTION
 *      Set background interrupt n.
 *
 *  RETURNS
 *      void.
 *
 *----------------------------------------------------------------------------*/
extern void bg_int1(void);
extern void bg_int2(void);
extern void bg_int3(void);
extern void bg_int4(void);
extern void bg_int5(void);
extern void bg_int6(void);
extern void bg_int7(void);
extern void bg_int8(void);
extern void bg_int9(void);
extern void bg_int10(void);

void init_sched(void);
void sched_stop(void);
TIME sched(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      get_time
 *
 *  DESCRIPTION
 *      Get the current system time.
 *
 *  RETURNS
 *      TIME - the current system time.
 *
 *----------------------------------------------------------------------------*/
extern TIME get_time(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_add
 *
 *  DESCRIPTION
 *      Add two time values. Adding the numbers can overflow the range of a
 *      TIME, so the user must be cautious.
 *
 *  RETURNS
 *      TIME - the sum of "t1" and "t2".
 *
 *----------------------------------------------------------------------------*/
#define time_add(t1, t2) ((t1) + (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_sub
 *
 *  DESCRIPTION
 *      Subtract two time values. Subtracting the numbers can provoke an
 *      underflow, so the user must be cautious.
 *
 *  RETURNS
 *      TIME - "t1" - "t2".
 *
 *----------------------------------------------------------------------------*/
#define time_sub(t1, t2)    ((int32_t) (t1) - (int32_t) (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_eq
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" equal "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define time_eq(t1, t2) ((t1) == (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_gt
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is greater than "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define time_gt(t1, t2) (time_sub((t1), (t2)) > 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_ge
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is greater than, or equal to "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define time_ge(t1, t2) (time_sub((t1), (t2)) >= 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_lt
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is less than "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define time_lt(t1, t2) (time_sub((t1), (t2)) < 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      time_le
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is less than, or equal to "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define time_le(t1, t2) (time_sub((t1), (t2)) <= 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      put_message
 *
 *  DESCRIPTION
 *      Sends a message consisting of the integer "mi" and the void * pointer
 *      "mv" to the message queue "q".
 *
 *      "mi" and "mv" are neither inspected nor changed by the scheduler - the
 *      task that owns "q" is expected to make sense of the values. "mv" may
 *      be null.
 *
 *  NOTE
 *      If "mv" is not null then it will typically be a chunk of pmalloc()ed
 *      memory, though there is no need for it to be so. Tasks should normally
 *      obey the convention that when a message built with pmalloc()ed memory
 *      is given to put_message() then ownership of the memory is ceded to the
 *      scheduler - and eventually to the recipient task. I.e., the receiver of
 *      the message will be expected to pfree() the message storage.
 *
 *  RETURNS
 *      msgid - message identifier, can be used with cancel_message().
 *
 *----------------------------------------------------------------------------*/
extern msgid put_message(
    qid q,
    uint16_t mi,
    void *mv
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      broadcast_message
 *
 *  DESCRIPTION
 *      Sends a message to all tasks.
 *
 *      The user must supply a "factory function" that is called once
 *      for every task that exists. The "factory function", msg_build_func,
 *      must allocate and initialise the message and set the msg_build_ptr
 *      to point to the message when done.
 *
 *  NOTE
 *      N/A
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
extern void broadcast_message(uint16_t mi,
                              void *(*msg_build_func)(void*),
                              void *msg_build_ptr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      get_message
 *
 *  DESCRIPTION
 *      Obtains a message from the message queue "q" if one is available. The
 *      calling task must own "q". The message consists of one or both of a
 *      uint16_t and a void *.
 *
 *      If the calling task does not own "q" then the scheduler calls panic().
 *
 *  RETURNS
 *      bool_t - TRUE if a message has been obtained from the queue, else FALSE.
 *      If a message is taken from the queue, then "*pmi" and "*pmv" are set to
 *      the "mi" and "mv" passed to put_message() respectively.
 *
 *      "pmi" and "pmv" can be null, in which case the corresponding value from
 *      them message is discarded.
 *
 *----------------------------------------------------------------------------*/

extern bool_t get_message(
    qid q,
    uint16_t *pmi,
    void **pmv
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      cancel_message
 *
 *  DESCRIPTION
 *      Attempts to prevent delivery of the message with message identifier
 *      "mid" which was previously sent onto message queue "q". Other arguments
 *      are as for get_message().
 *
 *      "mid" will have been returned from a call to put_message().
 *
 *  NOTES
 *      If the function returns TRUE then the caller of cancel_message() is
 *      responsible for discarding any pmalloc()ed memory thoughtfully.
 *
 *      A call to cancel_message() may fail because the target message has
 *      already been delivered.
 *
 *  RETURNS
 *      bool_t - TRUE if the message was caught in time, else FALSE.
 *
 *----------------------------------------------------------------------------*/
extern bool_t cancel_message(
    qid q,
    msgid mid,
    uint16_t *pmi,
    void **pmv
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      timed_event_at
 *
 *  DESCRIPTION
 *      Causes the void function "fn" to be called with the arguments
 *      "fniarg" and "fnvarg" at time "when".
 *
 *      The period to "when" must be less than half the range of a TIME.
 *
 *      timed_event_at() does nothing with "fniarg" and "fnvarg" except
 *      deliver them via a call to "fn()".   (Unless cancel_timed_event()
 *      is used to prevent delivery.)
 *
 *  NOTE
 *      The function will be called at or after "when"; the actual delay will
 *      depend on the timing behaviour of the scheduler's tasks.
 *
 *  RETURNS
 *      tid - A timed event identifier, can be used in cancel_timed_event().
 *
 *----------------------------------------------------------------------------*/
extern tid timed_event_at(
    TIME when,
    void (*fn) (uint16_t, void *),
    uint16_t fniarg,
    void *fnvarg
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      timed_event_in
 *
 *  DESCRIPTION
 *      Causes the void function "fn" to be called with the arguments
 *      "fniarg" and "fnvarg" after "delay" has elapsed.
 *
 *      "delay" must be less than half the range of a TIME.
 *
 *      timed_event_in() does nothing with "fniarg" and "fnvarg" except
 *      deliver them via a call to "fn()".   (Unless cancel_timed_event()
 *      is used to prevent delivery.)
 *
 *  NOTE
 *      The function will be called at or after "delay"; the actual delay will
 *      depend on the timing behaviour of the scheduler's tasks.
 *
 *  RETURNS
 *      tid - A timed event identifier, can be used in cancel_timed_event().
 *
 *----------------------------------------------------------------------------*/
extern tid timed_event_in(
    TIME delay,
    void (*fn) (uint16_t, void *),
    uint16_t fniarg,
    void *fnvarg
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      cancel_timed_event
 *
 *  DESCRIPTION
 *      Attempts to prevent the timed event with identifier "eventid" from
 *      occurring.
 *
 *  RETURNS
 *      bool_t - TRUE if cancelled, FALSE if the event has already occurred.
 *
 *----------------------------------------------------------------------------*/
extern bool_t cancel_timed_event(
    tid eventid,
    uint16_t *pmi,
    void **pmv
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      put_message_at
 *
 *  DESCRIPTION
 *      Sends a message consisting of the integer "mi" and the void* pointer
 *      "mv" to the message queue "q" at time "when".
 *
 *      "mi" and "mv" are neither inspected nor changed by the scheduler -
 *      the task that owns "q" is expected to make sense of the values.
 *      "mv" may be null.
 *
 *      The period to "when" must be less than half the range of a TIME.
 *
 *  RETURNS
 *      msgid - A message identifier, can be used with cancel_timed_message().
 *
 *----------------------------------------------------------------------------*/
extern msgid put_message_at(
    TIME when,
    qid q,
    uint16_t mi,
    void *mv
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      put_message_in
 *
 *  DESCRIPTION
 *      See comments for put_message_at().   This is identical, except that
 *      the message is delivered to "q" after period "delay" (from now)
 *      rather than at a specified time.
 *
 *      The value of "delay" must be less than half the range of a TIME.
 *
 *  RETURNS
 *      msgid -
 *
 *----------------------------------------------------------------------------*/
extern msgid put_message_in(
    TIME delay,
    qid q,
    uint16_t mi,
    void *mv
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      cancel_timed_message
 *
 *  DESCRIPTION
 *      Attempts to prevent delivery of the message with message identifier
 *      "mid" which was previously scheduled to be delivered to message
 *      queue "q" using either put_message_at() or put_message_in().
 *
 *      Other arguments are as for get_message().
 *
 *      "mid" will have been returned from a call to put_message_at() or
 *      put_message_in().
 *
 *  RETURNS
 *      bool_t - TRUE if the message was caught in time, else FALSE.
 *
 *----------------------------------------------------------------------------*/
extern bool_t cancel_timed_message(
    qid q,
    msgid mid,
    uint16_t *pmi,
    void **pmv
    );

void register_external_send(bool_t (*f)(qid ,uint16_t,void *));

void sched_init_tasks(int no_tasks, TaskDefinitionType * tasks);

uint16_t sched_number_of_tasks(void);
const char * sched_task_name(uint16_t taskid);

void deinit_bg_ints(void);

void deinit_sched(void);
void sched_task_deinit(void);

qid sched_get_task_queue(void);

#ifdef __cplusplus
}
#endif

#endif
