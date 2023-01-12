#define ACKNOWLEDGEMENT "ACK"

struct acknowledgement
{
    char ack = (char)ACKNOWLEDGEMENT; 
    int seqNr; 
};