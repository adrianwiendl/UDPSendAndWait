#define BUFFERSIZE 1024
#define ACKNOWLEDGEMENT "ACK"

// Windows implementation of bzero() function. Thanks to Romain Hippeau on Stackoverflow!
// https://stackoverflow.com/a/3492670
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)

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

struct line
{
    char *data;
    int lineNo;
};