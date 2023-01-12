#define BUFFERSIZE 1024
struct acknowledgement
{
    char ack[BUFFERSIZE]; 
    int seqNr; 
    int ackChecksum;
};