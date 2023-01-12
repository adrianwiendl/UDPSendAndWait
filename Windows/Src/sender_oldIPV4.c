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
    int port = atoi(argv[2]);
    char* server = argv[3];
    
    //
    char line[BUFFERSIZE];
    char recvbuf[BUFFERSIZE];
    int checksum;
    int sendlen;
    
    //
    struct packet       packetToSend;
    struct sockaddr_in  saddr, caddr;
    struct timeval      timeout;

    //
    char* receivedAcknowledgement;

    //Timeout for waiting on acknowledgement
    timeout.tv_sec = WAITTIME;
    
    //Open socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
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
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr (server);
    saddr.sin_port = htons(port);



    //Print Confirmation for user
    printf("Sending %s (file) to %u (IP) on %u (port)\n", input_file, saddr.sin_addr.s_addr, saddr.sin_port);

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



    //Read and send the file line by line
    while (fgets(line, BUFFERSIZE, infile)) 
    {
        /*Intended functionality outline:
        * 1. Send 1st packet
        * 2. Await acknowledgement (WAITTIME Seconds)
        * 3.1 On positive acknowledgement: reset timer and send 2nd packet and so on
        * 3.2 On negative/no acknowledgement: Resend 1st packet, as long as timer has not expired, else abort.
        * */
       

        
        //Calculate the checksum for the line
        long checksum = generateChecksum(line, strlen(line));
        //Print line + checksum being sent
        printf("Sending: \"%s\" w/ Checksum: %ld\n",line, checksum);
        //printf("Sending: %s \n",line);
        //Temp solution: checksum is always 5
        //checksum = 5;
        
        //Prepare packet
        strcpy(packetToSend.textData, line);
        packetToSend.seqNr = currentPacket;
        packetToSend.checksum = checksum;


 
        //char sendPacketSerialized[BUFFERSIZE];
        
        //Zero serialized packet for each new send operation
        //bzero(sendPacketSerialized, sizeof(sendPacketSerialized));
        
        //Serialize packet - UNNECCESSARY
        // strcat(sendPacketSerialized,packetToSend.textData);
        // strcat(sendPacketSerialized,itoa(packetToSend.seqNr,sendPacketBuffer,10));
        // strcat(sendPacketSerialized,itoa(packetToSend.checksum,sendPacketBuffer,10));
        //sendlen = strlen(sendPacketSerialized);
        //printf("packet  :%s\n",sendPacketSerialized);

        sendlen = sizeof(struct packet);
        //Run loop to send
        while (TRUE)
        {
            //if (send(sockfd, sendPacketSerialized, sendlen, 0) != sendlen)
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

            char* receivedAcknowledgement = "";
            
            if (res == 1 && packetRetries < MAXRETRIES)
            {
                //Answer received. 
                int recvlen = recv(sockfd, receivedAcknowledgement, sizeof(receivedAcknowledgement), 0);
                if(recvlen < 0)
                {
                    printf("Error receiving. Error Code: %d",WSAGetLastError());
                    closesocket(sockfd);
                    return (-1);
                }
                receivedAcknowledgement[recvlen]=0;
                //printf("Received acknowledgement \'%s\' from Server.\n",receivedAcknowledgement);
                
                //Check acknowledgement for correctness
                if(strcmp(receivedAcknowledgement,ACKNOWLEDGEMENT) == 0)
                {
                    //Received correct acknowledgement
                    //Break from current loop to send next packet
                    puts("----------Received acknowledgement. Sending next packet.----------");
                    packetRetries = 0;
                    currentPacket++;
                    break;
                }
                else
                {
                    //Incorrect acknowledgement
                    //resend last packet
                    puts("----------Incorrect acknowledgement. Resending last packet.----------");
                    packetRetries++;
                    continue;
                }
            }
            else if (res == 0 && packetRetries < MAXRETRIES)
            {
                //Timeout
                //Resend
                printf("----------No Acknowledgement received after %d seconds. Resending last packet.----------\n",WAITTIME);
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
            }
        }
    }

    //Close the input file and the UDP socket
    //Cleanup
    printf("File sent. Shutting down connection.\n");
    fclose(infile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}
