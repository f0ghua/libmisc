#ifndef _TIMER_H_
#define _TIMER_H_

/** Number of milliseconds in 1 second */
#define MSECS_IN_SEC  1000
#define EVENT_TIMER_NAME_LENGTH 16

#define TIMER_FLAG_LOOP (1<<0)

typedef void (*event_func)(void*);

int timer_init(void **timer_handle);

void timer_cleanup(void **handle);

int timer_event_add(void *handle, event_func func, void *ctx_data,
                    int ms, const char *name);

int timer_event_delete(void *handle, event_func func, void *ctx_data);

void timer_execute_expire_events(void *handle);

#endif
