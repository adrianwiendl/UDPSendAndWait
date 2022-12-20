    /* Compute Internet Checksum for "count" bytes
    *  beginning at location "addr".
    *  Taken from https://www.rfc-editor.org/rfc/rfc1071#section-4
    *  as linked in the task.
    */
#define BUFFERSIZE 1024
unsigned short generateChecksum(char addr[BUFFERSIZE], int count)
{
    register unsigned short checksum;

    register long sum = 0;
    while( count > 1 )  {
        /*  This is the inner loop */
            sum += *(unsigned short *) addr++;
            count -= 2;
    }
        /*  Add left-over byte, if any */
    if( count > 0 )
            sum += * (unsigned char *) addr;

        /*  Fold 32-bit sum to 16 bits */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);

    checksum = ~sum;

    return checksum;
}
