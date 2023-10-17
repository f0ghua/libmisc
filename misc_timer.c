#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "misc_timer.h"

#define PRINTF printf

/** This macro will evaluate TRUE if a is earlier than b */
#define IS_EARLIER_THAN(a, b) (((a)->tv_sec < (b)->tv_sec) ||   \
                               (((a)->tv_sec == (b)->tv_sec) && \
                                ((a)->tv_usec < (b)->tv_usec)))

#define IS_ZERO_TIMEVAL(a) (((a)->tv_sec == 0)&&((a)->tv_usec == 0))

typedef struct timer_event
{
    struct timer_event *next;   /**< pointer to the next timer. */
    struct timeval      expire; /**< Timestamp (in the future) of when this
                                    *   timer event will expire. */
    event_func          func;   /**< handler func to call when event expires. */
    void               *ctx_data; /**< context data to pass to func */
    char                name[EVENT_TIMER_NAME_LENGTH]; /**< name of this timer */
} timer_event_t;

/** Internal timer handle. */
typedef struct timer_handle
{
   timer_event_t *events;       /**< Singly linked list of events */
   int            number;       /**< Number of events in this handle. */
} timer_handle_t;

static int timer_get_tv(struct timeval *tv)
{
    int n;
    
    n = gettimeofday(tv, NULL);

    return n;
}

static void timer_add_msec(struct timeval *tv, int ms)
{
    int sec, msec;

    sec = ms / MSECS_IN_SEC;
    msec = (ms % MSECS_IN_SEC) * MSECS_IN_SEC;

    tv->tv_sec += sec;
    tv->tv_usec += msec;

    return;
}

int timer_init(void **timer_handle)
{
    *timer_handle = calloc(1, sizeof(timer_handle_t));
    if(*timer_handle == NULL)
    {
        perror("malloc");
        return -1;
    }
    
    return 0;
}

void timer_cleanup(void **handle)
{
    timer_handle_t *th = (timer_handle_t *)(*handle);
    timer_event_t *event;

    while((event = th->events) != NULL)
    {
        th->events = event->next;
        free(event);
    }

    free(*handle);

    return;
}

static int timer_is_event_present(void *handle, event_func func, void *ctx_data)
{
    const timer_handle_t *th = (const timer_handle_t *)handle;
    timer_event_t *event;
    
    event = th->events;

    while(event != NULL)
    {
        if(event->func == func && event->ctx_data == ctx_data)
            break;
        event = event->next;
    }

    return (event != NULL);
}

int timer_event_add(void *handle, event_func func, void *ctx_data,
                    int ms, const char *name)
{
    timer_handle_t *th = (timer_handle_t *)handle;
    timer_event_t *curr, *prev, *new;

    if(timer_is_event_present(handle, func, ctx_data))
    {
        printf("There is already an event func 0x%x, ctx_data 0x%x\n",
               func, ctx_data);
        return -1;
    }

    new = malloc(sizeof(*new));
    if(new == NULL)
    {
        perror("malloc:");
        return -1;
    }

    new->func = func;
    new->ctx_data = ctx_data;
    
    timer_get_tv(&new->expire);
    timer_add_msec(&new->expire, ms);

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
        
        if(IS_EARLIER_THAN(&new->expire, &curr->expire))
        {
            new->next = curr;
            th->events = new;
        }
        else
        {
            while(1)
            {
                prev = curr;
                curr = curr->next;

                if((curr == NULL) ||
                   (IS_EARLIER_THAN(&new->expire, &curr->expire)))
                {
                    new->next = prev->next;
                    prev->next = new;
                    break;
                }
            }
        }
    }

    th->number++;

    PRINTF("added event %s, expires in %ums(at %u.%03u), func = 0x%x, ctx_data = 0x%x\n",
           new->name, ms, new->expire.tv_sec, new->expire.tv_usec,
           func, ctx_data);

    return 0;
}

int timer_event_delete(void *handle, event_func func, void *ctx_data)
{
    timer_handle_t *th = (timer_handle_t *)handle;
    timer_event_t *curr, *prev;

    if((curr = th->events) == NULL)
    {
        PRINTF("no events to delete (func=0x%x data=%p)\n", func, ctx_data);
        return -1;
    }

    if(curr->func == func && curr->ctx_data == ctx_data)
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
               curr->ctx_data == ctx_data)
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
        PRINTF("canceled event %s, count=%d\n", curr->name, th->number);

        free(curr);
    }
    else
    {
        PRINTF("could not find requested event to delete, func=0x%x ctx_data=%p count=%d\n",
               func, ctx_data, th->number);        
    }

    return 0;
}

void timer_execute_expire_events(void *handle)
{
    timer_handle_t *th = (timer_handle_t *)handle;
    timer_event_t *curr;
    struct timeval tv;

    timer_get_tv(&tv);
    curr = th->events;
    
    while((curr != NULL) && IS_EARLIER_THAN(&curr->expire, &tv))
    {
        th->events = curr->next;
        curr->next = NULL;
        th->number--;
        
        PRINTF("executing timer event %s func 0x%x ctx_data 0x%x\n",
               curr->name, curr->func, curr->ctx_data);
        
        (curr->func)(curr->ctx_data);
   
        free(curr);
        
        curr = th->events;
    }

    return;
}
