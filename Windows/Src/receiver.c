#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define BUFFERSIZE 1024
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

int main (int argc, char* argv[])
{
    //Check correct program call
    if (argc != 3) {
    fprintf(stderr, "Usage: %s <output file> <port> \n", argv[0]);
    exit(1);
    }

     WSADATA wsaData;


    //Initialize the socket API with the version of the
    //Winsock specification that we want to use
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // An error occurred
        perror("Error initializing the socket API");
        return (-1);
    }   

    int sockfd, newsockfd;
    int clielen, recvlen;
    char recvbuf[BUFFERSIZE];
    struct sockaddr_in saddr, caddr;
    int port = atoi(argv[2]);
    char* output_file = argv[1];

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket");
        return (-1);
    }

    bzero ((char* ) &saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl (INADDR_ANY);
    saddr.sin_port = htons(port);

    //Bind socket to port
    if (bind(sockfd, (struct sockaddr* ) &saddr, sizeof(saddr)) < 0)
    {
        perror("Error binding socket");
        return (-1);
    }

    // Open the output file for writing
    FILE *outfile = fopen(output_file, "w");
    if (outfile == NULL) {
        perror("Error opening output file");
        return (-1);
    }

    //Listen on Socket
    listen (sockfd, 5);

    clielen = sizeof(caddr);

    newsockfd = accept (sockfd, (struct sockaddr*) &caddr, &clielen);
    if (newsockfd < 0)
    {
        perror("Error accepting");
        return (-1);
    }

    //Receives files as long as sender has not shut down.
    do
    {
        recvlen = recv(newsockfd, recvbuf, BUFFERSIZE, 0);
        if(recvlen < 0)
        {
            printf("Error receiving. Error Code: %d",WSAGetLastError());
            fclose(outfile);
            closesocket(newsockfd);
            return (-1);
        }
        recvbuf[recvlen] = 0;

        if (recvlen != 0)
        {
            //Write received line to file
            printf("Received: %s\n", recvbuf);
            fputs((char*)recvbuf, outfile);
        }
    }while(recvlen != 0);

    printf("Connection has been shut down. Saving and exiting.\n");
    //Close the output file and the UDP socket
    fclose(outfile);
    closesocket(newsockfd);
    
    return 0;
}