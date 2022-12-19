#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "packetStruct.h"
#include "checksum.h"


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
    //Variables for sequencing
    int currentPacket = 0;
    int expectedPacket = 0;
    int totalPacketCount = 0;

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
        printf("Error accepting connection. Error Code: %d",WSAGetLastError());
        return (-1);
    }
    struct packet receivedPacket;
    char receivedPacketSerialized[BUFFERSIZE];
    //Receives files as long as sender has not shut down.
    do
    {
        //recvlen = recv(newsockfd, recvbuf, BUFFERSIZE, 0);
        //recvlen = recv(newsockfd, receivedPacketSerialized, BUFFERSIZE, 0);
        //receivedPacketSerialized[recvlen] = 0;
        recvlen = recv(newsockfd, (unsigned char* )&receivedPacket, sizeof(struct packet), 0);
        if(recvlen < 0)
        {
            printf("Error receiving. Error Code: %d",WSAGetLastError());
            fclose(outfile);
            closesocket(newsockfd);
            return (-1);
        }
        
        if (recvlen != 0)
        {
            //Print status
            printf("Received text %s of packet %d w/ checksum %d\n",
                                            receivedPacket.textData,
                                            receivedPacket.seqNr,
                                            receivedPacket.checksum);
            if (receivedPacket.seqNr == expectedPacket)
            {
                //Correct sequence
                //Check checksum
                int calculatedChecksum = generateChecksum(receivedPacket.textData, strlen(receivedPacket.textData));
                printf("Calculated checksum: %ld\n",calculatedChecksum);
                if (receivedPacket.checksum == calculatedChecksum)
                {
                    //Correct checksum
                    //Send positive acknowledgement
                    if((send(newsockfd, ACKNOWLEDGEMENT, strlen(ACKNOWLEDGEMENT),0)) != strlen(ACKNOWLEDGEMENT))
                    {
                        //Error sending acknowledgement. Break.
                        printf("Error Sending Acknowledgment. Error Code: %d\n",WSAGetLastError());
                        //Unsure how to proceed here. Resend acknowledgement? Wait?
                        break;
                    }
                    else
                    {
                        //Acknowledgement sent
                        printf("Sent acknowledgement\n");
                        //Save received text to file
                        //Await next packet
                        fputs(receivedPacket.textData, outfile);
                        expectedPacket++;
                    }
                }
                else
                {
                    //Wrong checksum. Do not send acknowledgement.
                    puts("Received checksum does not match calculated checksum. Awaiting resend.");
                }
            }
        }
    }while(recvlen != 0);

    printf("Connection has been shut down. Saving and exiting.\n");
    //Close the output file and the UDP socket
    fclose(outfile);
    closesocket(newsockfd);
    
    return 0;
}