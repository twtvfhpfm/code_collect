#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "module1.h"

int func1_1()
{
    printf("this is %s\n", __FUNCTION__);
    return 0;
}

int func1_2()
{
    printf("this is %s\n", __FUNCTION__);
    return 0;
}

int func2_1()
{
    printf("this is %s\n", __FUNCTION__);
    return 0;
}

int func2_2()
{
    printf("this is %s\n", __FUNCTION__);
    return 0;
}

int func1()
{
    func1_1();
    func1_2();
    return 0;
}

int func2()
{
    func2_1();
    func2_2();
    return 0;
}

void* thread1_func(void* arg)
{
    module1_set_name("hello world");
    usleep(1000 * 1000);
    return NULL;
}

int main()
{
    func1();
    func2();

    module1_init();
    pthread_t tid;
    pthread_create(&tid, NULL, thread1_func, NULL);

    int value = module1_get_value();
    printf("module1_value=%d\n", value);

    usleep(1000 * 1000 * 1000);

    return 0;
}