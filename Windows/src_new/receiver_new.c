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

//#define WAITTIME 5

int main(int argc, char *argv[])
{
    // Check correct program call
    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Usage: %s <output file> <port>\n", argv[0]);
        exit(1);
    }

    //
    if(argc == 4)
    {
        char* r1 = argv[3];
        menuReceiver(r1);
    }
    puts("============SAW-Protokoll auf Basis von UDP-Sockets============");
    puts("Beleg im Modul I160: RN/KS im WS2022/23 an der HTW Dresden");
    puts("Martin Dittrich, Michael Novak, Benjamin Kunath, Adrian Wiendl");
    puts("===============================================================");
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

    int sockfd;
    int clielen, recvlen;
    char recvbuf[BUFFERSIZE];
    struct sockaddr_in6 saddr, caddr;
    //saddr: everything for server (receiver)
    //caddr: everything for client (sender)
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
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("Error creating socket");
        return (-1);
    }

    bzero((char *)&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_addr = in6addr_any;
    saddr.sin6_port = htons(port);

    bzero((char *)&caddr, sizeof(caddr));
    caddr.sin6_family = AF_INET6;

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

    FD_SET(sockfd, &fds);
    puts("Server is ready to receive.\n");
    do // Receives data as long as sender has not shut down.
    {
        // Set socket to non-blocking mode
        u_long iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);

        listen(sockfd, 5);
        int res = select(sockfd + 1, &fds, NULL, NULL, &timeout); // wait for connection

        printf("Incoming connection requests:  [%d].\n", res);
        if (res > 0) // Receiving something
        {
            printf("Receiving...\n");
            int size = sizeof(caddr);
            recvlen = recvfrom(sockfd, (unsigned char *)&receivedPacket, sizeof(struct packet), 0, (struct sockaddr *)&caddr, &size); // Receive data

            // Remove trailing newline character for better diagnosis output in storage variable
            strcpy(packetData, receivedPacket.textData);
            packetData[strcspn(packetData, "\n")] = 0;
        }
        else if (res == 0)
        {
            printf("No answer received in timeframe.\n");
            recvlen = 0;
            continue;
        }
        else // Error on select-timeout, res < 0
        {
            if(errno == 0)
            {
                printf("Receive Error. Error Code: [%d]\nEOF reached.\n",errno);
            }
            else
            {
                printf("Error on select(). Error Code: [%d]\n",errno);
            }
            break;
        }

        if (recvlen < 0) // Receive error
        {
            printf("Error receiving. Code [%d].", WSAGetLastError());
            fclose(outfile);
            closesocket(sockfd);
            return (-1);
        }

        // Successfully received data
        else if (recvlen != 0)
        {
            totalPacketCount++;
            // Print status
            printf("Received sequence-no. [%d] with data [\"%s\"] and checksum [%d] in packet [%d].\n",
                   receivedPacket.seqNr,
                   packetData,
                   receivedPacket.checksum,
                   totalPacketCount);

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
                    printf("Missing ack pack test output: %d\n", MissingAckPack);
                    if (provokeMissingAck(s_ack.seqNr, MissingAckPack) != 0)
                    {
                        if ((sendto(sockfd, (unsigned char *)&s_ack, sendlen, 0, (struct sockaddr *)&caddr, sizeof(caddr))) != sendlen) // Sending acknowledgement failed
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

                    if ((sendto(sockfd, (unsigned char *)&s_ack, sendlen, 0, (struct sockaddr *)&caddr, sizeof(caddr))) != sendlen)
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
                int sendlen = sizeof(struct acknowledgement);
                if ((sendto(sockfd, (unsigned char *)&s_ack, sendlen, 0, (struct sockaddr *)&caddr, sizeof(caddr))) != sendlen)
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
    } while (TRUE);

    // Close the output file and the UDP socket; clean up
    printf("No new transmission received after %d seconds, assuming EOF was reached.\n",WAITTIME);
    puts("Saving and exiting...");
    fclose(outfile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}