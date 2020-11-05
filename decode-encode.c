#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#define MESSAGE_SIZE 513
char* encode(char *status_code,char key[256],char val[256])
{
    char *en_message=(char*)malloc(MESSAGE_SIZE * sizeof(char) + 1);
    //int size = 8;    //size of string
    char *pad  = "#";     //one ASCII zero
    strcpy(en_message, status_code);
    strcat(en_message,key);
    for(int i=strlen(key)+1;i<=256;i++)
    {
        en_message[i]='\0';
    }
    if(val!=NULL)
    {
        for(int i=0;i<strlen(val);i++)
        {
            en_message[i+257]=val[i];
            //printf("%c\n",en_message[i+257]);
        }
        
        for(int i=257+strlen(val);i<513;i++)
        {
            en_message[i]='\0';
        }
    }
    else
    {
        for(int i=strlen(en_message);i<=513;i++)
        {
            en_message[i]='\0';
        }
    }
    
    /*
    while (strlen(en_message) < 257)
    {
        strcat(en_message, pad);
 
    }
    if (val!=NULL)
    {
        strcat(en_message,val);
    }
    while (strlen(en_message) < MESSAGE_SIZE)
    {
        strcat(en_message, pad);
 
    }*/

    return en_message;

}

typedef struct decoded_message
{
    char key[256];
    char value[256];
    int status_code;
}decoded_message;


void decode(char message[513],decoded_message *dec_mess)
{
    dec_mess->status_code=message[0];
    dec_mess->key[0]='\0';
    dec_mess->value[0]='\0';
    for(int i=1;i<=256;i++)
    {
        if(message[i]=='\0')
        {
            break;
        }
        strncat(dec_mess->key,(message+i),1);
    }
    for(int i=257;i<513;i++)
    {
        if(message[i]=='\0')
        {
            break;
        }
        strncat(dec_mess->value,(message+i),1);
    }
}