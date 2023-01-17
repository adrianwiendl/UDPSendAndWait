#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>

#include "structs.h"
#include "checksum.h"
#include "sim_errors.h"
#include "arguments.h"

//#define WAITTIME 5 // Seconds to wait for acknowledgement
#define MAXRETRIES 6

int main(int argc, char *argv[])
{
    // Check parameter count
    if (argc < 4 || argc > 6) // Incorrect program call
    {
        fprintf(stderr, "Usage: %s <input file> <port> <IPv6 address> <error=packetnumber>\n", argv[0]);
        exit(1);
    }

    if (argc > 4) // Program call with error arguments
    {
        char *e1 = argv[4];
        char *e2 = argv[5];

        if(argc < 6)
        {
          e2 = "-l=0";
        }

        menuSender(e1, e2);

    }

    WSADATA wsaData;
    // Initialize the socket API with the version of the
    // Winsock specification that we want to use
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        // Error initializing Windows Socket API
        printf("Error initializing the socket API. Error Code: %d", WSAGetLastError());
        return (-1);
    }

    // Variables for sequencing
    int currentPacket = 0;
    int totalPacketCount = 0;
    int packetRetries = 0;

    //
    int sockfd;
    char sendPacketBuffer[BUFFERSIZE];

    // Parse passed command-line arguemnts
    char *input_file = argv[1];
    unsigned short port = atoi(argv[2]);
    char *server = argv[3];

    //
    char recvbuf[BUFFERSIZE];
    int checksum;
    int sendlen;

    //
    struct packet packetToSend;
    struct sockaddr_in6 saddr, caddr;
    struct timeval timeout;
    struct acknowledgement recvAck;
    //
    char *receivedAcknowledgement;

    // Timeout for waiting on acknowledgement
    timeout.tv_sec = WAITTIME;

    // Output
    char packetData[BUFFERSIZE];

    // Open socket
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        // Error opening socket
        printf("Error opening socket. Error Code: [%d]", WSAGetLastError());
        WSACleanup();
        return (-1);
    }

    //
    struct fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    // Parse IP-Address and port
    bzero((char *)&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, server, &saddr.sin6_addr);
    saddr.sin6_port = htons(port);

    // Print Confirmation for user
    char buf[BUFFERSIZE];
    printf("Sending file [%s] to IP [%s] on PORT [%d].\n", input_file, inet_ntop(AF_INET6, &saddr.sin6_addr, buf, sizeof(buf)), ntohs(saddr.sin6_port));

    // Open the input file "input_file" for reading
    FILE *infile = fopen(input_file, "r");
    if (infile == NULL)
    {
        // Error opening input file
        perror("Error opening file.");
        closesocket(sockfd);
        WSACleanup();
        return (-1);
    }

    char lines[100][512]; // 2-Dimensional-Array for read lines & linenumbers

    int i = 0;
    while (fgets(lines[i], 512, infile))
    {
        i++;
    } // Read file line by line and store each line in the array

    /* Intended functionality outline:
     * 1.   Send 1st packet
     * 2.   Wait for acknowledgement (WAITTIME seconds)
     * 3.1  On positive acknowledgement:     reset timer and send 2nd packet and so on
     * 3.2  On negative/no acknowledgement:  resend 1st packet, as long as timer has not expired, else abort.
     * */

    // Run loop to send
    while (TRUE)
    {
        // Calculate the checksum for the line
        long checksum = generateChecksum(lines[currentPacket], strlen(lines[currentPacket]));

        // Provoke erroneous sequencing on 5th packet
        currentPacket = provokeSeqError(currentPacket, SeqErrorPack); // (current, packet to falsify)

        // Provoke erroneous checksum on 7th packet
        checksum = provokeChecksumError(currentPacket, checksum, CsmErrorPack); // (current, checksum, packet to falsify)

        // Prepare packet
        strcpy(packetToSend.textData, lines[currentPacket]);
        packetToSend.seqNr = currentPacket;
        packetToSend.checksum = checksum;

        // Remove trailing newline character for better diagnosis output
        strcpy(packetData, lines[currentPacket]);
        packetData[strcspn(packetData, "\n")] = 0;

        printf("\nCurrent Packet: [%d]. Content: [\"%s\"].\n", currentPacket, packetData);

        sendlen = sizeof(struct packet);
        if (sendto(sockfd, (unsigned char *)&packetToSend, sendlen, 0, (struct sockaddr *)&saddr, sizeof(saddr)) != sendlen) // Error on sending
        {
            printf("Error sending packet [%d]. Error Code: [%d].", currentPacket, WSAGetLastError());
            fclose(infile);
            closesocket(sockfd);
            WSACleanup();
            return (-1);
        }
        // Sending successful

        // Print line + checksum sent
        printf("Sent packet [%d] with data [\"%s\"] and checksum [%ld].\n", currentPacket, packetData, checksum);

        // Set socket to non-blocking mode
        u_long iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);

        // Listen on socket for acknowledgement for WAITTIME seconds
        listen(sockfd, 5);
        int res = select(sockfd + 1, &fds, NULL, NULL, &timeout);
        printf("Incoming connection requests: [%d].\n", res);

        if (res == 1 && packetRetries < MAXRETRIES)
        {
            // Answer received.
            int size = sizeof(saddr);
            int recvlen = recvfrom(sockfd, (unsigned char *)&recvAck, sizeof(struct acknowledgement), 0, (struct sockaddr *)&saddr, &size);
            if (recvlen < 0)
            {
                printf("Error receiving. Code [%d].", WSAGetLastError());
                closesocket(sockfd);
                return (-1);
            }
            // Check acknowledgement for correctness
            if (strcmp(recvAck.ack, ACKNOWLEDGEMENT) == 0)
            {
                if (recvAck.seqNr == currentPacket && recvAck.ackChecksum != checksum) // received checksum doesn't match calculated
                {
                    printf("Server received incorrect checksum. Resending...\n\n");
                    packetRetries++;
                    continue;
                }
                else if (recvAck.seqNr == currentPacket) // Received correct acknowledgement; break from current loop to send next packet
                {
                    printf("Received acknowledgement. Sending next packet...\n\n");
                    packetRetries = 0;
                    currentPacket++;
                    if (currentPacket >= i) // EOF (currentPacket == i) reached
                    {
                        break;
                    }

                    continue; // EOF not yet reached; continue cycling through the file
                }
                else // Incorrect acknowledgement; resend last packet
                {
                    printf("Received acknowledgement for unexpected sequence number. Waiting %d seconds for correct acknowledgement...\n", WAITTIME);
                    listen(sockfd, 5);

                    int res = select(sockfd + 1, &fds, NULL, NULL, &timeout);
                    printf("\nReceived answer [%d].\n", res);
                    if (res > 0)
                    {
                        int recvlen = recvfrom(sockfd, (unsigned char *)&recvAck, sizeof(struct acknowledgement), 0, NULL, NULL);

                        if (recvlen < 0)
                        {
                            printf("Error receiving. Error Code: %d\n", WSAGetLastError());
                            closesocket(sockfd);
                            return (-1);
                        }

                        // Check acknowledgement for correctness
                        if (strcmp(recvAck.ack, ACKNOWLEDGEMENT) == 0)
                        {
                            if (recvAck.seqNr == currentPacket)
                            {
                                // Received correct acknowledgement
                                // Break from current loop to send next packet
                                printf("Received correct acknowledgement within %d seconds. Sending next packet...\n", WAITTIME);
                                packetRetries = 0;
                                currentPacket++;
                                if (currentPacket >= i)
                                {
                                    break;
                                }
                                continue;
                            }
                            else
                            {
                                // edge-case
                                // additional wrong acknowledgement within timeframe
                                // will not be handled differently.
                            }
                        }
                    }
                    else
                    {
                        // stays on wrong seqnr
                        FD_SET(sockfd, &fds);
                    }

                    printf("Incorrect acknowledgement (packet [%d]). Resending last packet...\n", recvAck.seqNr);
                    currentPacket = recvAck.seqNr + 1;
                    packetRetries++;
                    continue;
                }
            }
        }
        else if (res == 0 && packetRetries < MAXRETRIES)
        {
            // Timeout; resend
            printf("Timeout: no Acknowledgement received after %d seconds. Resending last packet...\n", WAITTIME);
            FD_SET(sockfd, &fds);

            packetRetries++;
            continue;
        }
        else if (packetRetries == MAXRETRIES)
        {
            // Too many retries; close connection
            printf("Too many failed send attempts. Aborting.");
            break;
        }
        else if (res == -1)
        {
            printf("Error on select(). Error code [%d].\n", WSAGetLastError());
            return (-1); // temp
        }
    }

    // Close the input file and the UDP socket; clean up
    printf("EOF reached. Ending transmission...\n");
    fclose(infile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}
