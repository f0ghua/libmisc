/* #define F_DEBUG */
#ifdef F_DEBUG
#include <errno.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/vfs.h>

#include "misc_oil.h"
#include "libmisc.h"

#ifndef TIMESPEC_TO_TIMEVAL
# define TIMESPEC_TO_TIMEVAL(tv, ts) {          \
        (tv)->tv_sec = (ts)->sec;               \
        (tv)->tv_usec = (ts)->nsec / 1000;      \
    }
#endif

#ifndef TIMEVAL_TO_TIMESPEC
# define TIMEVAL_TO_TIMESPEC(tv, ts) {          \
        (ts)->sec = (tv)->tv_sec;               \
        (ts)->nsec = (tv)->tv_usec * 1000;      \
    }
#endif

void oil_tmsGet(oilTimeStamp_t *ts)
{
    struct timeval tv;
    int n;

    n = gettimeofday(&tv, NULL);

    if(n == 0)
    {
        TIMEVAL_TO_TIMESPEC(&tv, ts);
    }
    else
    {
        ts->sec = 0;
        ts->nsec = 0;
    }
}

void oil_openlog(void)
{
   openlog(NULL, 0, LOG_DAEMON);
   return;
}

void oil_syslog(int level, const char *buf)
{
   syslog(level, buf);
   return;
}

void oil_closelog(void)
{
   closelog();
   return;
}

int oil_shmInit(int shmSize, int *shmId, void **shmAddr)
{
    void *addr;
    int id, shmflg;
    
    if ((*shmId) != UNINITIALIZED_SHM_ID)
    {
        /* let system to choose a suitable address */
        addr = shmat(*shmId, (void *)NULL, 0); 
        if (addr == (void *) -1)
        {
#ifdef F_DEBUG
            perror("shmat:");
#endif            
            misc_logError("Could not attach to shmId = %d", *shmId);
            *shmAddr = NULL;
            return -1;
        }

        *shmAddr = addr;

        return 0;
    }

   /*
    * OK, if we get here, then shmId must be -1, which means we
    * have to create the shared memory region.
    */
   /*
    * Create the shared memory region.
    */
   misc_logNotice("creating shared memory region, size=%d", shmSize);
   shmflg = 0666; /* allow everyone to read/write */
   shmflg |= (IPC_CREAT | IPC_EXCL);

   if ((id = shmget(0, shmSize, shmflg)) == -1)
   {
      misc_logError("Could not create shared memory.");
      return -1;
   }
   else
   {
      misc_logDebug("created shared memory, shmid=%d", id);
   }

   if ((addr = shmat(id, (void *)NULL, 0)) == (void *) -1)
   {
      misc_logError("could not attach %d", id);
      oil_shmCleanup(id, NULL);
   }   

   *shmId = id;
   *shmAddr = addr;
   
   return 0;
}

static void oil_shmDetach(void *shmAddr)
{
    if (shmAddr == NULL)
    {
        misc_logError("got uninitialized shmAddr, no detach needed");
    }
    else
    {
        if (shmdt(shmAddr) != 0)
        {
            misc_logError("shmdt of shmAddr=%p failed", shmAddr);
        }
        else
        {
            misc_logDebug("detached shmAddr=%p", shmAddr);
        }
    }
}

void oil_shmCleanup(int shmId, void *shmAddr)
{
   struct shmid_ds shmbuf;

   /*
    * stat the shared memory to see how many processes are attached.
    */
   memset(&shmbuf, 0, sizeof(shmbuf));
   if (shmctl(shmId, IPC_STAT, &shmbuf) < 0)
   {
      misc_logError("shmctl IPC_STAT failed");
      return;
   }
   else
   {
      misc_logDebug("nattached=%d", shmbuf.shm_nattch);
   }    

   /* just detach myself first */
   oil_shmDetach(shmAddr);


   /* no other proceeses are still attached, free id */
   if (shmbuf.shm_nattch <= 1)
   {
       memset(&shmbuf, 0, sizeof(shmbuf));
       if (shmctl(shmId, IPC_RMID, &shmbuf) < 0)
       {
           misc_logError("shm destory of shmId=%d failed.", shmId);
       }
       else
       {
           misc_logDebug("shared mem (shmId=%d) destroyed.", shmId);
       }       
   }

   return;
}

#ifdef CONFIG_SCM_SUPPORT
#include "nvram.h"

char *oil_getCfgValue(const char *name)
{
    char *p;
    static char attrValue[OIL_MAX_STR_LEN];

    attrValue[0] = '\0';
    
    if((p = nvram_get(name)) != NULL)
    {
        snprintf(attrValue, sizeof(attrValue), "%s", p);
        free(p);
    }

    return attrValue;
}
#else
char *oil_getCfgValue(const char *name)
{
    return NULL;
}
#endif
