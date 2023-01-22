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
#include "defines.h"

/*
Copied from Andreas Bonini on Stackoverflow:
https://stackoverflow.com/a/1921557 
*/
typedef int bool;
#define TRUE 1
#define FALSE 0

int currentPacket = 0;
int totalPacketCount = 0;
int packetRetries = 0;
int totalLines = 0;
long checksum = 0;

struct timeval timeout;
struct fd_set fds;

int main(int argc, char *argv[])
{
    // Check parameter count
    if (argc < 4 || argc > 6) // Incorrect program call
    {
        fprintf(stderr, "Usage: %s <input file> <port> <IPv6 address> <error=[packetnumber]> <error=[packetnumber]>\n", argv[0]);
        exit(1);
    }

    if (argc > 4) // Program call with error arguments
    {
        char *e1 = argv[4];
        char *e2 = argv[5];

        if(argc < 6)
        {
          e2 = "-l=1";
        }

        menuSender(e1, e2);

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
        // Error initializing Windows Socket API
        printf("Error initializing the socket API. Error Code: %d", WSAGetLastError());
        return (-1);
    }
    //
    int sockfd;
    char sendPacketBuffer[BUFFERSIZE];

    // Parse passed command-line arguemnts
    char *input_file = argv[1];
    unsigned short port = atoi(argv[2]);
    char *server = argv[3];

    //
    char recvbuf[BUFFERSIZE];

    //
    struct packet packetToSend;
    struct sockaddr_in6 saddr, caddr;
    // struct timeval timeout;
    struct acknowledgement recvAck;
    //
    char *receivedAcknowledgement;

    // Timeout for waiting on acknowledgement
    timeout.tv_sec = WAITTIME;

    // Output
    char packetData[BUFFERSIZE];


    bool continueSending = TRUE;
    bool resentCurrentPacket = FALSE;
    // Open socket
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        // Error opening socket
        printf("Error opening socket. Error Code: [%d]", WSAGetLastError());
        WSACleanup();
        return (-1);
    }

    //
    // struct fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    // Parse IP-Address and port
    bzero((char *)&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, server, &saddr.sin6_addr);
    saddr.sin6_port = htons(port);

    // Print Confirmation for user
    char buf[BUFFERSIZE];


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

    // Cache input file into lines-array for easier access 
    int i = 0;
    while (fgets(lines[i], 512, infile))
    {
        i++;
    } // Read file line by line and store each line in the array
    totalLines = i;
    /* Intended functionality outline:
     * 1.   Send 1st packet
     * 2.   Wait for acknowledgement (WAITTIME seconds)
     * 3.1  On positive acknowledgement:     reset timer and send 2nd packet and so on
     * 3.2  On negative/no acknowledgement:  resend 1st packet, as long as timer has not expired, else abort.
     * */
    //Print Confirmation
    printf("Sending file [%s] to IP [%s] on PORT [%d].\n", input_file, inet_ntop(AF_INET6, &saddr.sin6_addr, buf, sizeof(buf)), ntohs(saddr.sin6_port));
    

    // Run loop to send
    while (continueSending)
    {
        printf("===============Packet [%d]===============\n",totalPacketCount);
        
        // Provoke erroneous sequencing on chosen packet
        currentPacket = provokeSeqError(currentPacket, SeqErrorPack); // (current, packet to falsify)
        
        // Remove trailing newline character for better diagnosis output
        strcpy(packetData, lines[currentPacket]);
        packetData[strcspn(packetData, "\n")] = 0;
        
        //Build packet and send to Server
        if(sendToServer(sockfd, &lines[currentPacket], currentPacket, saddr) != 0)
        {
            //Error while sending to Server. Exits the program with error.
            printf("Error sending packet [%d]. Error Code: [%d].", currentPacket, WSAGetLastError());
            fclose(infile);
            closesocket(sockfd);
            WSACleanup();
            return (-1);       
        }
        else
        {
            // Packet successfully sent to server
            // Print line + checksum sent
            // Increase total amount of packets sent by 1
            printf("Sent sequence-no. [%d] with data [\"%s\"] and checksum [%ld] in packet [%d].\n", 
                                                currentPacket, 
                                                packetData, 
                                                checksum, 
                                                totalPacketCount);
            totalPacketCount++;
        }

        // Set socket to non-blocking mode
        u_long iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);

        //Make sure the maximum number of retries for each packet (defined in MAXRETRIES) is not surpassed
        if (packetRetries < MAXRETRIES)
        {
            switch (receiveFromServer(sockfd,saddr))
            {
            // case 1:
            //     //Resend current packet because server couldn't verify checksum
            //     printf("Server received incorrect checksum. Resending...\n\n");
            //     packetRetries++;
            //     break;
            case RETURN_EOF:
                //EOF has been reached. End transmission
                continueSending = FALSE;
                break;
            case RETURN_OK:
                //Server received and acknowledged correct packet. 
                //Continue sending file at next line
                printf("Received acknowledgement. Sending next packet...\n\n");
                packetRetries = 0;
                currentPacket++;
                resentCurrentPacket = FALSE;
                break;
            case RETURN_MISSINGACK:
                //Server did not acknowledge correct packet in time.
                //Resending down from current packet until correct packet is sent
                if(resentCurrentPacket == FALSE)
                {
                    resentCurrentPacket = TRUE;
                    packetRetries++;
                }
                else
                {
                    currentPacket--;
                    if(currentPacket < 0)
                    {
                        //Make sure we don't send undefined data
                        currentPacket = 0;
                    }
                    packetRetries++;
                }
                break;

            default:
                //Error receiving
                printf("Error receiving from Server. Aborting.\n");
                fclose(infile);
                closesocket(sockfd);
                WSACleanup();
                return (-1);                
            }
        }
        else // too many send attempts failed. Aborting current Transmission. (packetRetries >= MAXRETRIES)
        {
            // Too many retries on one packet. Exits the program with error.
            printf("Too many failed send attempts. Aborting.\n");
            fclose(infile);
            closesocket(sockfd);
            WSACleanup();
            return (-1);
        }
    }

    /*
    End of successful transmission.
    Show total lines sent as well as amount of packets required to do so.
    Close input file and the UDP socket and clean up WSA.
    */
    printf("---------------------------------------\n");
    printf("EOF reached.\n");
    printf("Sent [%d] lines of text in [%d] packets.\n",totalLines,totalPacketCount);
    printf("Ending transmission...\n");
    printf("Shutting down. Goodbye...\n");
    fclose(infile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}


