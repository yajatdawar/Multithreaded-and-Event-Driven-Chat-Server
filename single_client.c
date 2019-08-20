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

int SERV_PORT = 8080;
#define MAX 1024

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

	struct sockaddr_in servaddr;

	//127.0.0.1
	SERV_PORT = atoi(argv[2]);

	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	bzero(&servaddr,sizeof(servaddr));

	servaddr.sin_family = AF_INET;

	servaddr.sin_port = htons(SERV_PORT);

	servaddr.sin_addr.s_addr = inet_addr (argv[1]);

	if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))==0)
		printf("Connection Successful\n");
	else
	{
		printf("\nSorry, Could Not Connect to the Server\n\n");
		exit(0);
	}

	char name[1024];

	printf("\nWelcome To the Chat Application\n");

	printf("\nPlease Enter JOIN <Your Name> to connect to the world\n");

	//scanf("%[^\n]", name);
	fgets(name,MAX,stdin);
	name[strlen(name)-1] = '\0';

	send(sockfd, name,MAX,0);
	// Send A Join Message to Server


	while(1){

		char temp[MAX];

		printf("\nchatapp>>");

		char ch;

		//scanf("%c",&ch);

		//scanf("%[^\n]", temp);

		fgets(temp,MAX,stdin);

		temp[strlen(temp)-1] = '\0';

		char* parsed[MAX];

		char temp1[MAX];

		strcpy(temp1,temp);

		parse_with_Spaces(temp1,parsed);

		if(strcmp(temp,"LIST")==0)
		{
			send(sockfd,temp,sizeof(temp),0);
			sleep(1);
			while(1)
			{
				char buff[1024];
				int c = recv (sockfd, buff,sizeof(buff),MSG_DONTWAIT);

				if(c==-1)
					break;

				printf("%s",buff);

			}
			

		}

		else if(strcmp(temp,"showMyMessages")==0)
		{
			while(1)
			{
				char buff[1024];
				int c = recv (sockfd, buff,MAX,MSG_DONTWAIT);

				if(c==-1)
					break;

				printf("%s\n",buff);

			}
		}


		else if(strcmp(temp,"LEAV")==0)
		{
			send(sockfd,temp,sizeof(temp),0);
			printf("\nGood Bye\n");
			exit(0);
		}


		else if(strcmp(parsed[0],"UMSG")==0 || strcmp(parsed[0],"BMSG")==0)
		{
			strcat(temp," ");
			strcat(temp,name);
			send(sockfd,temp,sizeof(temp),0);
			sleep(1);
			char buff[MAX];
			int c = recv (sockfd, buff,sizeof(buff),MSG_DONTWAIT);

			if(c!=-1)
			{

				if(strcmp(buff,"ERROR <Not Online>")==0)
				{
					printf("\n%s\n",buff);
				}

			}
		}


		else 
		{
			printf("\nThe Format of Message is Not Correct\n");
		}


		

	}


	return 0;
}
