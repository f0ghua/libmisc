#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "misc_crash.h"

#if 0
void func_C(int i)
{
#if 0
    int j = 0;
    
    while(j++ < 10)
    {
        sleep(1);
        printf("wait for the signal\n");
    }
#else
    char *p = NULL;
    *p = i;
#endif    
}

void func_B(int i)
{
    i++;
    func_C(i);
}

void func_A()
{
    int i = 0;
    
    func_B(i++);
}
#endif

void signalHandler( int sig, siginfo_t* siginfo, void* notused)
{
    /* Print out the signal info */
    printf("signal %d received\n", sig);

    exit(1);
}

int main(int argc, char **argv)
{
    pid_t pid;

    struct sigaction sa;

    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    sigaction( SIGHUP ,  &sa,  NULL );     /* Hangup (POSIX).  */
    sigaction( SIGINT ,  &sa,  NULL );     /* Interrupt (ANSI).  */
    sigaction( SIGQUIT,  &sa,  NULL );     /* Quit (POSIX).  */
    sigaction( SIGILL ,  &sa,  NULL );     /* Illegal instruction (ANSI).  */
    sigaction( SIGTRAP,  &sa,  NULL );     /* Trace trap (POSIX).  */
    sigaction( SIGABRT,  &sa,  NULL );     /* Abort (ANSI).  */
    sigaction( SIGEMT,   &sa,  NULL );
    sigaction( SIGFPE ,  &sa,  NULL );     /* Floating-point exception (ANSI).  */
    sigaction( SIGBUS,   &sa,  NULL );     /* BUS error (4.2 BSD).  */
    sigaction( SIGSEGV,  &sa,  NULL );     /* Segmentation violation (ANSI).  */
    sigaction( SIGSYS,   &sa,  NULL );
    sigaction( SIGPIPE,  &sa,  NULL );     /* Broken pipe (POSIX).  */
    sigaction( SIGALRM,  &sa,  NULL );     /* Alarm clock (POSIX).  */
    sigaction( SIGTERM,  &sa,  NULL );     /* Termination (ANSI).  */
    sigaction( SIGUSR1,  &sa,  NULL );     /* User-defined signal 1 (POSIX).  */
    sigaction( SIGUSR2,  &sa,  NULL );     /* User-defined signal 2 (POSIX).  */

    /* mCrash will call popen, so SIGCHLD should handle correctly */
    sigaction( SIGCHLD,  &sa,  NULL );     /* Child status has changed (POSIX).  */
    sigaction( SIGPWR,   &sa,  NULL );     /* Power failure restart (System V).  */
    sigaction( SIGWINCH, &sa,  NULL );     /* Window size change (4.3 BSD, Sun).  */
    sigaction( SIGURG,   &sa,  NULL );     /* Urgent condition on socket (4.2 BSD).  */
    sigaction( SIGIO,    &sa,  NULL );     /* I/O now possible (4.2 BSD).  */
    sigaction( SIGTSTP,  &sa,  NULL );     /* Keyboard stop (POSIX).  */
    sigaction( SIGCONT,  &sa,  NULL );     /* Continue (POSIX).  */
    sigaction( SIGTTIN,  &sa,  NULL );     /* Background read from tty (POSIX).  */
    sigaction( SIGTTOU,  &sa,  NULL );     /* Background write to tty (POSIX).  */
    sigaction( SIGVTALRM,&sa,  NULL );     /* Virtual alarm clock (4.2 BSD).  */
    sigaction( SIGPROF,  &sa,  NULL );     /* Profiling alarm clock (4.2 BSD).  */
    sigaction( SIGXCPU,  &sa,  NULL );     /* CPU limit exceeded (4.2 BSD).  */
    sigaction( SIGXFSZ,  &sa,  NULL );     /* File size limit exceeded (4.2 BSD).  */   

    printf("SIGCHLD=%d SIGCONT=%d SIGTTIN=%d, SIGTTOU=%d\n",
           SIGCHLD, SIGCONT, SIGTTIN, SIGTTOU);
   
    signal(SIGINT, SIG_IGN);

    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
   
    miscCrash_register("/dev/mtdblock5");

    pid = fork();
    if(pid == 0)
    {
        lib_func_A();
        //func_A();
        return 0;
    }
    
    return 0;
}
