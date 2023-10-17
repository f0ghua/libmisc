/**
 * @file   misc_crash.c
 * @author fog
 * @date   Fri Mar  5 13:19:34 2010
 * 
 * @brief  /proc/meminfo, /proc/modules
 *         /proc/<pid>/stat, /proc/<pid>/maps, /proc/<pid>/cmdline 
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifndef ARCH_MIPS
#include <execinfo.h>
#endif

#include "misc_crash.h"

#define NIY()	printf("%s: Not Implemented Yet!\n", __FUNCTION__)

static mCrashParameters_t gbl_params;
static int gbl_fd=-1;

static int    gbl_backtraceEntries;
static void **gbl_backtraceBuffer;
static char **gbl_backtraceSymbols;
static int    gbl_backtraceDoneFlag = 0;

/*!
 * Output text to a fd, looping to avoid being interrupted.
 *
 * @param str   String to output
 * @param bytes String length
 * @param fd    File descriptor to write to
 *
 * @returns bytes written, or error on failure.
 */
static int blockingWrite(char *str, int bytes, int fd)
{
	int offset=0;
	int bytesWritten;
	int totalWritten = 0;
	
	while (bytes > 0) {
		bytesWritten = write(fd, &str[offset], bytes);
		if (bytesWritten < 1) break;
		totalWritten += bytesWritten;
		bytes -= bytesWritten;
	}

	return totalWritten;

}

/*!
 * Print out a line of output to all our destinations
 *
 * One by one, output a line of text to all of our output destinations.
 *
 * Return failure if we fail to output to any of them.
 *
 * @param format   Normal printf style vararg format
 *
 * @returns bytes written, or error on failure.
 */
static int outputPrintf(char *format, ...)
{
	/* Our output line of text */
	static char outputLine[MAX_LINE_LEN];
	int bytesInLine;
	va_list ap;
	int return_value=0;

	va_start(ap, format);

	bytesInLine = vsnprintf(outputLine, MAX_LINE_LEN-1, format, ap);
	if (bytesInLine > -1 && bytesInLine < (MAX_LINE_LEN-1)) {
		/* We're a happy camper -- start printing */
		if (gbl_params.filename) {
			/* append to our file -- hopefully it's been opened */
			if (gbl_fd != -1) {
			   if (blockingWrite(outputLine, bytesInLine, gbl_fd)) {
					return_value=-2;
			   }
			}
		}

		/* Write to our file pointer */
		if (gbl_params.filep != NULL) {
			if (fwrite(outputLine, bytesInLine, 1, gbl_params.filep) != 1) {
				return_value=-3;
			}
			fflush(gbl_params.filep);
		}

		/* Write to our fd */
		if (gbl_params.fd != -1) {
		   if (blockingWrite(outputLine, bytesInLine, gbl_params.fd)) {
				return_value=-4;
		   }
		}
	} else {
		/* We overran our string. */
		return_value=-1;
	}

	return return_value;
}

/*!
 * Initialize our output (open files, etc)
 *
 * This file initializes all output streams, since we're about
 * to have output.
 *
 */
static void outputInit( void )
{
	if (gbl_params.filename) {
		/* First try append */
		gbl_fd = open(gbl_params.filename, O_WRONLY|O_APPEND);
		if (gbl_fd < 0) {
			gbl_fd = open(gbl_params.filename, O_RDWR|O_CREAT,
						  S_IREAD|S_IWRITE|S_IRGRP|S_IROTH); /* 0644 */
			if (gbl_fd < 0) {
				gbl_fd = -1;
			}
		}
	}
}

/*!
 * Finalize our output (close files, etc)
 *
 * This file closes all output streams.
 *
 */
static void outputFini( void )
{
	if (gbl_fd > -1)
		close(gbl_fd);

	if (gbl_params.filep != NULL)
		fclose(gbl_params.filep);

	if (gbl_params.fd > -1)
		close(gbl_params.fd);

	// Just in case someone tries to call outputPrintf after outputFini
	gbl_fd = gbl_params.fd = -1;
	gbl_params.filep = NULL;

	sync();
}

static void outputFile(char *file)
{
    FILE *fp;
    char buf[1024];

    outputPrintf("mCrash dumping file: %s\n", file);
    
    if((fp = fopen(file, "r")) == NULL)
        return;

    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        outputPrintf(buf);
    }

    fclose(fp);

    outputPrintf("\n");
}

/** 
 * Use pipe to get the command output, then dump for checking.
 *
 * Please notice that popen create a new process, and when it
 * finished, we will receive a SIGCHLD signal. so we should backup the
 * oldone and replace it with SIGDFL before the popen run.
 * 
 * @param command 
 */
