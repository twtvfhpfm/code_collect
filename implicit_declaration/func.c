#include <stdio.h>
#include <stdint.h>

int func(int64_t arg1, int arg2, int arg3, int arg4)
{
    printf("%lld, %d, %d, %d\n", arg1, arg2, arg3, arg4);
    return 0;
}
