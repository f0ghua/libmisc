/**
 * @file   misc_mipsbt.c
 * @author fog
 * @date   Sun Mar  7 11:35:09 2010
 * 
 * @brief
 * 
 * Purpose: standard backtrace function provided by glic does NOT
 *          support MIPS architecture. This file provids same
 *          functionality for MIPS, retreive the calling function
 *          stack pointer address based on current PC.
 *
 * Notice:  please take care that gcc optimize option(e.g. -O2) maybe
 *          re-strcutre the code, which disorder the .text
 *          instruction, then our function will not work. So remove
 *          them for debugging.
 * 
 * 
 */
/* #define F_DEBUG */
#define _SYS_UCONTEXT_H
#define _BITS_SIGCONTEXT_H

#include <signal.h>

#include "asm-mips/types.h"
#include "asm-mips/sigcontext.h"
#include "asm-mips/ucontext.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NREG_RA 31
#define NREG_SP 29

typedef struct pmapLine {
    unsigned long    vmStart;
    unsigned long    vmEnd;
    char             permission[5];
    unsigned long    vmPageOffset;
    char             dev[6];
    unsigned long    innodeNumber;
    struct pmapLine *next;
} pmapLine_t;

pmapLine_t *gbl_pmapLineHead = NULL;

static int bt_getProcessMaps(void)
{
    FILE *fp;
    pmapLine_t *plTail = NULL;
    pmapLine_t *pl = NULL;
    char pmapFile[64];
    char buf[1024];
    int pid;
    
    pid = getpid();
    snprintf(pmapFile, sizeof(pmapFile), "/proc/%ld/maps", (long)pid);
    if((fp = fopen(pmapFile, "r")) == NULL)
        return -1;

    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
#ifdef F_DEBUG        
        printf("%s", buf);
#endif        
        pl = calloc(1, sizeof(pmapLine_t));
        sscanf(buf,
               "%lx-%lx %4s %lx %5s %lu",
               &pl->vmStart, &pl->vmEnd, pl->permission,
               &pl->vmPageOffset, pl->dev, &pl->innodeNumber);
        pl->next = NULL;
        if(!gbl_pmapLineHead)
            gbl_pmapLineHead = pl;
        if(plTail)
            plTail->next = pl;
        plTail = pl;
    }

    fclose(fp);

    return 0;
}

/** 
 * To check if an memory address is readable
 * 
 * @param addr 
 * 
 * @return 
 */
static int bt_regularAddr(unsigned long addr)
{
    pmapLine_t *pl = gbl_pmapLineHead;
    
    if(!gbl_pmapLineHead)
        return 0;

    while(pl != NULL)
    {
        if((pl->permission[0] == 'r') &&
           (addr >= pl->vmStart) && (addr <= pl->vmEnd))
            return 1;
        pl = pl->next;
    }
    
    printf("Can't read address %08x\n", addr);
    return 0;
}

/** 
 * The idea of this function comes from internet, the process is like
 * this:
 *
 * 1. get ucontext info from sigaction, which inclued following
 *    addresses:
 * 
 *         $pc - current exectuing instruction address
 *         $ra - return address
 *         $sp - stack pointer of the process
 *         
 * 2. search prev instructions from $pc
 * 
 * 3. if we find any instrcution of saving $ra address, get the sp
 *    offset of it, so that we can read it later.
 *
 * 4. if we find any instrcution of sp change, then we know the stack
 *    size of this function. So we can find the ra address and parent
 *    fucntions's sp from the stack.
 *
 * This idea is based on mips architecture, store ra instruction
 * should always before the move sp instruction.
 * 
 * @param buffer 
 * @param size 
 * @param uc 
 * 
 * @return 
 */
int mips_backtrace(void **buffer, int size, void *uc)
{
    unsigned long *addr = NULL;
    unsigned long *pc = NULL;
    unsigned long *ra = NULL;
    unsigned long *sp = NULL;
    int depth = 0, stopFunc = 1;
    int raOffset, stackSize;

    if(bt_getProcessMaps() == -1)
        return depth;
    
    pc = (unsigned long *)(unsigned long)
        ((struct ucontext *)uc)->uc_mcontext.sc_pc;
    ra = (unsigned long *)(unsigned long)
        (((struct ucontext *)uc)->uc_mcontext.sc_regs[NREG_RA]);
    sp = (unsigned long *)(unsigned long)
        (((struct ucontext *)uc)->uc_mcontext.sc_regs[NREG_SP]);

#ifdef F_DEBUG    
    printf("$pc = %08lx\n", pc);
#endif
    
    if(!bt_regularAddr((unsigned long)pc))
    {
        return depth;
    }

    raOffset = stackSize = 0;
    buffer[depth++] = addr = pc;
    
    while(1)
    {
        if(!bt_regularAddr((unsigned long)addr))
            return depth;
        
        if(*addr == 0x1000ffff)
            return depth;
#ifdef F_DEBUG        
        printf("addr:%08lx, *addr:%08lx\n", addr, *addr);
#endif        
        switch(*addr & 0xffff0000)
        {
            case 0x23bd0000:
                /* 0x23bdxxxx: addi sp, sp, xxxx */
            case 0x27bd0000:
                /* 0x27bdxxxx: addiu sp, sp, xxxx */
                stackSize = (short)(*addr & 0xffff);
                /*
                 * For most cases, $sp decrease at when enter a
                 * function and increase when left it. So, if we find
                 * prev instruction from $pc, we will not meet the
                 * increase case. But, gcc optimize sometimes re-order
                 * the instruction, which make us meet the increase sp
                 * one, so we should ignore it.
                 */ 
                if(stackSize > 0)
                {
                    addr--;
                    continue;
                }
                stackSize = abs(stackSize);
                if((depth-1) && (!stackSize || !raOffset))
                    return depth;

                if(!bt_regularAddr((unsigned long)sp + raOffset))
                    return depth;

                if(raOffset)
                {
                    ra = (unsigned long *)(*(unsigned long *)
                                           ((unsigned long)sp + raOffset));
                    raOffset = 0;
                }

                if(stackSize)
                {
                    sp = (unsigned long *)
                        ((unsigned long)sp + stackSize);
                    stackSize = 0;
                }

#ifdef F_DEBUG                
                printf("* hit addi/addiu sp, $sp(0x%08lx), jump to $ra(0x%08lx)\n",
                       sp, ra);
#endif
                buffer[depth++] = addr = ra;
                if(depth >= size-1)
                    return depth;
                
                break;
            case 0xafbf0000:
                /* 0xafbfxxxx : sw ra ,xxxx(sp) */
            case 0xffbf0000:
#ifdef F_DEBUG                
                printf("* hit sw/sd ra, store ra offset\n");
#endif
                /* 0xffbfxxxx : sd ra ,xxxx(sp) */
                raOffset = (short)(*addr & 0xffff);
                --addr;
                break;
            default:
                --addr;
        }
    }

    return depth;
}
