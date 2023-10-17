/* #define F_DEBUG */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>      /* open */
#include <syslog.h>

#include "misc_oil.h"
#include "misc_log.h"

/* #define SHM_SUPPORT */

typedef struct logAttr_s {
    char logApplicationName[MAX_LOG_NAME_LENGTH];
    /**< Message logging level.
     * This is set to one of the message
     * severity levels: LOG_ERR, LOG_NOTICE
     * or LOG_DEBUG. Messages with severity
     * level lower than or equal to logLevel
     * will be logged. Otherwise, they will
     * be ignored. This variable is local
     * to each process and is changeable
     * from CLI.
     */    
    logLevel_t logLevel;
    /**< Message logging destination.
     * This is set to one of the
     * message logging destinations:
     * STDERR or SYSLOGD. This
     * variable is local to each
     * process and is changeable from
     * CLI.
     */    
    logDest_t logDestination;
    /**< Bitmask of which pieces of info we want
     * in the log line header.
     */    
    unsigned int logHeaderMask;
}logAttr_t;

#ifdef SHM_SUPPORT
static logAttr_t *logAttribute = NULL;
#else
static logAttr_t logAttr;
static logAttr_t *logAttribute = &logAttr;
#endif

static inline char *get_appLogAttr(char *appName, char *attrPendix)
{
    char *val;
    char attr[MAX_LOG_NAME_LENGTH];
    
    snprintf(attr, sizeof(attr), "%s_%s", appName, attrPendix);
    val = oil_getCfgValue(attr);
    if(misc_isNullStr(val))
        return NULL;
    
    return val;
}

static void updateLogAttr(void)
{
    char *s;
    char *appName = logAttribute->logApplicationName;
    
    if((s = get_appLogAttr(appName, "log_level")) == NULL)
        logAttribute->logLevel = DEFAULT_LOG_LEVEL;
    else
    {
        logAttribute->logLevel = atoi(s);
    }
    
    if((s = get_appLogAttr(appName, "log_dest")) == NULL)
        logAttribute->logDestination = DEFAULT_LOG_DESTINATION;
    else
    {
        logAttribute->logDestination = atoi(s);
    }

    if((s = get_appLogAttr(appName, "log_mask")) == NULL)
        logAttribute->logHeaderMask = DEFAULT_LOG_HEADER_MASK;
    else
    {
        logAttribute->logHeaderMask = atoi(s);
    }
}

void log_log(logLevel_t level, const char *func, int line, const char *fmt, ... )
{
   va_list		ap;
   char buf[MAX_LOG_LINE_LENGTH] = {0};
   int len = 0, maxLen;
   char *logLevelStr = NULL;
   int logTelnetFd = -1;

#ifdef F_DEBUG   
   printf("level = %d, logLevel = %d, headMask = %d\n",
          level,
          logAttribute?logAttribute->logLevel:0,
          logAttribute?logAttribute->logHeaderMask:0);
#endif

   if(logAttribute == NULL)
       return;
   
   updateLogAttr();
   
   maxLen = sizeof(buf);
   
   if (level <= logAttribute->logLevel)
   {
      va_start(ap, fmt);

      if (logAttribute->logHeaderMask & LOG_HDRMASK_APPNAME)
      {
          len = snprintf(buf, maxLen, "%s:", logAttribute->logApplicationName);
      }


      if ((logAttribute->logHeaderMask & LOG_HDRMASK_LEVEL) && (len < maxLen))
      {
         /*
          * Only log the severity level when going to stderr
          * because syslog already logs the severity level for us.
          */
         if (logAttribute->logDestination == LOG_DEST_STDERR)
         {
            switch(level)
            {
            case LOG_LEVEL_ERR:
               logLevelStr = "error";
               break;
            case LOG_LEVEL_NOTICE:
               logLevelStr = "notice";
               break;
            case LOG_LEVEL_INFO:
               logLevelStr = "info";
               break;               
            case LOG_LEVEL_DEBUG:
               logLevelStr = "debug";
               break;
            default:
               logLevelStr = "invalid";
               break;
            }
            len += snprintf(&(buf[len]), maxLen - len, "%s:", logLevelStr);
         }
      }

      /*
       * Log timestamp for both stderr and syslog because syslog's
       * timestamp is when the syslogd gets the log, not when it was
       * generated.
       */
      if ((logAttribute->logHeaderMask & LOG_HDRMASK_TIMESTAMP) && (len < maxLen))
      {
         oilTimeStamp_t ts;

         oil_tmsGet(&ts);
         len += snprintf(&(buf[len]), maxLen - len, "%u.%03u:",
                         ts.sec%1000, ts.nsec/NSECS_IN_MSEC);
      }

      if ((logAttribute->logHeaderMask & LOG_HDRMASK_LOCATION) && (len < maxLen))
      {
         len += snprintf(&(buf[len]), maxLen - len, "%s.%u:", func, line);
      }

      if (len < maxLen)
      {
         maxLen -= len;
         vsnprintf(&buf[len], maxLen, fmt, ap);
      }

#ifdef F_DEBUG      
      printf("logDestination = %d\n", logAttribute->logDestination);
#endif
      
      if (logAttribute->logDestination == LOG_DEST_STDERR)
      {
         fprintf(stderr, "%s\n", buf);
      }
      else if (logAttribute->logDestination == LOG_DEST_TELNET )
      {
   #ifdef DESKTOP_LINUX
         /* Fedora Desktop Linux */
         logTelnetFd = open("/dev/pts/1", O_RDWR);
   #else
         /* CPE use ptyp0 as the first pesudo terminal */
         logTelnetFd = open("/dev/ttyp0", O_RDWR);
   #endif
         if(logTelnetFd != -1)
         {
            write((FILE *)logTelnetFd, buf, strlen(buf));
            write((FILE *)logTelnetFd, "\n", strlen("\n"));
            close(logTelnetFd);
         }
      }
      else
      {          
          syslog(level, buf);
      }

      va_end(ap);
   }

}

