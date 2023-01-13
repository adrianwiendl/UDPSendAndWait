#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "checksum.h"
#include "structs.h"
#define WAITTIME 5

int main(int argc, char *argv[])
{
    // Check correct program call
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <output file> <port> \n", argv[0]);
        exit(1);
    }

    WSADATA wsaData;
    // Initialize the socket API with the version of the
    // Winsock specification that we want to use
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        // An error occurred
        perror("Error initializing the socket API");
        return (-1);
    }
    // Variables for sequencing
    int currentPacket = 0;
    int expectedPacket = 0;
    int totalPacketCount = 0;

    int sockfd, newsockfd;
    int clielen, recvlen;
    char recvbuf[BUFFERSIZE];
    struct sockaddr_in6 saddr, caddr;
    struct timeval timeout;
    unsigned short port = atoi(argv[2]);
    char *output_file = argv[1];
    timeout.tv_sec = WAITTIME;

    struct fd_set fds;
    FD_ZERO(&fds);

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket");
        return (-1);
    }

    bzero((char *)&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_addr = in6addr_any;
    saddr.sin6_port = htons(port);

    // Bind socket to port
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Error binding socket");
        return (-1);
    }

    // Acknowledgement
    struct acknowledgement s_ack;

    // Open the output file for writing
    FILE *outfile = fopen(output_file, "w");
    if (outfile == NULL)
    {
        perror("Error opening output file");
        return (-1);
    }

    // Listen on Socket
    listen(sockfd, 5);

    clielen = sizeof(caddr);

    newsockfd = accept(sockfd, (struct sockaddr *)&caddr, &clielen);
    if (newsockfd < 0)
    {
        printf("Error accepting connection. Error Code: %d", WSAGetLastError());
        return (-1);
    }
    struct packet receivedPacket;
    // Receives files as long as sender has not shut down.
    int tempRetries = 0;
    /* data declaration */
    time_t start, end;

    FD_SET(newsockfd, &fds);
    /* ... */

    do
    {
        // Set socket to non-blocking mode
        u_long iMode = 1;
        ioctlsocket(newsockfd, FIONBIO, &iMode);

        listen(newsockfd, 5);
        int res = select(newsockfd + 1, &fds, NULL, NULL, &timeout);
        printf("res from select() %d\n", res);
        if (res > 0)
        {
            puts("RECEIVING");
            recvlen = recv(newsockfd, (unsigned char *)&receivedPacket, sizeof(struct packet), 0);
        }
        else
        {
            printf("i go cri now bye");
        }
        if (recvlen < 0)
        {
            printf("Error receiving. Error Code: %d", WSAGetLastError());
            fclose(outfile);
            closesocket(newsockfd);
            return (-1);
        }

        if (recvlen != 0)
        {
            // Print status
            printf("Received text %s of packet %d w/ checksum %d\n",
                   receivedPacket.textData,
                   receivedPacket.seqNr,
                   receivedPacket.checksum);

            int calculatedChecksum = generateChecksum(receivedPacket.textData, strlen(receivedPacket.textData));
            if (receivedPacket.seqNr == expectedPacket)
            {
                // Correct sequence
                // Check checksum

                printf("Calculated checksum: %ld\n", calculatedChecksum);
                if (receivedPacket.checksum == calculatedChecksum)
                {
                    // Correct checksum
                    // Send positive acknowledgement
                    s_ack.seqNr = expectedPacket;
                    strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                    s_ack.ackChecksum = calculatedChecksum;
                    int sendlen = sizeof(struct acknowledgement);

                    // Provoke missing acknowledgement  on 8th packet
                    //  if(s_ack.seqNr == 8 && tempRetries < 1)
                    //  {
                    //      printf("Forcing missing acknowledgement on 8th packet\n");
                    //      tempRetries++;
                    //      //Sleep(6000);
                    //      /* wait 2.5 seconds */
                    //      time(&start);
                    //      do time(&end); while(difftime(end, start) <= 5);
                    //      continue;
                    //  }

                    if ((send(newsockfd, (unsigned char *)&s_ack, sendlen, 0)) != sendlen)
                    {
                        // Error sending acknowledgement. Break.
                        printf("Error Sending Acknowledgment. Error Code: %d\n", WSAGetLastError());
                        // Unsure how to proceed here. Resend acknowledgement? Wait?
                        break;
                    }
                    else
                    {
                        // Acknowledgement sent
                        printf("Sent acknowledgement %s\n", s_ack.ack);
                        // Save received text to file
                        // Await next packet
                        fputs(receivedPacket.textData, outfile);
                        expectedPacket++;
                    }
                }
                else
                {
                    // Wrong checksum. //Do not send acknowledgement.
                    s_ack.seqNr = expectedPacket;
                    strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                    s_ack.ackChecksum = calculatedChecksum;
                    int sendlen = sizeof(struct acknowledgement);

                    puts("Received checksum does not match calculated checksum. Awaiting resend.");
                    if ((send(newsockfd, (unsigned char *)&s_ack, sizeof(s_ack), 0)) != sizeof(s_ack))
                    {
                        // Error sending acknowledgement. Break.
                        printf("Error Sending Acknowledgment. Error Code: %d\n", WSAGetLastError());
                        // Unsure how to proceed here. Resend acknowledgement? Wait?
                        break;
                    }
                    else
                    {
                        // Acknowledgement sent
                        printf("Sent acknowledgement of wrong checksum. Expecting %d, received %d\n", calculatedChecksum, receivedPacket.checksum);
                    }
                }
            }
            // ELSE-Branch for wrong sequence number
            else
            {
                printf("Received unexpected packet\n");
                s_ack.seqNr = expectedPacket - 1;
                strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                s_ack.ackChecksum = calculatedChecksum;
                if ((send(newsockfd, (unsigned char *)&s_ack, sizeof(s_ack), 0)) != sizeof(s_ack))
                {
                    // Error sending acknowledgement. Break.
                    printf("Error Sending Acknowledgment. Error Code: %d\n", WSAGetLastError());
                    // Unsure how to proceed here. Resend acknowledgement? Wait?
                    break;
                }
                else
                {
                    // Acknowledgement sent
                    printf("Sent acknowledgement of wrong sequential number. Expecting %d, received %d\n", expectedPacket, receivedPacket.seqNr);
                }
            }
        }
    } while (recvlen != 0);

    printf("Connection has been shut down. Saving and exiting.\n");
    // Close the output file and the UDP socket
    fclose(outfile);
    closesocket(newsockfd);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}