#ifndef DEFINES_H
#define DEFINES_H

#define BUFFERSIZE 1024


// RETURN VALUES
#define RETURN_OK 0
#define RETURN_ERROR -1
#define RETURN_NOANSWER 50
#define RETURN_TIMEOUT 51
#define RETURN_EOF 52
#define RETURN_INCORRECTCHECKSUM 53
#define RETURN_INCORRECTSEQUENCE 54
#define RETURN_SOCKALREADYDEFINED 55
#define RETURN_MISSINGACK 56

// ACKNOWLEDGEMENT
#define ACK_SUCCESS "ACK"

// TEXT
#define MAXLINES 100
#define MAXCOLS 512

// CONNECTION
#define MAXCLIENTS 1
#define WAITTIME 5 //Time to wait for acknowledgement before timing out
#define MAXRETRIES 6

// Windows implementation of bzero() function. Thanks to Romain Hippeau on Stackoverflow!
// https://stackoverflow.com/a/3492670
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)


#endif