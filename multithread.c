#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
struct userinfo{

	char* name;
	int connfd;	
	int index;
};

typedef struct userinfo* Userinfo;

// We Will Maintain an array of Active Users

long MAX = 1024;

Userinfo users[1024];

int total_active_users = 0;

clock_t start;

pthread_mutex_t lock_var = PTHREAD_MUTEX_INITIALIZER;


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

void* handleRequest(void* arg)
{
	pthread_detach(pthread_self());

	printf("Request Handled\n");
	int connfd = *((int *) arg);
	printf("%d\n",connfd);
	int current_index;
	// Send The Data Here
 	pthread_mutex_lock(&lock_var);

 	// Get The Join Request Here First

 	char buf[1024];
 	char name[1024];

	int c = recv(connfd,buf,MAX,0);

	buf[c] = '\0';
	
	// 	printf("%s\n",buf);

	char* parsed[MAX];

	parse_with_Spaces(buf,parsed);

	if(strcmp(parsed[0],"JOIN")!=0)
	{
		printf("Please Join The Server First\n");
		return NULL;
	}

	strcpy(name,parsed[1]);

    users[total_active_users] = (Userinfo)malloc(sizeof(struct userinfo));

    users[total_active_users]->connfd = connfd;

    users[total_active_users]->index = total_active_users;

    current_index = total_active_users;

    users[total_active_users]->name = (char*)malloc(sizeof(char)*MAX);

    strcpy(users[total_active_users]->name,name);

    total_active_users++;

    printf("Time Taken between accept and JOIN Message Recieve = %ld\n",(clock()-start));

    pthread_mutex_unlock(&lock_var);


	while(1)
	{
		char buff[1024];
		int c = recv (connfd, buff, MAX,0);

		printf("%s\n",buff);

		if(c>0)
		{
			buff[c]='\0';

			 char* parsed[MAX];

			char temp[MAX];
			strcpy(temp,buff);
			int num_words_in_message = parse_with_Spaces(temp,parsed);

			if(strcmp(buff,"LIST")==0)
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


				send(connfd,message,MAX,0);
			}

			else if(strcmp(parsed[0],"UMSG")==0)
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
						send(users[i]->connfd,message,MAX,0);
						flag = 1;
						break;
					}
				}

				if(!flag)
				{
					// User is Not Online 
					send(connfd,"ERROR <Not Online>",MAX,0);
				}

			}

			else if(strcmp(parsed[0],"BMSG")==0)
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
						send(users[i]->connfd,message,MAX,0);
					// Send The Message to all The Users
					//send(users[i]->connfd,buff,MAX,0);
				}
			}

			else if(strcmp(buff,"LEAV")==0)
			{
				// Delete the entry of user and break;
				users[current_index] = NULL;
				return NULL;
			}

			else
				printf("%s\n",buff);
		}


	}
 	

	// Parse The Message Here 

	// End


	return NULL;
}

int SERV_PORT = 8080;
int LISTENQ = 500;

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

	for(;;)
	{
		clilen = sizeof(cliaddr);

		connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);

		int* arg = malloc(sizeof(int));
		*arg = connfd;

		pthread_t tid;

		start = clock();
		// Create a new Thread Here
		pthread_create(&tid,NULL,&handleRequest,arg);

		//int j == (int) ((void *) j);

	}

	return 0;
}

