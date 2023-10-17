#ifndef _LIBMISC_H_
#define _LIBMISC_H_

/* ------------------------------- log -------------------------------------- */
/*!\enum logLevel_t
 * \brief Logging levels.
 * These correspond to LINUX log levels for convenience.  Other OS's
 * will have to map these values to their system.
 */
typedef enum
{
    LOG_LEVEL_CRIT       = 2, /**< Message at critical level. */
    LOG_LEVEL_ERR        = 3, /**< Message at error level. */
    LOG_LEVEL_WARNING    = 4, /**< Message at warning level. */
    LOG_LEVEL_NOTICE     = 5, /**< Message at notice level. */
    LOG_LEVEL_INFO       = 6, /**< Message at informational level. */
    LOG_LEVEL_DEBUG      = 7  /**< Message at debug level. */
} logLevel_t;

#define misc_logCrit(args...)    log_log(LOG_LEVEL_CRIT, __FUNCTION__, __LINE__, args)
#define misc_logError(args...)   log_log(LOG_LEVEL_ERR, __FUNCTION__, __LINE__, args)
#define misc_logWarning(args...) log_log(LOG_LEVEL_WARNING, __FUNCTION__, __LINE__, args)
#define misc_logNotice(args...)  log_log(LOG_LEVEL_NOTICE, __FUNCTION__, __LINE__, args)
#define misc_logInfo(args...)    log_log(LOG_LEVEL_INFO, __FUNCTION__, __LINE__, args)
#define misc_logDebug(args...)   log_log(LOG_LEVEL_DEBUG, __FUNCTION__, __LINE__, args)

void log_log(logLevel_t level, const char *func, int line, const char *fmt, ... );
void log_init(char *appname);
void log_cleanup(void);

/* ------------------------------- timer -------------------------------------- */
#define misc_timerInit                timer_init
#define misc_timerCleanup             timer_cleanup
#define misc_timerEventAdd            timer_event_add
#define misc_timerEventDelete         timer_event_delete
#define misc_timerExecuteExpireEvents timer_execute_expire_events

typedef void (*event_func)(void*);
int timer_init(void **timer_handle);
void timer_cleanup(void **handle);
int timer_event_add(void *handle, event_func func, void *ctx_data,
              int ms, const char *name);
int timer_event_delete(void *handle, event_func func, void *ctx_data);
void timer_execute_expire_events(void *handle);

/* ------------------------------- net -------------------------------------- */

char *misc_getIpAddress(char *ifname);


/* ------------------------------- util -------------------------------------- */

#define SYSTEM misc_system

void misc_procEp(char *pidfile);
void misc_printFile(char *format, ...);
void misc_printConsole(const char *format, ...);
void misc_pipeCmd(char *command, char **output);
void misc_system(const char *format, ...);
int misc_isNullStr(const char *str);
int misc_selSleep(int sec);

#endif /* _LIBMISC_H_ */
