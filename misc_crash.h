#ifndef _MISC_CRASH_H_
#define _MISC_CRASH_H_

#include <stdio.h>
#include <signal.h>

#define MAX_LINE_LEN 256

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef BOOL
#define BOOL int
#endif

#define MCRASH_DEFAULT_STACK_DEPTH 10
#define MCRASH_DEFAULT_BACKTRACE_SIGNAL SIGUSR2
#define MCRASH_DEFAULT_THREAD_WAIT_TIME 10
#define MCRASH_MAX_NUM_SIGNALS 30

#define MCRASH_DEBUG_ENABLE  /* undef to turn off debug */

#ifdef MCRASH_DEBUG_ENABLE
# define MCRASH_DEBUG_VERY_VERBOSE 1
# define MCRASH_DEBUG_VERBOSE      2
# define MCRASH_DEBUG_INFO         3
# define MCRASH_DEBUG_WARN         4
# define MCRASH_DEBUG_ERROR        5
# define MCRASH_DEBUG_OFF          6
# define MCRASH_DEBUG_DEFAULT	  (MCRASH_DEBUG_ERROR)
# define DPRINTF(level, fmt...) \
		if (level >= gbl_params.debugLevel) { printf(fmt); fflush(stdout); }
#else /* MCRASH_DEBUG_ENABLE */
# define DPRINTF(level, fmt...) 
#endif /* MCRASH_DEBUG_ENABLE */

/** \struct mCrashParameters
 *  \brief mCrash Initialization Parameters
 *  
 *  This structure contains all the global initialization functions
 *  for mCrash.
 * 
 *  @see mCrash_Init
 */
typedef struct {
	/*  OUTPUT OPTIONS */
    /** Filename to output to, or NULL */
	char *filename;			
    /** FILE * to output to or NULL */
	FILE *filep;
    /** fd to output to or -1 */
	int	fd;

	int debugLevel;

    /** How far to backtrace each stack */
	unsigned int maxStackDepth;

    /** Default signal to use to tell a process to drop it's
     *  stack.
     */
	int defaultBacktraceSignal;

    /** If this is non-zero, the dangerous function, backtrace_symbols
     * will be used.  That function does a malloc(), so is not async
     * signal safe, and could cause deadlocks.
     */
	int useBacktraceSymbols; 

    /** To avoid the issues with backtrace_symbols (see comments above)
     * the caller can supply it's own symbol table, containing function
     * names and start addresses.  This table can be created using 
     * a script, or a static table.
     *
     * If this variable is not null, it will be used, instead of 
     * backtrace_symbols, reguardless of the setting
     * of useBacktraceSymbols.
     */
	/* mCrashSymbolTable *symbolTable; */

    /** Array of crash signals to catch,
     *  ending in 0  I would have left it a [] array, but I
     *  do a static copy, so I needed a set size.
     */
	int signals[MCRASH_MAX_NUM_SIGNALS];

} mCrashParameters_t;

int miscCrash_register(char *fdpath);
 
#endif
