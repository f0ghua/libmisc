#ifndef _MISC_TIMER2_H_
#define _MISC_TIMER2_H_

#define MSECS_IN_SEC            1000
#define EVENT_TIMER_NAME_LENGTH 16

typedef void (*eventFunc_t)(void*);

int mTimer_init(void **handle);

void mTimer_cleanup(void **handle);

/** 
 * Add a timer event to the handle, register with pre-defined interval
 * and loop count.
 * 
 * @param handle 
 * @param func 
 * @param ctxArg 
 * @param interval the timer interval
 * @param count the max count number for executing, 0 means loop forever
 * @param name 
 * 
 * @return 
 */
int mTimer_add(void *handle, eventFunc_t func, void *ctxArg,
               int interval, int count, const char *name);

void mTimer_executeExpireEvents(void *handle);

#endif
