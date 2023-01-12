#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "packetStruct.h"
#include <time.h>
#include "checksum.h"
#include "ackStruct.h"
#include "lineStruct.h"
#include <malloc.h>

#define WAITTIME 5 //seconds to wait for acknowledgement
#define MAXRETRIES 6



int main (int argc, char* argv[])
{
    //Check correct program call
    if (argc != 4) 
    {
        fprintf(stderr, "Usage: %s <input file> <port> <IPv6 address>\n", argv[0]);
        exit(1);
    }
    
    WSADATA wsaData;
    //Initialize the socket API with the version of the
    //Winsock specification that we want to use
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //Error initializing Windows Socket API
        printf("Error initializing the socket API. Error Code: %d",WSAGetLastError());
        return (-1);
    }

    //Variables for sequencing
    int currentPacket       = 0; 
    int totalPacketCount    = 0;
    int packetRetries       = 0;

    //
    int sockfd;
    char sendPacketBuffer[BUFFERSIZE];
    

    //Parse passed command-line arguemnts
    char* input_file = argv[1];
    unsigned short port = atoi(argv[2]);
    char* server = argv[3];

    //
    char line[BUFFERSIZE];
    char recvbuf[BUFFERSIZE];
    int checksum;
    int sendlen;
    
    //
    struct packet        packetToSend;
    struct sockaddr_in6  saddr, caddr;
    struct timeval       timeout;
    struct acknowledgement recvAck;
    struct line         s_line;
    //
    char* receivedAcknowledgement;

    struct line lines[BUFFERSIZE];

    //Timeout for waiting on acknowledgement
    timeout.tv_sec = WAITTIME;
    
    //Open socket
    if ((sockfd = socket (AF_INET6, SOCK_STREAM, 0)) < 0)
    {
        //Error opening socket
        printf("Error opening socket. Error Code: %d",WSAGetLastError());
        WSACleanup();
        return (-1);
    }
    
    //
    struct fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    //Parse IP-Address and port
    bzero ((char* ) &saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, server,&saddr.sin6_addr);
    saddr.sin6_port = htons(port);


    //Print Confirmation for user
    char buf[BUFFERSIZE];
    printf("Sending %s (file) to %s (IP) on %d (port)\n", input_file, inet_ntop(AF_INET6,&saddr.sin6_addr,buf,sizeof(buf)), ntohs(saddr.sin6_port));

    //Open the input file for reading
    FILE *infile = fopen(input_file, "r");
    if (infile == NULL) 
    {
        //Error opening input file
        perror("Error opening input file.");
        closesocket(sockfd);
        WSACleanup();
        return (-1);
    }

    //Connect socket
    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(saddr) ) < 0)
    {
        //Error connecting socket
        printf("Error connecting socket: Error code %d\n", WSAGetLastError());
        closesocket(sockfd);
        fclose(infile);
        WSACleanup();
        return (-1);
    }
    else
    {
        printf("Successfully connected to server.\n");
    }
    
    char linez[100][512];

    int i = 0;

    while (fgets(linez[i], 512, infile)) 
    {
        i++;
    }


    //Read and send the file line by line
    //while (fgets(line, BUFFERSIZE, infile)) 
    //{
        /*Intended functionality outline:
        * 1. Send 1st packet
        * 2. Await acknowledgement (WAITTIME Seconds)
        * 3.1 On positive acknowledgement: reset timer and send 2nd packet and so on
        * 3.2 On negative/no acknowledgement: Resend 1st packet, as long as timer has not expired, else abort.
        * */


        
        int tempRetries = 0;
        int erroneousChecksumRetries = 0;
        //Run loop to send
        while(TRUE)
        {
            //Provoke erroneous sequencing on 5th packet
            // if(currentPacket == 5 && tempRetries < 1)
            // {
            //     currentPacket++;
            //     tempRetries++;
            //     continue;
            // }
            //Calculate the checksum for the line
            //long checksum = generateChecksum(line, strlen(line));
            //puts("uwuw");
            printf("___________Current Packet: %d. Content: %s",currentPacket,linez[currentPacket]);
            long checksum = generateChecksum(linez[currentPacket], strlen(linez[currentPacket]));
            
            //Provoke erroneous checksum on 7th packet
            // if(currentPacket == 7 && erroneousChecksumRetries < 1)
            // {
            //     printf("Forcibly falsifying checksum cause ewil uwu\n");
            //     if(checksum != 69420)
            //     {
            //         checksum = 69420;
            //     }
            //     else
            //     {
            //         checksum++;
            //     }
            //     erroneousChecksumRetries++;
            // }



            //Print line + checksum being sent
            printf("Sending: \"%s\" w/ Checksum: %ld\n",linez[currentPacket], checksum);

            //Prepare packet
            printf("----Current Packet is %d-----\n",currentPacket);
            strcpy(packetToSend.textData, linez[currentPacket]);
            //packetToSend.seqNr = lines[currentPacket].lineNo;
            packetToSend.seqNr = currentPacket;
            packetToSend.checksum = checksum;
            
            sendlen = sizeof(struct packet);

            if (send(sockfd, (unsigned char* ) &packetToSend, sendlen, 0) != sendlen)
            {
                //Error on sending
                printf("Error sending. Error Code: %d",WSAGetLastError());
                fclose(infile);
                closesocket(sockfd);
                WSACleanup();
                return (-1);
            }
            
            //Set socket to non-blocking mode
            u_long iMode = 1;
            ioctlsocket(sockfd, FIONBIO, &iMode);
            
            //Listen on socket for acknowledgement for WAITTIME seconds
            listen (sockfd, 5);
            int res = select(sockfd + 1, &fds, NULL, NULL, &timeout);
            printf("Res received = %d\n", res);


            
            if (res == 1 && packetRetries < MAXRETRIES)
            {
                //Answer received. 
                
                int recvlen = recv(sockfd, (unsigned char* )&recvAck, sizeof(struct acknowledgement), 0);
                //int recvlen = recv(sockfd, (unsigned char* )&s_ack, sizeof(s_ack), 0);
                if(recvlen < 0)
                {
                    //Don't remove! We don't know why, but without it everything breaks...
                    //printf("recvlen: %d",recvlen);
                    printf("Error receiving. Error Code: %d",WSAGetLastError());
                    closesocket(sockfd);
                    return (-1);
                }
                //Check acknowledgement for correctness
                if(strcmp(recvAck.ack,ACKNOWLEDGEMENT) == 0)
                {
                    if(recvAck.ackChecksum != checksum)
                    {
                        printf("Incorrect checksum received on Server. Resending packet...\n");
                        packetRetries++;
                        continue;
                    }
                    else if(recvAck.seqNr == currentPacket)
                    {
                        //Received correct acknowledgement
                        //Break from current loop to send next packet
                        printf("----------Received acknowledgement. Sending next packet.----------\n");
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
                        //Incorrect acknowledgement
                        //TEST
                        //resend last packet
                        printf("-----------Incorrect seqnr ack recv'd. Waiting %d second for correct seqnr.---------\n",WAITTIME);
                        listen (sockfd, 5);
                        
                        int res = select(sockfd + 1, &fds, NULL, NULL, &timeout);
                        printf(".....res:%d\n",res);
                        if (res > 0)
                        {
                            //
                            //check new ack seq nr
                            //if correct continue
                            int recvlen = recv(sockfd, (unsigned char* )&recvAck, sizeof(struct acknowledgement), 0);
                            
                            if(recvlen < 0)
                            {
                                //Don't remove! We don't know why, but without it everything breaks...
                                //printf("recvlen: %d",recvlen);
                                printf("Error receiving. Error Code: %d\n",WSAGetLastError());
                                closesocket(sockfd);
                                return (-1);
                            }
                            
                            //Check acknowledgement for correctness
                            if(strcmp(recvAck.ack,ACKNOWLEDGEMENT) == 0)
                            {
                                if(recvAck.seqNr == currentPacket)
                                {
                                    //Received correct acknowledgement
                                    //Break from current loop to send next packet
                                    printf("----------Received correct acknowledgement within %d seconds. Sending next packet.----------\n",WAITTIME);
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
                                    //edge-case
                                    //additional wrong acknowledgement within timeframe
                                    //will not be handled differently.
                                }
                            }
                        }
                        else
                        {
                            //stays on wrong seqnr
                            FD_SET(sockfd, &fds);

                        }

                        printf("----------Incorrect acknowledgement (packet %d). Resending last packet.----------\n",recvAck.seqNr);
                        currentPacket = recvAck.seqNr+1;
                        packetRetries++;
                        continue;
                    }
                }

            }
            else if (res == 0 && packetRetries < MAXRETRIES)
            {
                //Timeout
                //Resend
                printf("----------No Acknowledgement received after %d seconds. Resending last packet.----------\n",WAITTIME);
                FD_SET(sockfd, &fds);

                packetRetries++;
                continue;
            }
            else if (packetRetries == MAXRETRIES)
            {
                //Too many retries
                //Close connection.
                printf("Too many failed send attempts. Aborting.");
                break;
            }
            else if(res == -1)
            {
                printf("Error on select(). Error code: %d\n",WSAGetLastError());
                
                
                //TEMP
                return(-1);
            }
        }
    //}

    //Close the input file and the UDP socket
    //Cleanup
    printf("File sent. Shutting down connection.\n");
    fclose(infile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}




