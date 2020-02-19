#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

int main()
{
    int pfd[2];
    struct timeval tv;
    
    if (pipe(pfd) < 0){
        printf("pipe failed\n");
        return -1;
    }

    int pid = fork();
    if (pid < 0){
        printf("fork failed\n");
        return -1;
    }

    if (0 == pid){
        //child
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        close(pfd[0]);

        execl("./popen_child", NULL, NULL);
    }
    else{
        //parent
        close(pfd[1]);
        char buf[1024];

        FILE* fp = fdopen(pfd[0], "r");
        while(fgets(buf, 1024, fp) != NULL){
            gettimeofday(&tv, NULL);
            printf("parent time: %u.%u <-> %s\n", tv.tv_sec, tv.tv_usec, buf);
        }
    }

    return 0;
}