static logAttr_t *initLogEntity(char *appName, logAttr_t *logAttrArray)
{
    int i;
    logAttr_t *logAttr;

    logAttr = logAttrArray;
    for(i = 0; i < MAX_LOG_ENTITY; i++)
    {        
        if(strcmp(logAttr->logApplicationName, appName) == 0)
        {            
            return logAttr;
        }
        logAttr++;
    }

    logAttr = logAttrArray;
    for(i = 0; i < MAX_LOG_ENTITY; i++)
    {
        if(logAttr->logApplicationName[0] == 0)
        {
            break;
        }
        
        logAttr++;
    }    

    if(i == MAX_LOG_ENTITY)
        return NULL;

    snprintf(logAttr->logApplicationName,
             sizeof(logAttr->logApplicationName),
             "%s", appName);
    logAttr->logLevel       = DEFAULT_LOG_LEVEL;
    logAttr->logDestination = DEFAULT_LOG_DESTINATION;
    logAttr->logHeaderMask  = DEFAULT_LOG_HEADER_MASK;
        
    return logAttr;
}

void log_init(char *appname)
{
    int shmId = UNINITIALIZED_SHM_ID;
    int ret;
    void *shmAddr;
    FILE *fp;
    char buf[32];
    char *s, *appName = appname;

    for (s = appname; *s != '\0';)
    {
        if (*s++ == '/')
            appName = s;
    }    
    
#ifdef SHM_SUPPORT
    if(access(LOG_SHM_FILE, F_OK) == 0)
    {
        if((fp = fopen(LOG_SHM_FILE, "r")) == NULL)
            return;
        fread(buf, sizeof(buf), 1, fp);
        fclose(fp);
        
        shmId = atoi(line);

#ifdef F_DEBUG
        printf("%s %s %d shmId = %d\n",
               __FUNCTION__, __FILE__, __LINE__,
               shmId);
#endif        
        ret = oil_shmInit(sizeof(logAttr_t)*MAX_LOG_ENTITY, &shmId, &shmAddr);
        if(ret != 0)
        {
            printf("oil_shmInit error\n");
            return;
        }
    }
    else
    {
        /* first initialize */
        ret = oil_shmInit(sizeof(logAttr_t)*MAX_LOG_ENTITY, &shmId, &shmAddr);
        if(ret != 0)
        {
            printf("oil_shmInit initialize error\n");
            return;
        }        

#ifdef F_DEBUG
        printf("%s %s %d shmId = %d, shmAddr = 0x%x\n",
               __FUNCTION__, __FILE__, __LINE__,
               shmId, shmAddr);
#endif        
        if((fp = fopen(LOG_SHM_FILE, "w")) == NULL)
            return;
        fprintf(fp, "%d", shmId);
        fclose(fp);
        
        memset(shmAddr, 0, sizeof(logAttr_t)*MAX_LOG_ENTITY);
#ifdef F_DEBUG
        printf("%s %s %d tracing ...\n", __FUNCTION__, __FILE__, __LINE__);
#endif        
    }
    
    logAttribute = initLogEntity(appName, (logAttr_t *)shmAddr);

#else
    if((s = get_appLogAttr(appName, "log_level")) == NULL)
        logAttribute->logLevel = DEFAULT_LOG_LEVEL;
    else
    {
        logAttribute->logLevel = atoi(s);
    }
    
    if((s = get_appLogAttr(appName, "log_dest")) == NULL)
        logAttribute->logDestination = DEFAULT_LOG_DESTINATION;
    else
    {
        logAttribute->logDestination = atoi(s);
    }

    if((s = get_appLogAttr(appName, "log_mask")) == NULL)
        logAttribute->logHeaderMask = DEFAULT_LOG_HEADER_MASK;
    else
    {
        logAttribute->logHeaderMask = atoi(s);
    }

    snprintf(logAttribute->logApplicationName,
             sizeof(logAttribute->logApplicationName),
             "%s", appName);
#endif
    
    oil_openlog();
   
    return;

}  /* End of cmsLog_init() */

void log_cleanup(void)
{
    oil_closelog();
    return;
} 
