#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#define QUEUE_LEN 20
#define MAX_FDS 200
#define MAX 1024

int SERV_PORT = 8000;
int LISTENQ = 500;
clock_t start[MAX];
clock_t end[MAX];
double cpu_time;
int total_active_users = 0;
struct user{

	char* name;
	int fd;

};

typedef struct user* User;

User users[MAX];


int parse_with_Spaces(char* str, char** parsed) 
{ 
	int i; 

	for (i = 0; i < MAX; i++) { 
		parsed[i] = strsep(&str, " "); 

		if (parsed[i] == NULL) 
			{
				break; 
			}
		if (strlen(parsed[i]) == 0) 
			i--; 
	} 
	return i;
} 

int main(int argc,char** argv)
{
	int listenfd;
	int connfd;

	//pthread_t tid;

	socklen_t clilen;

	struct sockaddr_in cliaddr;
	struct sockaddr_in servaddr;

	listenfd = socket(AF_INET,SOCK_STREAM,0);

	bzero(&servaddr,sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));

	listen(listenfd,LISTENQ);

	printf("Server Is up and Running\n");

	int epfd = epoll_create(QUEUE_LEN);

	struct epoll_event listen_event;
	listen_event.data.fd = listenfd;
	listen_event.events = EPOLLIN;

	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &listen_event);

	struct epoll_event* events = malloc(MAX_FDS*sizeof(struct epoll_event));



	for(;;)
	{
		int nready = epoll_wait(epfd, events, MAX_FDS, -1);

		for(int j=0;j<nready;j++)
		{
			if(events[j].data.fd == listenfd)
			{
				// Accept the Connections Here
				// Add to the epoll interest List
				int clilen = sizeof(cliaddr);

				int sockfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);
				start[sockfd] = clock();
				// Setting the socket as non BLocking 
				fcntl(sockfd,F_SETFL,O_NONBLOCK);

				// Maintain A List of Online Users Only After Join 

				printf("Client Handled at %d\n",sockfd);

				struct epoll_event event;
				event.data.fd = sockfd;
				event.events = EPOLLIN | EPOLLOUT;

				epoll_ctl(epfd, EPOLL_CTL_ADD,sockfd, &event);

			}

			else
			{
				// A Client Socket is ready

				int fd = events[j].data.fd;

				// Read From This fd the data

				char buf[1024];
 				char name[1024];

				int c = recv(fd,buf,MAX,MSG_DONTWAIT);

				if(c==0)
					continue;

				if(c==-1)
					continue;

				buf[c] = '\0';

				char* parsed[MAX];

				int num_words_in_message = parse_with_Spaces(buf,parsed);

				if(strcmp(parsed[0],"JOIN")==0)
				{
					// Add to User List Here
					strcpy(name,parsed[1]);
					users[total_active_users] = (User)malloc(sizeof(struct user));
					users[total_active_users]->fd = fd;
					users[total_active_users]->name = (char*)malloc(sizeof(char)*MAX);
					strcpy(users[total_active_users]->name,name);
					total_active_users++;
					printf("Time Taken between accept and JOIN Message Receive = %ld\n",(clock()-start[fd])/1000);
					continue;
				}

				if(strcmp(parsed[0],"LIST")==0)
				{
					char* message = (char*)malloc(sizeof(char)* MAX);
					strcpy(message,"\nOnline Users\n");

				//printf("\nOnline Users\n");
					for(int i=0;i<total_active_users;i++)
					{
						if(users[i]!=NULL)
						{
							strcat(message,users[i]->name);
							strcat(message,"\n");
						}
					}

				send(fd,message,MAX,0);
				continue;

				}

				if(strcmp(parsed[0],"UMSG")==0)
				{
					int flag = 0;

					for(int i=0;i<total_active_users;i++)
					{
						if(users[i]==NULL)
							continue;

						if(strcmp(users[i]->name,parsed[1])==0)
						{
							// Send The Message to this specific user
							// Modify Buff Value 
							char* message = (char*)malloc(sizeof(char)*MAX);
							strcpy(message,"");
							strcat(message,parsed[num_words_in_message-1]);
							strcat(message," -");
							for(int i=2;i<num_words_in_message-2;i++)
							{
								strcat(message," ");
								strcat(message,parsed[i]);
							}
							send(users[i]->fd,message,MAX,0);
							flag = 1;
							break;
						}
					}

				if(!flag)
				{
					// User is Not Online 
					send(fd,"ERROR <Not Online>",MAX,0);
				}


					continue;
				}

				if(strcmp(parsed[0],"BMSG")==0)
				{
					for(int i=0;i<total_active_users;i++)
					{
						if(users[i]==NULL)
							continue;

							char* message = (char*)malloc(sizeof(char)*MAX);
							strcpy(message,"");
							strcat(message,parsed[num_words_in_message-1]);
							strcat(message," -");
							for(int i=1;i<num_words_in_message-2;i++)
							{
								strcat(message," ");
								strcat(message,parsed[i]);
							}
							send(users[i]->fd,message,MAX,0);
						// Send The Message to all The Users
					}
					continue;
				}

				if(strcmp(parsed[0],"LEAV")==0)
				{
					// Null The Corrseponding User, Also Do epoll_ctl
					for(int i=0;i<total_active_users;i++)
					{
						if(users[i]==NULL)
							continue;

						if(fd==users[i]->fd)
						{
							// NULL This User
							users[i] = NULL;
							epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						}
					}
					continue;
				}


			}
		}


	}



	return 0;
}
