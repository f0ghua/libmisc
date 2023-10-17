#include <stdio.h>

void lib_func_C(int i)
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

void lib_func_B(int i)
{
    i++;
    lib_func_C(i);
}

void lib_func_A()
{
    int i = 0;
    
    lib_func_B(i++);
}
