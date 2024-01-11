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
#include <time.h>

#define PORT 80/* the port client will be connecting to */

#define MAXDATASIZE 1200 /* max number of bytes we can get at once */

#define SEND_SIZE 160

unsigned long long get_time_ms()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000ULL + t.tv_usec / 1000;
}

int sock_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);

    return flags;
}

int check_send(unsigned long long send_t, int send_interval)
{
    unsigned long long now = get_time_ms();
    if (now - send_t > send_interval) {
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    unsigned long long recv_buf[SEND_SIZE];
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
    unsigned long long send_t, recv_t;
    int addr_len = sizeof(their_addr);
    int send_interval = 10;
    int send_rate = 0;
    int send_bytes_count = 0;
    unsigned long long last_send_rate_time = 0;
    unsigned long long seq = 0;
    unsigned long long last_lost_rate_time = get_time_ms();
    unsigned long long last_lost_rate_seq = 0;
    int lost_rate = 0;
    int packets_recved = 0;
    char file_name[128];
    sprintf(file_name, "udp_rtt_%ld.log", time(NULL));
    FILE* log_fp = fopen(file_name, "w");
    while (1) {
        fd_set rfds, wfds;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(0, &rfds);
        FD_SET(sockfd, &rfds);
        if (send_flag){
            FD_SET(sockfd, &wfds);
        }


        struct timeval timeout = {.tv_sec = 0, .tv_usec = 1000};
        int ret = select(sockfd+1, &rfds, &wfds, NULL, &timeout);
        if (ret > 0){
            if (FD_ISSET(sockfd, &wfds)){
                send_flag = 0;
                unsigned long long send_buf[SEND_SIZE] = {0};
                send_t = get_time_ms();
                send_buf[0] = send_t;
                send_buf[1] = seq++;

                if (sendto(sockfd, send_buf, sizeof(send_buf), 0, (struct sockaddr*)&their_addr, addr_len) == -1){
                    perror("send");
                    exit (1);
                }
		send_bytes_count += sizeof(send_buf);
		if (send_t - last_send_rate_time > 1000) {
			last_send_rate_time = send_t;
			send_rate = send_bytes_count * 8;
			send_bytes_count = 0;
		}

            }
            if (FD_ISSET(sockfd, &rfds)){
                send_flag = check_send(send_t, send_interval);
                recv_t = get_time_ms();
                if ((numbytes=recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, NULL, NULL)) == -1) {
                    perror("recv");
                    exit(1);
                }
                packets_recved++;
                if (recv_t - last_lost_rate_time > 1000) {
                    last_lost_rate_time = recv_t;
                    lost_rate = 100 - 100 * packets_recved / (recv_buf[1] - last_lost_rate_seq);

                    packets_recved = 0;
                    last_lost_rate_seq = recv_buf[1];
                    fflush(log_fp);
                }

                time_t t = time(NULL);
                char time_buf[128] = {0};
                sprintf(time_buf, "%s", ctime(&t));
                sprintf(time_buf + strlen(time_buf) - 1, " seq %8llu rtt %8llu lost %8llu rate %8llu\n", recv_buf[1], recv_t - recv_buf[0], lost_rate, send_rate);
                fwrite(time_buf, strlen(time_buf), 1, log_fp);
                printf("%s", time_buf);
            }
            else if (FD_ISSET(0, &rfds)){
                char tmp_buf[128] = {0};
                read(0, tmp_buf, MAXDATASIZE);
                switch(tmp_buf[0]) {
                    case 'i': {
                          int i=0;
                          sscanf(tmp_buf+1, "%d", &i);
                          if (i > 0) {
                              printf("set send interval: %d\n", i);
                              send_interval = i;
                          }
                      }
                      break;
                }
            }
        } else if (ret == 0){
            send_flag = check_send(send_t, send_interval);
        }


    }

    close(sockfd);

    return 0;
}


