#include <stdio.h>
#include <windows.h>
#include <string.h>

// int seqRetries = 1;
// int csmRetries = 1;
// int ackRetries = 1;
int MissingAckPack =-1;
int SeqErrorPack = -1;
int CsmErrorPack = -1;

int menuSender(char* s1, char* s2)//SENDER function to trigger errors
{
    //a= ack, c= checksum, s=sequenz

    //combine arguments to one string

puts(s1);
puts(s2);
    char errorstr[30];
    sprintf(errorstr, "%s=%s", s1, s2);

printf("TEST OUTPUT errorstr: %s\n", errorstr);

    //split string into 4 array entries
    int j = 0;
    char *partS = strtok (errorstr, "=, -");
    char *arrayS[4] = {NULL, NULL, NULL, NULL};

    while (partS != NULL)
    {
        arrayS[j++] = partS;
        partS = strtok (NULL, "=, -");
    }

    //
    if(atoi(arrayS[1]) != 0) //test for integer
    {
        char *argument1;
        argument1 = arrayS[0];

        switch(*argument1)
        {
        case 'c':
            csmRetries = 0;
            CsmErrorPack = atoi(arrayS[1]);
    puts("casec");
            break;
        case 's':
            seqRetries = 0;
            SeqErrorPack = atoi(arrayS[1]);
    puts("cases");
            break;
        case 'l':
    puts("casel1");
            break;
        default:
    puts("def");
            printf("Error wrong argument. -%s is no known argument\n", arrayS[0]); //TODO help message
            return -1;
        }
    }

    if(atoi(arrayS[3]) != 0) //test for integer
    {
        char *argument2;
        argument2 = arrayS[2];

        switch(*argument2)
        {
        case 'a':
            ackRetries = 0;
    puts("casea2");
            break;
        case 'c':
            csmRetries = 0;
            CsmErrorPack = atoi(arrayS[3]);
    puts("casec2");
            break;
        case 's':
            seqRetries = 0;
            SeqErrorPack = atoi(arrayS[3]);
    puts("cases2");
            break;
        case 'l':
    puts("casel2");
            break;
        default:
    puts("def2");
            printf("Error wrong argument. -%s is no known argument\n", arrayS[2]); //TODO help message
            return -1;
        }
    }

    return 0;
}

int menuReceiver(char* r1)//Receiver Function to Trigger errors
{
    int l = 0;
    char *partR = strtok (r1, "=, -");
    char *arrayR[2] = {NULL, NULL};

    while (partR != NULL)
    {
        arrayR[l++] = partR;
        partR = strtok (NULL, "=, -");
    }

    if(arrayR[0] = 'a' && atoi(arrayR[1]) != 0)
    {
        ackRetries = 0;
        MissingAckPack = atoi(arrayR[1]);
    }
    return 0;
}




