#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "decode-encode.c"

int main(int argc, char** argv)
{
	int s;
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	s = getaddrinfo(NULL, "8080", &hints, &result);
	connect(sock_fd, result->ai_addr, result->ai_addrlen);

	char buffer[520];
	char response[513];
	FILE *file;
	file=fopen("testcases.txt","r");
	while(fgets(buffer, 520, file) != NULL){
        char *split = strtok(buffer," ");
		char *status_code=split;
		split=strtok(NULL," ");
		char *key=split;
		split=strtok(NULL," ");
		char *value=NULL;
		if(split!=NULL)
		{
		value=split;
		value[strlen(value)-1]='\0';
		}
		else
		{
			key[strlen(key)-1]='\0';
		}
		//printf("%s\n",status_code);
		write(sock_fd, encode(status_code,key,value),513);
		//printf("below write\n");
		bzero(response, sizeof(response));
		read(sock_fd,response, sizeof(response));
		decoded_message dec_res;
		decode(response,&dec_res); 
		//printf("%s\n",dec_res.key);
		//printf("%s\n",dec_res.value);
		if(dec_res.status_code==240)
		{
			printf("ERROR\n");
		}
		else if(dec_res.status_code==200)
		{
			if(strcmp(status_code,"1")==0)
			{
				printf("GET %s\n",dec_res.value);
			}
			else if(strcmp(status_code,"2")==0)
			{
				printf("PUT\n");
			}
			else if(strcmp(status_code,"3")==0)
			{
				printf("DEL\n");
			}
		}
		//printf("%d\n",dec_res.status_code);

	}
    	return 0;
}
