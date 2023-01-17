#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "checksum.h"
#include "structs.h"
#include "sim_errors.h"
#include "arguments.h"

#define WAITTIME 5

int main(int argc, char *argv[])
{
    // Check correct program call
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <output file> <port>\n", argv[0]);
        exit(1);
    }

    WSADATA wsaData;
    // Initialize the socket API with the version of the
    // Winsock specification that we want to use
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        // Error initializing socket API
        perror("Error initializing the socket API.");
        return (-1);
    }

    // Defines:
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

    // Acknowledgement
    struct acknowledgement s_ack;

    // Packet
    struct packet receivedPacket;

    // Output
    char packetData[BUFFERSIZE];

    struct fd_set fds;
    FD_ZERO(&fds);

    // Create socket
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
        perror("Error binding socket.");
        return (-1);
    }

    // Open the output file for writing
    FILE *outfile = fopen(output_file, "w");
    if (outfile == NULL)
    {
        perror("Error opening output file.");
        return (-1);
    }

    // Listen on Socket
    listen(sockfd, 5);

    clielen = sizeof(caddr);
    newsockfd = accept(sockfd, (struct sockaddr *)&caddr, &clielen);
    if (newsockfd < 0)
    {
        printf("Error accepting connection. Code [%d].", WSAGetLastError());
        return (-1);
    }

    FD_SET(newsockfd, &fds);

    do // Receives data as long as sender has not shut down.
    {
        // Set socket to non-blocking mode
        u_long iMode = 1;
        ioctlsocket(newsockfd, FIONBIO, &iMode);

        listen(newsockfd, 5);
        int res = select(newsockfd + 1, &fds, NULL, NULL, &timeout); // wait for connection
        printf("Answer received:  [%d].\n", res);
        if (res > 0) // Receiving something
        {
            printf("Receiving...\n");
            recvlen = recv(newsockfd, (unsigned char *)&receivedPacket, sizeof(struct packet), 0); // Receive data

            // Remove trailing newline character for better diagnosis output
            strcpy(packetData, receivedPacket.textData);
            packetData[strcspn(packetData, "\n")] = 0;
        }
        else // Error or select-timeout
        {
            printf("Error during select(). Code [%d].", res);
        }

        if (recvlen < 0) // Receive error
        {
            printf("Error receiving. Code [%d].", WSAGetLastError());
            fclose(outfile);
            closesocket(newsockfd);
            return (-1);
        }

        // Successfully received data
        if (recvlen != 0)
        {
            // Print status
            printf("Received packet [%d] with data [\"%s\"] and checksum [%d].\n",
                   receivedPacket.seqNr,
                   packetData,
                   receivedPacket.checksum);

            // Calculate checksum
            int calculatedChecksum = generateChecksum(receivedPacket.textData, strlen(receivedPacket.textData));
            if (receivedPacket.seqNr == expectedPacket) // Correct sequence number; check checksum next
            {
                printf("Calculated checksum: [%ld].\n", calculatedChecksum);
                if (receivedPacket.checksum == calculatedChecksum) // Correct checksum; send acknowledgement next
                {
                    printf("Checksum correct.\n");
                    s_ack.seqNr = expectedPacket;
                    strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                    s_ack.ackChecksum = calculatedChecksum;
                    int sendlen = sizeof(struct acknowledgement);

                    // Provoke missing acknowledgement on 8th packet
                    if (provokeMissingAck(s_ack.seqNr, 8) != 0)
                    {
                        if ((send(newsockfd, (unsigned char *)&s_ack, sendlen, 0)) != sendlen) // Sending acknowledgement failed
                        {
                            printf("Error Sending Acknowledgment. Error Code: %d\n", WSAGetLastError());
                            // Unsure how to proceed here. Resend acknowledgement? Wait?
                            break;
                        }
                        else // Acknowledgement successfully sent
                        {
                            printf("Sent acknowledgement %s [%d] [%d]\n\n", s_ack.ack, s_ack.seqNr, s_ack.ackChecksum);
                            fputs(receivedPacket.textData, outfile); // Save received text to file; await next packet
                            expectedPacket++;
                        }
                    }

                }
                else // Wrong checksum. Do not send acknowledgement.
                {
                    s_ack.seqNr = expectedPacket;
                    strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                    s_ack.ackChecksum = calculatedChecksum;
                    int sendlen = sizeof(struct acknowledgement);

                    printf("Received checksum [%d]. Does not match calculated checksum [%d]. Awaiting resend.\n",
                            receivedPacket.checksum,
                            calculatedChecksum);

                    if ((send(newsockfd, (unsigned char *)&s_ack, sizeof(s_ack), 0)) != sizeof(s_ack))
                    {
                        // Error sending acknowledgement. Break.
                        printf("Error Sending Acknowledgment. Error Code: %d\n", WSAGetLastError());
                        // Unsure how to proceed here. Resend acknowledgement? Wait?
                        break;
                    }
                    else // Acknowledgement sent
                    {
                        printf("Sent acknowledgement of wrong checksum. Expecting [%d], received [%d].\n",
                                calculatedChecksum,
                                receivedPacket.checksum);
                    }
                }
            }
            // ELSE-Branch for wrong sequence number
            else
            {
                printf("Received packet with unexpected sequence number [%d]. Expected [%d].\n",
                        receivedPacket.seqNr,
                        expectedPacket);

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
                    printf("Sent acknowledgement of wrong sequential number. Expecting [%d], received [%d].\n",
                            expectedPacket,
                            receivedPacket.seqNr);
                }
            }
        }
    } while (recvlen != 0);

    // Close the output file and the UDP socket; clean up
    printf("Connection has been shut down. Saving and exiting.\n");
    fclose(outfile);
    closesocket(newsockfd);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}