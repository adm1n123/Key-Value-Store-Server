#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "kv_cache.c"

#include "read_config.c"

int main(int argc, char** argv)
{
	int s;
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	read_conf(&S1);

	kv_cache_init(S1->CACHE_SIZE, S1->POLICY);

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(NULL, "8080", &hints, &result);
	
	if (s != 0){
	        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        	exit(1);
	}

	if ( bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0 ){
		perror("bind()");
		exit(1);
	}

	if ( listen(sock_fd, 10) != 0 ){
		perror("listen()");
		exit(1);
	}

	printf("Waiting for connection...\n");
	int counter = 0;
	char return_buffer[50];
	while(1){
	  int client_fd = accept(sock_fd, NULL, NULL);
	  printf("Connection made: client_fd=%d\n", client_fd);

	  char buffer[1000];
	  int len = read(client_fd, buffer, 999);
	  buffer[len] = '\0';

	  printf("Read %d chars\n", len);
	  printf("===\n");
	  printf("%s\n", buffer);

	  sprintf(return_buffer, "Server response to %dth client connection", counter);
	 
	  write(client_fd, return_buffer, strlen(return_buffer));
	  counter++;
	}
	
    return 0;
}
