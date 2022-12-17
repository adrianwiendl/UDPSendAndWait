// gcc -Wall -o clie_tcp_simplex muster_client.c (-lsocket -lnsl)

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFERSIZE	20
#define PORT		7923
#define SERVER		"127.0.0.1"

int main(void) {
	int					sockfd;
	int 				sendlen;
	char				sendbuf[BUFFERSIZE];
	struct sockaddr_in	saddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	bzero((char*) &saddr, sizeof(saddr));
	saddr.sin_family	= AF_INET;
	saddr.sin_addr.s_addr	= inet_addr(SERVER);
	saddr.sin_port		= htons(PORT);

	strcpy(sendbuf, "Hello, world!");
	sendlen = strlen(sendbuf);

	if(connect(sockfd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		perror("connect");
		return -1;
	}

	if(send(sockfd, sendbuf, strlen(sendbuf), 0) != sendlen) {
		perror("send");
		return -1;
	}

	close(sockfd);
	return 0;
}
