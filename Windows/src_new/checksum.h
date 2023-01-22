/* Compute Internet Checksum for "count" bytes
 *  beginning at location "addr".
 *  Taken from https://www.rfc-editor.org/rfc/rfc1071#section-4
 *  as linked in the task.
 */

#define BUFFERSIZE 1024

unsigned short generateChecksum(char addr[BUFFERSIZE], int count);
