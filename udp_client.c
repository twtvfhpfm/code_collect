/***************************************************************************/
/*                                                                         */    
/* Client program which gets as parameter the server name or               */    
/*     address and tries to send the data into non-blocking server.        */    
/*                                                                         */    
/* The message is sent after 5 seconds of wait                             */    
/*                                                                         */    
/*                                                                         */    
/* based on Beej's program - look in the simple TCp client for further doc.*/    
/*                                                                         */    
/*                                                                         */    
/***************************************************************************/
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <fcntl.h>

#define PORT 80/* the port client will be connecting to */

#define MAXDATASIZE 100 /* max number of bytes we can get at once */


int sock_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);

    return flags;
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if (argc != 3) {
        fprintf(stderr,"usage: client hostname port\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    sock_set_nonblocking(sockfd);
    /*
    while(1){
        fd_set rfds, wfds;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(sockfd, &rfds);
        FD_SET(sockfd, &wfds);

        int ret = select(sockfd + 1, &rfds, &wfds, NULL, NULL);
        if (ret > 0){
            if (FD_ISSET(sockfd, &rfds)){
                printf("read\n");
            }
            if (FD_ISSET(sockfd, &wfds)){
                printf("write\n");
            }
        }
    }
    */

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(atoi(argv[2]));    /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */


    int send_flag = 0;
    struct timeval send_t, recv_t;
    int addr_len = sizeof(their_addr);
    while (1) {
        fd_set rfds, wfds;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(0, &rfds);
        FD_SET(sockfd, &rfds);
        if (send_flag){
            FD_SET(sockfd, &wfds);
        }

        int ret = select(sockfd+1, &rfds, &wfds, NULL, NULL);
        if (ret > 0){
            if (FD_ISSET(sockfd, &wfds)){
                printf("writable\n");
                int val;
                int len = sizeof(int);
                send_flag = 0;
                char send_buf[128] = {0};
                gettimeofday(&send_t, NULL);
                unsigned long long send_ms = send_t.tv_sec * 1000ULL + send_t.tv_usec / 1000;
                sprintf(send_buf, "timestamp: %llu", send_ms);

                if (sendto(sockfd, send_buf, strlen(send_buf)+1, 0, (struct sockaddr*)&their_addr, addr_len) == -1){
                    perror("send");
                    exit (1);
                }
                printf("After the send function \n");

            }
            if (FD_ISSET(sockfd, &rfds)){
                printf("readable\n");
                gettimeofday(&recv_t, NULL);
                if ((numbytes=recvfrom(sockfd, buf, MAXDATASIZE, 0, NULL, NULL)) == -1) {
                    perror("recv");
                    exit(1);
                }

                buf[numbytes] = '\0';
                unsigned long long send_ms = 0;
                sscanf(buf, "timestamp: %llu", &send_ms);
                unsigned long long recv_ms = recv_t.tv_sec * 1000ULL + recv_t.tv_usec / 1000;

                printf("text=: %s, time: %d\n", buf, recv_ms - send_ms);
            }
            else if (FD_ISSET(0, &rfds)){
                printf("read cmd\n");
                read(0, buf, MAXDATASIZE);
                send_flag = 1;
            }
        }


    }

    close(sockfd);

    return 0;
}


