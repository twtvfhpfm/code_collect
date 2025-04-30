#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
void expired(union sigval timer_data);
pid_t gettid(void);
struct t_eventData{
    int myData;
};
int main()
{
    int res = 0;
    timer_t timerId = 0;
    struct t_eventData eventData = { .myData = 0 };
    /*  sigevent 指定了过期时要执行的操作  */
    struct sigevent sev = { 0 };
    /* 指定启动延时时间和间隔时间 
    * it_value和it_interval 不能为零 */
    struct itimerspec its = {   .it_value.tv_sec  = 2,
                                .it_value.tv_nsec = 0,
                                .it_interval.tv_sec  = 0,
                                .it_interval.tv_nsec = 0
                            };
    printf("Simple Threading Timer - thread-id: %d\n", gettid());
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &expired;
    sev.sigev_value.sival_ptr = &eventData;
    /* 创建定时器 */
    res = timer_create(CLOCK_REALTIME, &sev, &timerId);
    if (res != 0){
        fprintf(stderr, "Error timer_create: %s\n", strerror(errno));
        exit(-1);
    }
    /* 启动定时器 */
    res = timer_settime(timerId, 0, &its, NULL);
    if (res != 0){
        fprintf(stderr, "Error timer_settime: %s\n", strerror(errno));
        exit(-1);
    }
    printf("Press ETNER Key to Exit\n");
    while(getchar()!='\n'){}
    return 0;
}
void expired(union sigval timer_data){
    struct t_eventData *data = timer_data.sival_ptr;
    printf("Timer fired %d - thread-id: %d\n", ++data->myData, gettid());
}
