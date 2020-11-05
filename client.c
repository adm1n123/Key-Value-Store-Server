#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "deocde-encode.h"
int main(int argc, char** argv)
{
	int s;
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	//char* status_code=argv[1];
	//char* key=argv[2];
	//char* value=argv[3];
	//printf("%ld",strlen(encode(status_code,key,value)));
	s = getaddrinfo(NULL, "8080", &hints, &result);
	connect(sock_fd, result->ai_addr, result->ai_addrlen);

	char buffer[513];
	while(fgets(buffer, 513, stdin) != NULL){
		//printf("SENDING: %s", buffer);
		//printf("===\n");
		
        char *split = strtok(buffer," ");
		char *status_code=split;
		split=strtok(NULL," ");
		char *key=split;
		split=strtok(NULL," ");
		char *value=NULL;
		if(split!=NULL)
		{
		//split=strtok(NULL," ");
		value=split;
		value[strlen(value)-1]='\0';
		}
		else
		{
			key[strlen(key)-1]='\0';
		}
		
		
		//printf("%s\n",status_code);
		//printf("%s\n",key);
		//if(value!=NULL)
		//printf("%s\n",value);
		//char *en_mess=encode(status_code,key,value);
		//for(int i=0;i<=512;i++)
		//	printf("%c",en_mess[i]);
		//char *split1=encode(status_code,key,value);

		//decoded_message dec_mess;
		//decode(encode(status_code,key,value),&dec_mess);
		//printf("%s\n",dec_mess.key);
		//if(dec_mess.value[0]!='\0')
		//printf("%s\n",dec_mess.value);
		//printf("%d\n",dec_mess.status_code);
		write(sock_fd, encode(status_code,key,value),513);
		//char resp[1000];
		//int len = read(sock_fd, resp, 999);
		//resp[len] = '\0';
		//printf("%s\n", resp);
	}
    	return 0;
}
