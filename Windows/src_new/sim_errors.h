#include <stdio.h>

int retries = 0;

int provokeSeqError(int currentPacket, int packetToFalsify)
{

    // Provoke erroneous sequencing on 5th packet
    if (currentPacket == packetToFalsify && retries < 1) // skip packet 5
    {
        currentPacket++;
        retries++;
    }

    return currentPacket;
}

int provokeChecksumError(int currentPacket, int checksum, int packetToFalsify)
{
    // Provoke erroneous checksum on 7th packet
    if (currentPacket == packetToFalsify && retries < 1)
    {
        printf("Forcibly falsifying checksum. (-1)\n");
        checksum--; // falsify checksum
        retries++;
    }

    return checksum;
}