static void outputCmd(char *command)
{
    FILE *fp;
    char buf[1024];
    struct sigaction sa, saOrg;
    
    outputPrintf("mCrash dumping command: %s\n", command);
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (void (*)(int)) SIG_DFL;
    sigaction(SIGCHLD, &sa, &saOrg);
    
    if((fp = popen(command, "r")) == NULL)
        return;
    
    while((fgets(buf, sizeof(buf), fp)) != NULL)
    {
        outputPrintf(buf);
    }
    
    pclose(fp);

    sigaction(SIGCHLD, &saOrg, NULL);

    outputPrintf("\n");
}

/*!
 * Dump our backtrace into a global location
 *
 * This function will dump out our backtrace into our
 * global holding area.
 *
 */
static void createGlobalBacktrace(int signo, siginfo_t *siginfo,
                                  void *uc)
{

#ifdef ARCH_MIPS
    gbl_backtraceEntries = mips_backtrace(gbl_backtraceBuffer,
                                          gbl_params.maxStackDepth,
                                          uc);
#else
	gbl_backtraceEntries = backtrace(gbl_backtraceBuffer,
					                 gbl_params.maxStackDepth);

	/* This is NOT signal safe -- it calls malloc.  We need to
	   let the caller pass in a pointer to a symbol table inside of
	   our params. TODO */
    if (gbl_params.useBacktraceSymbols != FALSE) {
        gbl_backtraceSymbols = backtrace_symbols(gbl_backtraceBuffer,
                                                 gbl_backtraceEntries);
    }
#endif
}

/*!
 * Print out (to all the fds, etc), or global backtrace
 */
static void outputGlobalBacktrace ( void )
{
	int i;

	for (i = 0; i < gbl_backtraceEntries; i++) {
        if (gbl_backtraceSymbols != FALSE) {
            outputPrintf("*      Frame %02d: %s\n",
                         i, gbl_backtraceSymbols[i]);
        } else {
            outputPrintf("*      Frame %02d: %p\n", i,
                         gbl_backtraceBuffer[i]);
        }
	}
}

/*!
 * Output our current stack's backtrace
 */
static void outputBacktrace(int signo, siginfo_t *siginfo, void *uc)
{
    char procFile[64];
    pid_t pid = getpid();
    struct tm *tm;
    time_t curtime;

    time( &curtime );    
    tm = gmtime( &curtime );
   
    outputPrintf("------------------------------------------------------------\n");
    outputPrintf("* mCrash received signal %d on thread PID %d\n",
                 signo, pid );
    outputPrintf("*      datetime: [%04d-%02d-%02d %02d:%02d:%02d]\n",
                 tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);    
    outputPrintf("*      siginfo->si_signo   = %d\n", siginfo->si_signo );
    outputPrintf("*      siginfo->si_errno   = %d\n", siginfo->si_errno );
    outputPrintf("*      siginfo->si_code    = %d\n", siginfo->si_code );
    outputPrintf("*      siginfo->si_pid     = %d\n", siginfo->si_pid );
    outputPrintf("*      siginfo->si_addr    = 0x%08x\n", (unsigned int)siginfo->si_addr );
    outputPrintf("*      siginfo->si_status  = 0x%08x\n", (unsigned int)siginfo->si_status );
    outputPrintf("------------------------------------------------------------\n");

    outputCmd("ps");
    
    snprintf(procFile, sizeof(procFile), "/proc/meminfo");
    outputFile(procFile);    
    
    snprintf(procFile, sizeof(procFile), "/proc/%ld/stat", (long)pid);
    outputFile(procFile);
    
    snprintf(procFile, sizeof(procFile), "/proc/%ld/maps", (long)pid);
    outputFile(procFile);

	outputPrintf("************************************************************\n");
	outputPrintf("*               mCrash BackTrace Dump\n");
	outputPrintf("************************************************************\n");
	outputPrintf("*\n");
    
	createGlobalBacktrace(signo, siginfo, uc);
	outputGlobalBacktrace();
    
	outputPrintf("*\n");
	outputPrintf("************************************************************\n");
	outputPrintf("*               mCrash BackTrace Dump\n");
	outputPrintf("************************************************************\n");    
}

/*!
 * Handle signals (crash signals)
 *
 * This function will catch all crash signals, and will output the
 * crash dump.  
 *
 * It will physically write (and sync) the current thread's information
 * before it attempts to send signals to other threads.
 * 
 * @param signum Signal received.
 */
static void crash_handler(int signo, siginfo_t *siginfo, void *uc)
{
    struct sigaction sa;
    
	outputInit();
    
	outputBacktrace(signo, siginfo, uc);
    
	outputFini();

    /* reset the signal action so that kernel can do it again with a
     * core dump
     */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (void (*)(int)) SIG_DFL;
    sigaction(signo, &sa, NULL);
    
}

