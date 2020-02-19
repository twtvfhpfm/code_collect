#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

int main()
{
    struct timeval tv;
    while(1){
        if (gettimeofday(&tv, NULL) < 0){
            printf("get time failed\n");
        }
        else{
            printf("child time: %u.%u\n", tv.tv_sec, tv.tv_usec);
        }

        usleep(1000 * 10);

    }

    return 0;
}
