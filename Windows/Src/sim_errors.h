#include <stdio.h>
#include <windows.h>
#define WAITTIME 5 // Seconds to wait for acknowledgement

int seqRetries = 1;
int csmRetries = 1;
int ackRetries = 1;

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
    if (seqNr == seqNrToSkip && ackRetries < 1)
    {
        printf("Forcing missing acknowledgement on packet with seq-nr [%d].\n\n", seqNrToSkip);
        ackRetries++;

        return 0;
    }
    return 1;

}