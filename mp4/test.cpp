#include <stdio.h>
int main()
{
    char b[] = "\x11\x22\x33\x44";
    printf("%x\n", *(int*)b);
    return 0;
}
