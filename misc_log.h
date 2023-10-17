#ifndef _LOG_H_
#define _LOG_H_

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

/*!\enum logDest_t
 * \brief identifiers for message logging destinations.
 */
typedef enum
{
   LOG_DEST_STDERR  = 1,  /**< Message output to stderr. */
   LOG_DEST_SYSLOG  = 2,  /**< Message output to syslog. */
   LOG_DEST_TELNET  = 3   /**< Message output to telnet clients. */
} logDest_t;

#define MAX_LOG_ENTITY         32
#define LOG_SHM_FILE           "/tmp/log_shm"

/** Show application name in the log line. */
#define LOG_HDRMASK_APPNAME    0x0001 

/** Show log level in the log line. */
#define LOG_HDRMASK_LEVEL      0x0002 

/** Show timestamp in the log line. */
#define LOG_HDRMASK_TIMESTAMP  0x0004

/** Show location (function name and line number) level in the log line. */
#define LOG_HDRMASK_LOCATION   0x0008
 
/** Default log level is error messages only. */
#define DEFAULT_LOG_LEVEL        LOG_LEVEL_ERR

/** Default log destination is standard error */
#define DEFAULT_LOG_DESTINATION  LOG_DEST_SYSLOG

/** Default log header mask */
#define DEFAULT_LOG_HEADER_MASK (LOG_HDRMASK_APPNAME)

/* Maxminu length of applicaiton name */
#define MAX_LOG_NAME_LENGTH      16

/** Maxmimu length of a single log line; messages longer than this are truncated. */
#define MAX_LOG_LINE_LENGTH      512

/** Internal message log function; do not call this function directly.
 *
 * NOTE: Applications should NOT call this function directly from code.
 *       Use the macros defined in log.h, i.e.
 *       log_error, log_notice, log_debug.
 *
 * This function performs message logging based on two control
 * variables, "logLevel" and "logDestination".  These two control
 * variables are local to each process.  Each log message has an
 * associated severity level.  If the severity level of the message is
 * numerically lower than or equal to logLevel, the message will be logged to
 * either stderr or syslogd based on logDestination setting.
 * Otherwise, the message will not be logged.
 * 
 * @param level (IN) The message severity level as defined in "syslog.h".
 *                   The levels are, in order of decreasing importance:
 *                   LOG_EMERG (0)- system is unusable 
 *                   LOG_ALERT (1)- action must be taken immediately
 *                   LOG_CRIT  (2)- critical conditions
 *                   LOG_ERR   (3)- error conditions 
 *                   LOG_WARNING(4) - warning conditions 
 *                   LOG_NOTICE(5)- normal, but significant, condition
 *                   LOG_INFO  (6)- informational message 
 *                   LOG_DEBUG (7)- debug-level message
 * @param func (IN) Function name where the log message occured.
 * @param line (IN) Line number where the log message occured.
 * @param fmt  (IN) The message string.
 *
 */
void log_log(logLevel_t level, const char *func, int line, const char *fmt, ... );

void log_init(char *appname);
void log_cleanup(void);

#endif
