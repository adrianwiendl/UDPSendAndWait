#include <stdio.h>
#include <windows.h>
#include <string.h>

//#include "sim_errors.h"

int MissingAckPack =-1;
int SeqErrorPack = -1;
int CsmErrorPack = -1;

int menuSender(char* s1, char* s2)//SENDER function to trigger errors
{
    //combine arguments to one string
    char errorstr[30];
    sprintf(errorstr, "%s=%s", s1, s2);

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
            break;
        case 's':
            seqRetries = 0;
            SeqErrorPack = atoi(arrayS[1]);
            break;
        default:
            printf("Wrong program call. -%s is no known argument.\n", arrayS[0]); //Wrong program call: output help and exit
            printf("Use -c=[Packetnumber] to Forcibly falsify checksum\n");
            printf("Use -s=[Packetnumber] to Forcibly skip packet\n");
            exit(1);
        }
    }
    else
    {
        printf("Wrong Program call use integer value > 0 as Packetnumber \n");
        exit(1);
    }

    if(atoi(arrayS[3]) != 0) //test for integer
    {
        char *argument2;
        argument2 = arrayS[2];

        switch(*argument2)
        {
        case 'c':
            csmRetries = 0;
            CsmErrorPack = atoi(arrayS[3]);
            break;
        case 's':
            seqRetries = 0;
            SeqErrorPack = atoi(arrayS[3]);
            break;
        case 'l':
            break;
        default:
            printf("Wrong program call. -%s is no known argument.\n", arrayS[0]); //Wrong program call: output help and exit
            printf("Use -c=[Packetnumber] to Forcibly falsify checksum\n");
            printf("Use -s=[Packetnumber] to Forcibly skip packet\n");
            exit(1);
        }
    }
    else
    {
        printf("Wrong Program call use integer value > 0 as Packetnumber \n");
    }

    return 0;
}

int menuReceiver(char* r1)//Receiver Function to Trigger errors
{
    int successR = 0;
    int l = 0;
    char *partR = strtok (r1, "=, -");
    char *arrayR[2] = {NULL, NULL};

    while (partR != NULL)
    {
        arrayR[l++] = partR;
        partR = strtok (NULL, "=, -");
    }
    
    char *argrec;
    argrec = arrayR[0];

    if(*argrec == 'a')
    {
        if(arrayR[1] != NULL)
        {
            if(atoi(arrayR[1]) != 0)
            {
                ackRetries = 0;
                MissingAckPack = atoi(arrayR[1]);
                successR++;
            }
        }
    }
    
    if(successR == 0)
    {
        printf("Wrong program call. Use -a=[Packetnumber] to trigger a Missing Acknowledgement\n");
        printf("[Packetnumber] has to be an Integer value > 0\n");
        exit(1);
    }
    return 0;
}




