#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "read_config.c"
#include "deocde-encode.h"
#include "kv_cache.c"

SERVER_CONF S;

typedef struct epoll_context{
    int efd;
    struct epoll_event event;
    struct epoll_event* events;
}epoll_context;


int turn=0;
epoll_context *epoll_data;
int sfd;

void *monitor(void *Id)
{   
    int *worker_id=(int *)Id;
    while (1)
    {
        int n, i;
        n = epoll_wait (epoll_data[*worker_id].efd, epoll_data[*worker_id].events, 64, -1);
        //printf("checkpoint\n");
        for (i = 0; i < n; i++)
        {
            if(sfd != epoll_data[*worker_id].events[i].data.fd)
            {  
                int done = 0;
                int len;
                char buf[513];

                len = read(epoll_data[*worker_id].events[i].data.fd, buf, sizeof(buf));
                if(len==0)
                {
                  close(epoll_data[*worker_id].events[i].data.fd);
                  break;
                }
                decoded_message dec_mess;
                decode(buf,&dec_mess);
                printf("%s\n",buf);

                int request = buf[0];
                int status;
                struct kv pair;
                memcopy(&pair, &(buf[1]), MAX_KEY_LEN + MAX_VAL_LEN);

                switch(request){
                	case 1: status = get(&pair);
                		break;
                	case 2: status = put(&pair);
                		break;
                	case 3: status = del(&pair);
                		break;
                	default: status = ERROR;
                }
                //printf("%d\n",dec_mess.status_code);
                //printf("%s\n",dec_mess.key);
                //printf("%s\n",dec_mess.value);
            }
        }
    }
}



void make_non_block(int sfd)
{
    int flags;
    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
        printf ("Non blocking socket failed\n");
        exit(0);
    }

    flags |= O_NONBLOCK;
    if (fcntl (sfd, F_SETFL, flags) == -1)
    {
        printf("Non blocking failed\n");
        exit(0);
    }

}


int main(int argc, char** argv)
{   
    read_conf(&S);
    //printf("%d %d",S.port,S.num_threads);
	kv_cache_init(S->CACHE_SIZE, S->POLICY);
    pthread_t workers[S.num_threads];
    epoll_data=(epoll_context *)malloc(S.num_threads*sizeof(epoll_context));
    int s,opt=1;
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1)
    {
        printf("Socket creation failed\n");
        exit(0);
    }
    struct sockaddr_in address;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))) 
    { 
        printf("Reusing of socket failed\n");
        exit(0);
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( S.port ); 
    make_non_block(sfd);    
    if (bind(sfd, (struct sockaddr *)&address,sizeof(address))<0) 
    { 
        printf("Bind failed\n"); 
        exit(0); 
    } 
 
    if(listen(sfd, 10) == -1) 
    {
		printf("Error in socket listen\n");
		close(sfd);
		exit(0);
	}
    
    //printf("checkpoint\n");
    for(int i=0;i<S.num_threads;i++)
    {
        epoll_data[i].efd=epoll_create1(0);
        epoll_data[i].event.data.fd=sfd;
        epoll_data[i].event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_data[i].efd,EPOLL_CTL_ADD,sfd,&epoll_data[i].event);
        epoll_data[i].events=calloc(64,sizeof(epoll_data[i].event));
    }

    for(int i=0;i<S.num_threads;i++)
    {
      int *arg = malloc(sizeof(*arg));
    	*arg=i;
      pthread_create(&workers[i],NULL,&monitor,arg);
    }

  int sefd;
  sefd = epoll_create1(0);
  struct epoll_event event;
  struct epoll_event *events;

  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (sefd, EPOLL_CTL_ADD, sfd, &event);
  events = calloc (64, sizeof event);
  //printf("checkpoint\n");
  while (1)
  {
    int n, i;

    n = epoll_wait (sefd, events, 64, -1);
    //printf("checkpoint\n");
    for (i = 0; i < n; i++)
    {

      if (sfd == events[i].data.fd)
      {//printf("checkpoint\n");
        while (1)
        {
          struct sockaddr in_addr;
          socklen_t in_len;
          int infd;
          char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

          in_len = sizeof in_addr;
          infd = accept (sfd, &in_addr, &in_len);
          //printf("%d\n",infd);
          if (infd == -1)
          {
            break;    
          }
          make_non_block(infd);
          epoll_data[turn].event.data.fd = infd;
          epoll_data[turn].event.events = EPOLLIN | EPOLLET;
          epoll_ctl (epoll_data[turn].efd, EPOLL_CTL_ADD, infd, &epoll_data[turn].event);
          turn = (turn+1)%S.num_threads;

        }
      }  
    }
  }
  
  return 0;
}