#include <stdio.h>
#include <windows.h>
#define WAITTIME 5 // Seconds to wait for acknowledgement

int seqRetries = 0;
int csmRetries = 0;
int ackRetries = 0;

int provokeSeqError(int currentPacket, int packetToSkip) // SENDER
{

    // Provoke erroneous sequencing on 5th packet
    if (currentPacket == packetToSkip && seqRetries < 1) // skip packet packetToSkip
    {
        printf("Forcibly skipping packet [%d].\n\n", packetToSkip);
        seqRetries++;
        currentPacket++;
    }

    return currentPacket;
}

int provokeChecksumError(int currentPacket, int checksum, int packetToFalsify) // SENDER
{
    // Provoke erroneous checksum on 7th packet
    if (currentPacket == packetToFalsify && csmRetries < 1)
    {
        printf("Forcibly falsifying checksum. (Checksum -= 1)\n\n");
        checksum--; // falsify checksum by changing value
        csmRetries++;
    }

    return checksum;
}

int provokeMissingAck(int seqNr, int seqNrToSkip) // RECEIVER
{
    time_t start, end;
    if (seqNr == seqNrToSkip && ackRetries < 1)
    {
        printf("Forcing missing acknowledgement on packet with seq-nr [%d].\n\n", seqNrToSkip);
        ackRetries++;
        //Sleep(6000); // longer than TIMEOUT (5s)
        /* wait 5 seconds */
        time(&start);
        do
            time(&end);
        while (difftime(end, start) <= 5);
        
        return 0;
        //return seqNr++;
    }
    return 1;
    //return seqNr;
}