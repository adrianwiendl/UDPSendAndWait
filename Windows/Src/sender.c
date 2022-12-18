#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define WAITTIME 5 //seconds to wait for acknowledgement
#define BUFFERSIZE 1024
//Windows implementation of bzero() function. Thanks to Romain Hippeau on Stackoverflow!
//https://stackoverflow.com/a/3492670
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)


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

    //
    int CurrentPacket = 0; 
    int sockfd;
    int sendlen;
    char sendbuf[BUFFERSIZE];
    struct sockaddr_in saddr;
    char* server = argv[3];
    int port = atoi(argv[2]);
    char* input_file = argv[1];

    //Open socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        //Error opening socket
        printf("Error opening socket. Error Code: %d",WSAGetLastError());
        WSACleanup();
        return (-1);
    }

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

    //Read and send the file line by line
    char line[BUFFERSIZE];
    while (fgets(line, BUFFERSIZE, infile)) 
    {
        //Calculate the checksum for the line
        //int checksum = generateChecksum();
        //Print line + checksum being sent
        //printf("Sending: \"%s\" w/ Checksum: %d\n",line, checksum);
        printf("Sending: %s \n",line);
   
        //TODO: Concatenate the line and the checksum and send the packet
        unsigned char packet[BUFFERSIZE];
        int sendlen = strlen(line);

        //Send line, length
        if (send (sockfd, line, strlen(line), 0) != sendlen)
        {
            //Error on sending
            printf("Error sending. Error Code: %d",WSAGetLastError());
            fclose(infile);
            closesocket(sockfd);
            WSACleanup();
            return (-1);
        }
    }

    //Close the input file and the UDP socket
    printf("File sent. Shutting down connection.\n");
    fclose(infile);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}

// int generateChecksum()
// {
//     /* Compute Internet Checksum for "count" bytes
//     *  beginning at location "addr".
//     *  Taken from https://www.rfc-editor.org/rfc/rfc1071#section-4
//     *  as linked in the task.
//     */
//     int count = 0/*TODO*/;
//     int checksum;

//     register long sum = 0;
//     while( count > 1 )  {
//         /*  This is the inner loop */
//             sum += * (unsigned short) addr++;
//             count -= 2;
//     }

//         /*  Add left-over byte, if any */
//     if( count > 0 )
//             sum += * (unsigned char *) addr;

//         /*  Fold 32-bit sum to 16 bits */
//     while (sum>>16)
//         sum = (sum & 0xffff) + (sum >> 16);

//     checksum = ~sum;

//     return checksum;
// }
