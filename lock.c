#include <pthread.h>
#include <stdio.h>

int main()
{
    pthread_mutex_t mutex = {0};
    printf("%p %d\n", &mutex, sizeof(mutex));
    char* a = (char*)(&mutex + 1);
    printf("%p\n", a);
    pthread_mutex_lock(&mutex);

    return 0;
}
