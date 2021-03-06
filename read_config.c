#include<stdio.h>
#include<string.h>

typedef struct SERVER_CONF{
	int PORT;
	int NUM_THREADS;
	int CACHE_SIZE;
	int POLICY;
	int NUM_FILES;
}SERVER_CONF;



void read_conf(SERVER_CONF *S1)
{
    
    char line[256];
    FILE *file;
    file=fopen("variables.conf","r");
    while(fgets(line,256,file)!=NULL)
    {   char var[25];
        char value[25];
        if(sscanf(line,"%s %s",var,value) != 2)
        {
            printf("Wrong format in config file");
        }
        if(strcmp(var,"PORT")==0)
        {
            S1->PORT=atoi(value);
        }
        else if(strcmp(var,"NUM_THREADS")==0)
        {
            S1->NUM_THREADS=atoi(value);
        }
        else if(strcmp(var,"CACHE_SIZE")==0)
        {
            S1->CACHE_SIZE=atoi(value);
        }
        else if(strcmp(var,"POLICY")==0)
        {
            if(strcmp(value,"LRU")==0)
                S1->POLICY=5;
            else if(strcmp(value,"LFU")==0)
                S1->POLICY=6;
        }
        else if(strcmp(var,"NUM_FILES")==0)
        {
            S1->NUM_FILES=atoi(value);
        }
    }
}