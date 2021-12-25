
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int MAXLNE = 64;

int udp_server_loop() 
{
    int listenfd, n;
    struct sockaddr_in servaddr;
    char buff[MAXLNE];

    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(56789);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    struct sockaddr_in client_addr;
    int clen;
    while (1) {
        n = recvfrom(listenfd, buff, MAXLNE, 0, &client_addr, &clen);
        if (n <= 0){
            printf("recv error: %d\n", n);
            break;
        }
        buff[n] = '\0';
        printf("recv msg from client: %s\n", buff);
        n = sendto(listenfd, buff, n, 0, &client_addr, clen);
        printf("send %d bytes\n", n);
        if (n <= 0){
            break;
        }
    }

    close(listenfd);
    return 0;
}