//saw_send-Function
int sendToServer(int sockfd, char* textToSend, int packetNumber, struct sockaddr_in6 dataOfServer)
{
    struct packet packetToSend;
    checksum = provokeChecksumError(packetNumber, generateChecksum(textToSend, strlen(textToSend)), CsmErrorPack); //checksum error on chosen packet
    //Fill packet to be sent to Server
    strcpy(packetToSend.textData,textToSend);
    packetToSend.checksum = checksum;
    packetToSend.seqNr = packetNumber;

    int sendlen = sizeof(packetToSend);

    if (sendto(sockfd, (unsigned char *)&packetToSend, sendlen, 0, (struct sockaddr *)&dataOfServer, sizeof(dataOfServer)) != sendlen) 
        {
            // Error on sending
            printf("Error sending packet [%d]. Error Code: [%d].\n", packetNumber, WSAGetLastError());
            return (-1);
        } 
    return 0;
}

//saw_receive-Function
int receiveFromServer(int sockfd, struct sockaddr_in6 dataOfServer)
{
    //Struct to store the received acknowledgement
    struct acknowledgement acknowledgementReceived;
    int res;
    do
    {
        // Answer received.
        //Listen for incoming requests on socket for WAITTIME seconds
        listen(sockfd, 5);
        res = select(sockfd + 1, &fds, NULL, NULL, &timeout);
        printf("Incoming requests: [%d].\n", res);
        
        if (res > 0)
        {
            //Answer(s) waiting to be received 
            int size = sizeof(dataOfServer);
            puts("Receiving...");
            int recvlen = recvfrom(sockfd, (unsigned char *)&acknowledgementReceived, sizeof(struct acknowledgement), 0, (struct sockaddr *)&dataOfServer, &size);
            if (recvlen < 0)
            {
                //Error receiving answer from Server.
                //return error and exit program in main.
                printf("Error receiving. Code [%d].", WSAGetLastError());
                return (-1);
            }
            // Check received acknowledgement for correctness, act accordingly on mismatches
            if (strcmp(acknowledgementReceived.ack, ACKNOWLEDGEMENT) == 0)
            {
                if (acknowledgementReceived.ackChecksum != checksum) 
                {
                    // received checksum doesn't match server-calculated
                    //return RETURN_MISSINGACK;
                    continue;
                }
                else if (acknowledgementReceived.seqNr != currentPacket)
                {
                    // Incorrect acknowledgement; resend last packet
                    //currentPacket = acknowledgementReceived.seqNr+1;
                    //Server acknowledged incorrect sequence number.
                    //Wait if correct acknowledgement will arrive within WAITTIME seconds, else resend expected packet.
                    printf("Received acknowledgement for unexpected sequence number (expected [%d]).\nWaiting %d seconds for correct acknowledgement...\n\n", currentPacket,WAITTIME);
                    //continue;
                    continue;
                }
                else  
                {
                    // Received correct acknowledgement (acknowledgementReceived.seqNr == currentPacket); proceed to send next packet or end transmission if EOF reached
                    if (currentPacket+1 >= totalLines) // check if EOF (currentPacket == i) reached
                    {
                        // EOF reached. Program will exit properly.
                        return RETURN_EOF;
                    }
                    // EOF not yet reached; continue cycling through the file
                    return RETURN_OK; 
                }
            }
        }
    }while (res > 0);
    if (res == 0)
    {
        //No incoming packet after WAITTIME --> no acknowledgement --> resend last packet
        FD_SET(sockfd, &fds);
        printf("Did not receive matching acknowledgement in time. Resending last packet ([%d])...\n\n", currentPacket);
        return RETURN_MISSINGACK;        
    }
    else
    {
        // Error
        return (-1);
    }
    
    return (-1);
}