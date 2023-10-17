/**
 * @file   misc_timer2.c
 * @author fog
 * @date   Fri Mar 12 23:54:52 2010
 * 
 * @brief  
 * 
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "misc_timer2.h"

/* #define DEBUG */

#ifdef DEBUG
#define DPRINTF(fmt, args...) printf(fmt, ##args)
#else
#define DPRINTF(fmt, args...)
#endif

/** This macro will evaluate TRUE if a is earlier than b */
#define IS_LE_TIMEVAL(a, b) (((a)->tv_sec <= (b)->tv_sec) ||    \
                             (((a)->tv_sec == (b)->tv_sec) &&   \
                              ((a)->tv_usec <= (b)->tv_usec)))

#define IS_ZERO_TIMEVAL(a) (((a)->tv_sec == 0)&&((a)->tv_usec == 0))

typedef struct timerEvent {
    struct timerEvent *next;
    int                interval; /**< millon second */
    int                count;    /**< -1 means loop forever */
    struct timeval     lastTime; /**< last execute time */
    eventFunc_t        func;     /**< handler func to call when event expires. */
    void              *ctxArg;   /**< context data to pass to func */
    char               name[EVENT_TIMER_NAME_LENGTH];
} timerEvent_t;

/** Internal timer handle. */
typedef struct timerHandle
{
   timerEvent_t  *events;       /**< Singly linked list of events */
   int            number;       /**< Number of events in this handle. */
} timerHandle_t;

static int getTimeval(struct timeval *tv)
{
    int n;
    
    n = gettimeofday(tv, NULL);

    return n;
}

static struct timeval tvAdd(struct timeval *tv, int ms)
{
    int sec, msec;
    struct timeval tvRet;
    
    sec = ms / MSECS_IN_SEC;
    msec = (ms % MSECS_IN_SEC) * MSECS_IN_SEC;

    tvRet.tv_sec  = tv->tv_sec + sec;
    tvRet.tv_usec = tv->tv_usec + msec;

    return tvRet;
}

static void addMillionSec(struct timeval *tv, int ms)
{
    int sec, msec;

    sec = ms / MSECS_IN_SEC;
    msec = (ms % MSECS_IN_SEC) * MSECS_IN_SEC;

    tv->tv_sec += sec;
    tv->tv_usec += msec;

    return;
}

static int isEventPresent(void *handle, eventFunc_t func, void *ctxArg)
{
    const timerHandle_t *th = (const timerHandle_t *)handle;
    timerEvent_t *event;
    
    event = th->events;

    while(event != NULL)
    {
        if(event->func == func && event->ctxArg == ctxArg)
            break;
        event = event->next;
    }

    return (event != NULL);
}

int mTimer_init(void **handle)
{
    *handle = calloc(1, sizeof(timerHandle_t));
    if(*handle == NULL)
    {
        perror("malloc");
        return -1;
    }
    
    return 0;
}

void mTimer_cleanup(void **handle)
{
    timerHandle_t *th = (timerHandle_t *)(*handle);
    timerEvent_t *event;

    while((event = th->events) != NULL)
    {
        th->events = event->next;
        free(event);
    }

    free(*handle);

    return;
}

int mTimer_add(void *handle, eventFunc_t func, void *ctxArg,
               int interval, int count, const char *name)
{
    timerHandle_t *th = (timerHandle_t *)handle;
    timerEvent_t *curr, *prev, *new;

    if(isEventPresent(handle, func, ctxArg))
    {
        DPRINTF("There is already an event func 0x%x, ctxArg 0x%x\n",
               func, ctxArg);
        return -1;
    }

    new = malloc(sizeof(*new));
    if(new == NULL)
    {
        perror("malloc:");
        return -1;
    }

    new->func = func;
    new->ctxArg = ctxArg;
    new->interval = interval;
    new->count = count;
    getTimeval(&new->lastTime);
    
    DPRINTF("add event at %d\n", new->lastTime.tv_sec);

    if(name != NULL)
    {
        snprintf(new->name, sizeof(new->name), "%s", name);
    }

    if(th->number == 0)
    {
        th->events = new;
    }
    else
    {
        curr = th->events;
        
        new->next = curr;
        th->events = new;
    }

    th->number++;

    DPRINTF("added event %s, interval = %d, func = 0x%x, ctxArg = 0x%x\n",
          new->name, interval, func, ctxArg);

    return 0;    
    
}

int mTimer_delete(void *handle, eventFunc_t func, void *ctxArg)
{
    timerHandle_t *th = (timerHandle_t *)handle;
    timerEvent_t *curr, *prev;

    if((curr = th->events) == NULL)
    {
        DPRINTF("no events to delete (func=0x%x data=%p)\n", func, ctxArg);
        return -1;
    }

    if(curr->func == func && curr->ctxArg == ctxArg)
    {
        th->events = curr->next;
        curr->next = NULL;        
    }
    else
    {
        while(curr != NULL)
        {
            prev = curr;
            curr = curr->next;

            if(curr != NULL &&
               curr->func == func &&
               curr->ctxArg == ctxArg)
            {
                prev->next = curr->next;
                curr->next = NULL;
                break;
            }
        }
    }

    if(curr != NULL)
    {
        th->number--;
        DPRINTF("canceled event %s, count=%d\n", curr->name, th->number);

        free(curr);
    }
    else
    {
        DPRINTF("could not find requested event to delete, func=0x%x ctxArg=%p count=%d\n",
               func, ctxArg, th->number);        
    }

    return 0;
}

void mTimer_executeExpireEvents(void *handle)
{
    timerHandle_t  *th = (timerHandle_t *)handle;
    timerEvent_t   *curr, *tmp;
    struct timeval  tv;

    getTimeval(&tv);
    curr = th->events;

    DPRINTF("call mTimer_executeExpireEvents at %d\n", tv.tv_sec);
    while (curr != NULL)
    {
        struct timeval eventTv;

        tmp = curr->next;

        eventTv = tvAdd(&curr->lastTime, curr->interval);
        
        DPRINTF("Time compare %d, %d\n", eventTv.tv_sec, tv.tv_sec);
        
        if(IS_LE_TIMEVAL(&eventTv, &tv))
        {
            DPRINTF("executing timer event %s func 0x%x ctxArg 0x%x\n",
                  curr->name, curr->func, curr->ctxArg);
        
            (curr->func)(curr->ctxArg);
            
            if(--curr->count == 0)
            {                
                mTimer_delete(handle, curr->func, curr->ctxArg);
                th->number--;
            }

            curr->lastTime = eventTv;
            DPRINTF("update lastTime to %d\n", curr->lastTime.tv_sec);
        }
        
        curr = tmp;
    }

    return;
}
