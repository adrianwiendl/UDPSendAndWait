#include <stdio.h>
#include <windows.h>
#include <string.h>

// int seqRetries = 1;
// int csmRetries = 1;
// int ackRetries = 1;
int MissingAckPack =-1;

int menu(char* e1, char* e2, char* e3)
{
    //a= ack, c= checksum, s=sequenz

    //combine arguments to one string

puts(e1);
puts(e2);
puts(e3);
    char errorstr[50];
    sprintf(errorstr, "%s=%s=%s", e1, e2, e3);

    printf("TEST OUTPUT errorstr: %s\n", errorstr);

    //split tring into 6 array entries
    int i = 0;
    char *part = strtok (errorstr, "=, -");
    char *array[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

    while (part != NULL)
    {
        array[i++] = part;
        part = strtok (NULL, "=, -");
    }

    //
    if(atoi(array[1]) != 0) //test for integer
    {
        char *argument1;
        argument1 = array[0];

        switch(*argument1)
        {
        case 'a':
            ackRetries = 0;
           // MissingAckPack = atoi(array[1]);
   // printf("MissingAckPack test out %d\n", MissingAckPack);
    puts("casea");
            break;
        case 'c':
            csmRetries = 0;
    puts("casec");
            break;
        case 's':
            seqRetries = 0;
    puts("cases");
            break;
        case 'l':
    puts("casel1");
            break;
        default:
    puts("def");
            printf("Error wrong argument. -%s is no known argument\n", array[0]); //TODO help message
            return -1;
        }
    }

    //
    if(atoi(array[3]) != 0) //test for integer
    {
        char *argument2;
        argument2 = array[2];

        switch(*argument2)
        {
        case 'a':
            ackRetries = 0;
    puts("casea2");
            break;
        case 'c':
            csmRetries = 0;
    puts("casec2");
            break;
        case 's':
            seqRetries = 0;
    puts("cases2");
            break;
        case 'l':
    puts("casel2");
            break;
        default:
    puts("def2");
            printf("Error wrong argument. -%s is no known argument\n", array[2]); //TODO help message
            return -1;
        }
    }

    if(atoi(array[5]) != 0) //test for integer
    {
        char *argument3;
        argument3 = array[4];

        switch(*argument3)
        {
        case 'a':
            ackRetries = 0;
    puts("casea3");
            break;
        case 'c':
            csmRetries = 0;
    puts("casec3");
            break;
        case 's':
            seqRetries = 0;
    puts("cases3");
            break;
        case 'l':
    puts("casel3");
            break;
        default:
    puts("def3");
            printf("Error wrong argument. -%s is no known argument\n", array[4]); //TODO help message
            return -1;
        }
    }
    return 0;
}