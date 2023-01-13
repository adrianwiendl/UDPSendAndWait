#include <stdio.h>
#include <windows.h>

int retries = 0;

int provokeSeqError(int currentPacket, int packetToSkip)
{

    // Provoke erroneous sequencing on 5th packet
    if (currentPacket == packetToSkip && retries < 1) // skip packet 5
    {
        printf("Forcibly skipping packet %d.\n", packetToSkip);
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
int provokeMissingAck(int seqNr, int seqNrToSkip)
{
    time_t start, end;
    if (seqNr == seqNrToSkip && retries < 1)
    {
        printf("Forcing missing acknowledgement on packet %d.\n", seqNrToSkip);
        retries++;
        Sleep(6000); // longer than TIMEOUT (5s)
        /* wait 2.5 seconds */
        time(&start);
        do
            time(&end);
        while (difftime(end, start) <= 5);
    }

    return seqNr++;
}