#include <stdio.h>
#include <windows.h>
#include <string.h>

int MissingAckPack =-1;
int SeqErrorPack = -1;
int CsmErrorPack = -1;

int menuSender(char* s1, char* s2)//Client function to trigger errors
{
    //combine arguments to one string
    char errorstr[30];
    sprintf(errorstr, "%s=%s", s1, s2);

    //split string into array entries
    /*Code copied from rlib on Stackoverflow:
    https://stackoverflow.com/a/15472429*/
    int j = 0;
    char *partC = strtok (errorstr, "=, -");
    char *arrayC[4] = {NULL, NULL, NULL, NULL};

    while (partC != NULL)
    {
        arrayC[j++] = partC;
        partC = strtok (NULL, "=, -");
    }

    if(atoi(arrayC[1]) != 0) //test for integer/Sequenznumber
    {
        char argument1[BUFFERSIZE];         
        strcpy(argument1,arrayC[0]);

        switch(*argument1)
        {
        //Trigger Checksumerror
        case 'c':
            csmRetries = 0;
            CsmErrorPack = atoi(arrayC[1]);
            break;
        //Trigger Sequenzerror
        case 's':
            seqRetries = 0;
            SeqErrorPack = atoi(arrayC[1]);
            break;
        default:
            printf("Wrong program call. -%s is no known argument.\n", arrayC[0]); //Wrong program call: output help and exit
            printf("Use -c=[Sequenznumber] to Forcibly falsify checksum\n");
            printf("Use -s=[Sequenznumber] to Forcibly skip packet\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Wrong Program call use integer value > 0 as Sequenznumber \n");
        exit(EXIT_FAILURE);
    }

    if(atoi(arrayC[3]) != 0) //test for integer/Sequenznumber
    {
        char argument2[BUFFERSIZE];         
        strcpy(argument2,arrayC[2]);

        switch(*argument2)
        {
        //Trigger Checksumerror
        case 'c':
            csmRetries = 0;
            CsmErrorPack = atoi(arrayC[3]);
            break;
        //Trigger Sequenzerror
        case 's':
            seqRetries = 0;
            SeqErrorPack = atoi(arrayC[3]);
            break;
        //Do nothing when no second argument was given
        case 'l':
            break;
        default:
            printf("Wrong program call. -%s is no known argument.\n", arrayC[0]); //Wrong program call: output help and exit
            printf("Use -c=[Sequenznumber] to Forcibly falsify checksum\n");
            printf("Use -s=[Sequenznumber] to Forcibly skip packet\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Wrong Program call use integer value > 0 as Sequenznumber \n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int menuReceiver(char* r1)//Server Function to Trigger errors
{
    int successS = 0;

    //split string into array entries
    /*Code copied from rlib on Stackoverflow:
    https://stackoverflow.com/a/15472429*/
    int l = 0;
    char *partS = strtok (r1, "=, -");
    char *arrayS[2] = {NULL, NULL};

    while (partS != NULL)
    {
        arrayS[l++] = partS;
        partS = strtok (NULL, "=, -");
    }
    
    char argumentServer[BUFFERSIZE];         
    strcpy(argumentServer,arrayS[0]);

    if(*argumentServer == 'a')//check if correct argument was used
    {
        if(arrayS[1] != NULL)//check if array is not empty, because atoi crashes when empty
        {
            if(atoi(arrayS[1]) != 0)//check for integer
            {
                ackRetries = 0;
                MissingAckPack = atoi(arrayS[1]);
                successS++;
            }
        }
    }
    
    if(successS == 0)
    {
        printf("Wrong program call. Use -a=[Sequenznumber] to trigger a Missing Acknowledgement\n");
        printf("[Sequenznumber] has to be an Integer value > 0\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}




