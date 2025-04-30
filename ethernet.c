#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>

#define DEST_MAC0 0xc8
#define DEST_MAC1 0x47
#define DEST_MAC2 0x8c
#define DEST_MAC3 0x00
#define DEST_MAC4 0x00
#define DEST_MAC5 0x18

unsigned short calculate_checksum(void *buf, int len) {
    unsigned short *buffer = buf;
    unsigned int sum = 0;
    unsigned short result;
    
    for (sum = 0; len > 1; len -= 2) {
        sum += *buffer++;
    }
    if (len == 1) {
        sum += *(unsigned char *)buffer;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    
    return result;
}

int main() {
    int sockfd;
    struct ifreq if_idx;
    struct ifreq if_mac;
    struct sockaddr_ll socket_address;
    char sendbuf[ETH_FRAME_LEN];
    struct ether_header *eh = (struct ether_header *)sendbuf;
    struct iphdr *iph = (struct iphdr *)(sendbuf + sizeof(struct ether_header));
    struct icmphdr *icmph = (struct icmphdr *)(sendbuf + sizeof(struct ether_header) + sizeof(struct iphdr));
    int packet_len = 0;
    char buf[4]= {192,168,1,159};
    int data_len = sizeof(buf);

    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, "wlxbc307eab1114", IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
        perror("SIOCGIFINDEX");
        exit(EXIT_FAILURE);
    }

    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, "wlxbc307eab1114", IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        perror("SIOCGIFHWADDR");
        exit(EXIT_FAILURE);
    }

    memset(sendbuf, 0, ETH_FRAME_LEN);

    // Set Ethernet header
    eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
    eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
    eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
    eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
    eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
    eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];

    eh->ether_dhost[0] = DEST_MAC0;
    eh->ether_dhost[1] = DEST_MAC1;
    eh->ether_dhost[2] = DEST_MAC2;
    eh->ether_dhost[3] = DEST_MAC3;
    eh->ether_dhost[4] = DEST_MAC4;
    eh->ether_dhost[5] = DEST_MAC5;

    eh->ether_type = htons(ETH_P_IP);
    packet_len += sizeof(struct ether_header);

    // Set IP header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + data_len);
    iph->id = htons(0);
    iph->frag_off = htons(0);
    iph->ttl = 64;
    iph->protocol = IPPROTO_ICMP;
    iph->check = 0;
    iph->saddr = inet_addr("1.2.3.4"); // 源 IP 地址
    iph->daddr = inet_addr("255.255.255.255");   // 目标 IP 地址

    iph->check = calculate_checksum((unsigned short *)iph, sizeof(struct iphdr));
    packet_len += sizeof(struct iphdr);

    // Set ICMP header
    icmph->type = ICMP_INFO_REQUEST;
    icmph->code = 0;
    icmph->checksum = 0;
    icmph->un.echo.id = htons(getpid());
    icmph->un.echo.sequence = htons(1);
    memcpy(icmph+1, buf, data_len);
    icmph->checksum = calculate_checksum((unsigned short *)icmph, sizeof(struct icmphdr)+data_len);
    packet_len += sizeof(struct icmphdr)+data_len;

    // 填充 socket 地址结构
    socket_address.sll_ifindex = if_idx.ifr_ifindex;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, eh->ether_dhost, ETH_ALEN);

    // 发送以太网帧
    if (sendto(sockfd, sendbuf, packet_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("ICMP Echo Request sent\n");

    close(sockfd);
    return 0;
}
