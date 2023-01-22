#include "defines.h"

struct packet
{
    char textData[BUFFERSIZE];
    int seqNr;
    long checksum;
};

struct acknowledgement
{
    char ack[BUFFERSIZE];
    int seqNr;
    int ackChecksum;
};