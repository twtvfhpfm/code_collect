#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

int main()
{
	struct sockaddr_in from_addr;
	from_addr.sin_addr.s_addr = 0;

	do{
		int sock = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in temp;
		temp.sin_family = AF_INET;
		temp.sin_addr.s_addr = 0;
		temp.sin_port = 8888;
		if (bind(sock, (struct sockaddr*)&temp, sizeof(temp)) != 0){
			printf("bind failed\n");
		}
		int loop = 1;
		setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
		int ttl = 0;
		setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
		struct sockaddr_in dest;
		dest.sin_family = AF_INET;
		dest.sin_addr.s_addr = inet_addr("228.23.45.67"); //arbitrary multicast address
		dest.sin_port = 8888;

		struct ip_mreq imr;
		imr.imr_multiaddr.s_addr = dest.sin_addr.s_addr;
		imr.imr_interface.s_addr = 0;
		
		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(struct ip_mreq)) < 0){
			printf("join group failed\n");
		}

		char buffer[] = "hello world";
		int bytes_sent = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&dest, sizeof(dest));
		if (bytes_sent != sizeof(buffer)){
			printf("wrote %d bytes instead of %ld\n", bytes_sent, sizeof(buffer));
			break;
		}
		else{
			printf("send success\n");
		}

		int len = sizeof(from_addr);
		memset(buffer, 0, sizeof(buffer));
		int bytes_read = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&from_addr, &len);
		printf("read %d bytes: %s\n", bytes_read, buffer);
		printf("from: %s\n", inet_ntoa(from_addr.sin_addr));
	}while(0);

	return 0;
}
