// gcc -Wall -o serv_tcp_simplex muster_server.c (-lsocket -lnsl)

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFERSIZE	20
#define PORT		7923

int main(void) {
	int					sockfd, newsockfd;
	int					clielen, recvlen;
	char				recvbuf[BUFFERSIZE];
	struct sockaddr_in	saddr, caddr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	bzero((char *) &saddr, sizeof(saddr));
	saddr.sin_family		= AF_INET;
	saddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	saddr.sin_port			= htons(PORT);

	if(bind(sockfd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		perror("bind");
		return -1;
	}

	listen(sockfd, 5);

	clielen = sizeof(caddr);
	newsockfd = accept(sockfd, (struct sockaddr *) &caddr, &clielen);

	if(newsockfd < 0) {
		perror("accept");
		return -1;
	}

	recvlen = recv(newsockfd, recvbuf, BUFFERSIZE, 0);

	if(recvlen < 0) {
		perror("receive");
		return -1;
	}

	recvbuf[recvlen] = 0;
	printf("%s\n", recvbuf);

	close(sockfd);
	close(newsockfd);
	return 0;

}
