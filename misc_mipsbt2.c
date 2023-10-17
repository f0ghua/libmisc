/**
 * @file   misc_mipsbt2.c
 * @author fog
 * @date   Mon Mar  8 18:21:49 2010
 * 
 * @brief  -rdynamic -ldl
 * 
 * 
 */
#define _GNU_SOURCE
#define __USE_GNU

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>

#if defined(REG_RIP)
# define SIGSEGV_STACK_IA64
# define REGFORMAT "%016lx"
#elif defined(REG_EIP)
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

int mips_backtrace(void **buffer, int size, void *uc)
{
    void *ip = NULL;
    void **bp = NULL;    
    int f = 0;
    Dl_info dlinfo;

    fprintf(stderr, "Start of stack trace\n");
    
#if defined(SIGSEGV_STACK_IA64)
    ip = (void*)ucontext->uc_mcontext.gregs[REG_RIP];
    bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
#elif defined(SIGSEGV_STACK_X86)
    ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
    bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
#endif

    while(bp && ip) {
        const char *symname;
        
        if(!dladdr(ip, &dlinfo))
            break;

        symname = dlinfo.dli_sname;

        fprintf(stderr, "% 2d: %p <%s+%u> (%s)\n",
                ++f,
                ip,
                symname,
                (unsigned)(ip - dlinfo.dli_saddr),
                dlinfo.dli_fname);


        if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
            break;

        ip = bp[1];
        bp = (void**)bp[0];
    }

    fprintf(stderr, "End of stack trace\n");

    exit(-1);
}
