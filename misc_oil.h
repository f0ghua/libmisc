#ifndef _MISC_TIME_H_
#define _MISC_TIME_H_

#define UNINITIALIZED_SHM_ID -1

#define OIL_MAX_STR_LEN 256

/** Number of nanoseconds in 1 second. */
#define NSECS_IN_SEC 1000000000

/** Number of nanoseconds in 1 milli-second. */
#define NSECS_IN_MSEC 1000000

/** Number of nanoseconds in 1 micro-second. */
#define NSECS_IN_USEC 1000

/** Number of micro-seconds in 1 second. */
#define USECS_IN_SEC  1000000

/** Number of micro-seconds in a milli-second. */
#define USECS_IN_MSEC 1000

/** Number of milliseconds in 1 second */
#define MSECS_IN_SEC  1000

/** OS independent timestamp structure.
 */
typedef struct
{
   unsigned int sec;   /**< Number of seconds since some arbitrary point. */
   unsigned int nsec;  /**< Number of nanoseconds since some arbitrary point. */
} oilTimeStamp_t;

void oil_tmsGet(oilTimeStamp_t *ts);
void oil_openlog(void);
void oil_syslog(int level, const char *buf);
void oil_closelog(void);
int oil_shmInit(int shmSize, int *shmId, void **shmAddr);
void oil_shmCleanup(int shmId, void *shmAddr);
char *oil_getCfgValue(const char *name);

#endif