int miscCrash_init(mCrashParameters_t *params)
{
	int sigIndex;
	int ret = 0;

    if (params != NULL) {
		/* Make ourselves a global copy of params. */
		gbl_params = *params;
		gbl_params.filename = strdup(params->filename);

		/* Set our defaults, if they weren't specified */
		if (gbl_params.maxStackDepth == 0 )
			gbl_params.maxStackDepth = MCRASH_DEFAULT_STACK_DEPTH;

        gbl_backtraceBuffer = malloc(sizeof(void *) * (params->maxStackDepth));
        
		if (gbl_params.defaultBacktraceSignal == 0 )
			gbl_params.defaultBacktraceSignal = MCRASH_DEFAULT_BACKTRACE_SIGNAL;

		if (gbl_params.debugLevel == 0 )
			gbl_params.debugLevel = MCRASH_DEBUG_DEFAULT;

		/* And, finally, register for our signals */
        {
            struct sigaction sa;
            
            memset(&sa, 0, sizeof(sa));
            sa.sa_flags = SA_SIGINFO;
            sa.sa_handler = (void (*)(int)) crash_handler;
            
            for (sigIndex = 0; gbl_params.signals[sigIndex] != 0; sigIndex++) {
                DPRINTF(MCRASH_DEBUG_VERY_VERBOSE,
                        "   Catching signal[%d] %d\n", sigIndex,
                        gbl_params.signals[sigIndex]);
            
                sigaction(gbl_params.signals[sigIndex], &sa, NULL);
            }
        }
	} else {
		DPRINTF(MCRASH_DEBUG_ERROR, "   Error:  Null Params!\n");
		ret = -1;
	}

	DPRINTF(MCRASH_DEBUG_VERY_VERBOSE, "Init Complete ret=%d\n", ret);
    
	return ret;
}

static char *miscCrash_getProcName(pid_t pid)
{
    static char curProcessName[MAX_LINE_LEN];
    FILE *fp;
    char pmapFile[64];
    char buf[1024];
    
    snprintf(pmapFile, sizeof(pmapFile), "/proc/%ld/stat", (long)pid);
    if((fp = fopen(pmapFile, "r")) == NULL)
        return NULL;

    if(fgets(buf, sizeof(buf), fp) == NULL)
        return NULL;

    memset(curProcessName, 0, sizeof(curProcessName));
    sscanf(buf, "%*[^(](%[^)]", curProcessName);

    return curProcessName;
}

int miscCrash_register(char *fdpath)
{
    mCrashParameters_t params;
    int i = 0, rc;
    pid_t pid;
    char buf[64];

    bzero(&params, sizeof(params));
    
    pid = getpid();
    snprintf(buf, sizeof(buf), "mCrash.%s.file",
             miscCrash_getProcName(pid));
    params.filename = strdup(buf);
    
    params.filep = stdout;

    if(fdpath != NULL) {
        params.fd = open(fdpath, O_WRONLY|O_CREAT,
                         S_IREAD|S_IWRITE|S_IRGRP|S_IROTH); /* 0644 */
        if (params.fd < 0) {
            /* Try again, with a append */
            params.fd = open(fdpath, O_WRONLY|O_TRUNC);
        }
    }
    
    params.signals[i++] = SIGHUP;  /* Hangup (POSIX).  */    
    params.signals[i++] = SIGILL;  /* Illegal instruction (ANSI).  */      
    params.signals[i++] = SIGFPE;  /* Floating-point exception (ANSI).  */
    params.signals[i++] = SIGBUS;  /* BUS error (4.2 BSD).  */
    params.signals[i++] = SIGSEGV; /* Segmentation violation (ANSI).  */
    params.signals[i++] = SIGURG;  /* Urgent condition on socket (4.2 BSD).  */
    params.signals[i++] = SIGXCPU; /* CPU limit exceeded (4.2 BSD).  */
    params.signals[i++] = SIGXFSZ; /* File size limit exceeded (4.2 BSD).  */

#if 0
    params.signals[i++] = SIGINT; /* Interrupt (ANSI).  */
    params.signals[i++] = SIGPIPE; /* Broken pipe (POSIX).  */
    params.signals[i++] = SIGQUIT; /* Quit (POSIX).  */
    params.signals[i++] = SIGEMT;
    params.signals[i++] = SIGTRAP; /* Trace trap (POSIX).  */
    params.signals[i++] = SIGABRT; /* Abort (ANSI).  */
    params.signals[i++] = SIGSYS;
    params.signals[i++] = SIGALRM; /* Alarm clock (POSIX).  */
    params.signals[i++] = SIGTERM; /* Termination (ANSI).  */
    params.signals[i++] = SIGPWR; /* Power failure restart (System V).  */
    params.signals[i++] = SIGIO; /* I/O now possible (4.2 BSD).  */
    params.signals[i++] = SIGTSTP; /* Keyboard stop (POSIX).  */
#endif
    
    rc = miscCrash_init(&params);    
	if (rc) {
		printf("eCrash_Init returned %d\n", rc);
		return -1;
	}

    return 0;
}
