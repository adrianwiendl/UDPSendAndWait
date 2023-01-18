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

// #define WAITTIME 5
//  Defines:
//  Variables for sequencing
int currentPacket = 0;
int expectedPacket = 0;
int totalPacketCount = 0;
struct timeval timeout;
struct fd_set fds;
        struct packet receivedPacket;
int sockfd;
int clielen, recvlen;
char recvbuf[BUFFERSIZE];
struct sockaddr_in6 saddr, caddr;
// saddr: everything for server (receiver)
// caddr: everything for client (sender)

int main(int argc, char *argv[])
{
    // Check correct program call
    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Usage: %s <output file> <port> <error=[packetnumber]>\n", argv[0]);
        exit(1);
    }

    // Program call with argument
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

    unsigned short port = atoi(argv[2]);
    char *output_file = argv[1];
    timeout.tv_sec = WAITTIME+1;

    // Acknowledgement
    struct acknowledgement s_ack;

    // Packet
    // struct packet receivedPacket;

    // Output
    char packetData[BUFFERSIZE];

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

    clielen = sizeof(caddr);

    FD_SET(sockfd, &fds);
    puts("Server is ready to receive.\n");
    do // Receives data as long as sender has not shut down.
    {

        // Set socket to non-blocking mode
        u_long iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);

        int recvlen = receiveFromClient(sockfd, caddr);
        if (recvlen > 0)
        {
            //
            // Successfully received data
            printf("===============Packet [%d]===============\n", totalPacketCount);

            // Remove trailing newline character for better diagnosis output in storage variable
            strcpy(packetData, receivedPacket.textData);
            packetData[strcspn(packetData, "\n")] = 0;

            // Print status
            printf("Received sequence-no. [%d] with data [\"%s\"] and checksum [%d] in packet [%d].\n",
                   receivedPacket.seqNr,
                   packetData,
                   receivedPacket.checksum,
                   totalPacketCount);

            int calculatedChecksum = generateChecksum(receivedPacket.textData, strlen(receivedPacket.textData));
            printf("Calculated checksum: [%ld].\n", calculatedChecksum);
            if (calculatedChecksum != receivedPacket.checksum)
            {
                // Incorrect Checksum received.
                // Requesting resend!

                printf("Received checksum [%d]. Does not match calculated checksum [%d]. Awaiting resend.\n",
                       receivedPacket.checksum,
                       calculatedChecksum);
                s_ack.seqNr = expectedPacket;

                strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                s_ack.ackChecksum = calculatedChecksum;
            }

            else if (receivedPacket.seqNr != expectedPacket)
            {
                printf("Received packet with unexpected sequence number [%d]. Expected [%d].\n",
                       receivedPacket.seqNr,
                       expectedPacket);

                s_ack.seqNr = expectedPacket - 1;
                strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                s_ack.ackChecksum = calculatedChecksum;
            }
            else
            {
                // Correct checksum
                // Packet as expected
                printf("Checksum and sequence-no. correct...\n");
                s_ack.seqNr = expectedPacket;
                strcpy(s_ack.ack, ACKNOWLEDGEMENT);
                s_ack.ackChecksum = calculatedChecksum;
                
                // Save received text to file; await next packet
                fputs(receivedPacket.textData, outfile); 

                expectedPacket++;
            }
            // Acknowledgement prepared according to received packet
            //--> send acknowledgement (s_ack)
            // Unless error case of missing acknowledgement is set and active
            if (provokeMissingAck(s_ack.seqNr, MissingAckPack) != 0) // Provoke missing acknowledgement on chosen packet
            {
                if (sendToClient(sockfd, s_ack, caddr) != 0)
                {
                    // Error sending acknowledgement. Break.
                    printf("Error Sending Acknowledgment. Error Code: %d\n", WSAGetLastError());
                    // Unsure how to proceed here. Resend acknowledgement? Wait?
                    break;
                }
                else // Acknowledgement sent
                {
                    printf("Successfully sent acknowledgement \"%s\" of seq-no [%d] with checksum [%d] in packet [%d]\n\n",
                    ACKNOWLEDGEMENT,
                    s_ack.seqNr,
                    s_ack.ackChecksum,
                    totalPacketCount);
                }
            }
            totalPacketCount++;
        }
        else if (recvlen == 0)
        {
            //
            continue;
        }
        else if (recvlen == -1)
        {
            //Transmission stopped
            break;
        }
        else
        {
            // Error on Select
        }
    } while (TRUE);

    // Close the output file and the UDP socket; clean up
    printf("No new transmission received after %d seconds, assuming EOF was reached or transmission has ended.\n", WAITTIME);
    puts("Saving and exiting...");
    fclose(outfile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}

int checkChecksum(int receivedChecksum, char *text)
{
    int calculatedChecksum = generateChecksum(text, strlen(text));

    if (receivedChecksum == calculatedChecksum)
    {
        // Checksums match
        return receivedChecksum;
    }
    return calculatedChecksum;
}

int receiveFromClient(int sockfd, struct sockaddr_in6 dataOfClient)
{
    // struct packet receivedPacket;
    // Listen on Socket
    listen(sockfd, 5);
    int res = select(sockfd + 1, &fds, NULL, NULL, &timeout); // wait for connection
    int recvlen;
    
    printf("Incoming connection requests:  [%d].\n", res);

    if (res > 0) // Receiving something
    {
        printf("Receiving...\n");
        int size = sizeof(caddr);
        recvlen = recvfrom(sockfd, (unsigned char *)&receivedPacket, sizeof(struct packet), 0, (struct sockaddr *)&caddr, &size); // Receive data
    }
    else if (res == 0)
    {
        printf("No answer received in timeframe.\n");
        recvlen = 0;
    }
    else // Error on select-timeout, res < 0
    {
        if (errno == 0)
        {
            printf("Receive Error. Error Code: [%d]\n", errno);//EOF reached.\n
            recvlen = -1;
        }
        else
        {
            printf("Error on select(). Error Code: [%d]\n", errno);
            recvlen = -2;
        }
        
    }

    return recvlen;
}

// saw_send-Function
int sendToClient(int sockfd, struct acknowledgement ackToSend, struct sockaddr_in6 dataOfClient)
{
    int sendlen = sizeof(ackToSend);

    if (sendto(sockfd, (unsigned char *)&ackToSend, sendlen, 0, (struct sockaddr *)&dataOfClient, sizeof(dataOfClient)) != sendlen)
    {
        // Error on sending
        printf("Error sending Acknowledgement. Error Code: [%d].\n", WSAGetLastError());
        return (-1);
    }
    return 0;
}