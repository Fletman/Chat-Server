
#include "Client.h"

pthread_t threads[2];
int condition = 0;

int main(int argc, char* argv[])
{
	char name[24];
	int status = -1;
	int tcpSocket;
	int i;

	tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(tcpSocket == -1)
	{
		printf("Failure creating socket\n");
		return -1;
	}

	struct hostent *host = gethostbyname("Flet-Desktop");
	if(host == NULL)
	{
		printf("Host not found.\n");
		return -1;
	}

	struct sockaddr_in servAddr;
    	servAddr.sin_family = AF_INET;
    	servAddr.sin_port = htons(DEFAULT_PORT);
    	servAddr.sin_addr = *(struct in_addr*)host->h_addr;
    	bzero(&(servAddr.sin_zero), 8);

	printf("Hello and Welcome to Fletchat!\n");

	do
	{
		printf("Enter a Username to get started. (No Spaces)\n");
		fgets(name, 24, stdin);
	}
	while (name[0] == '\n' || name[0] == '\0' || name == NULL || strstr(name, " ") != NULL);

	if(name[strlen(name)-1] == '\n')
	{
		name[strlen(name) - 1] = '\0';
	}

	//Attempt to connect server 3 times
	for (i = 0; i < 3; i++)
	{
		printf("(%d) Attempting server connection...\n", i + 1);
		status = connect(tcpSocket,(struct sockaddr *)&servAddr,sizeof(struct sockaddr_in));

		if (status == -1 && i >= 2)
		{
			close(tcpSocket);
			printf("Connection Timeout. Exiting.\n");
			usleep(10000);
			return -1;
		}

		else if (status == -1)
		{
			printf("Connection failed. Retrying.\n\n");
			usleep(10000);
		}

		else
		{
			break;
		}
	}
	printf("Connection Established!\n");
	send(tcpSocket, name, strlen(name) + 1, 0);

	pthread_create(&threads[0], NULL, toward, (void*)&tcpSocket);
	pthread_create(&threads[1], NULL, from, (void*)&tcpSocket);

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);

	usleep(17500);
	return 0;
}

//Thread to handle messages sent to server
void* toward(void* param)
{
	int tcpSocket = *(int*)param;
	char sendBuff[BUFFER_S];
	int status, i;

	printf("Enter a message to send to server. Type '/help' for a list of commands.\n");

	while (strcmp(sendBuff, "/close\n") != 0 && strcmp(sendBuff, "/kill\n") != 0 && condition != -1)
	{
		fgets(sendBuff, BUFFER_S, stdin);

		for (i = 0; i < 3; i++)
		{
			status = send(tcpSocket, sendBuff, strlen(sendBuff) + 1, 0);
			if (status > -1)
			{
				break;
			}
			printf("Message not sent. Resending data...\n");
			usleep(7500);
		}
		if (status < 0)
		{
			printf("Lost Connection to Host. Exiting...\n");
			usleep(17500);
			exit(-1);
		}
	}

	condition = -1;
	pthread_exit(NULL);
}

//Thread to handle messages sent from server
void* from(void* param)
{
	int tcpSocket = *(int*)param;
	char recvBuff[BUFFER_S];
	int status;
	char* token;
	
	while (condition != -1)
	{
		status = recv(tcpSocket, recvBuff, BUFFER_S, 0);
		if (status == -1)
		{
			printf("Fatal error receiving from server. Closing...\n");
			usleep(17500);
			exit(-1);
		}

		if (strstr(recvBuff, "\n") != NULL)
		{recvBuff[strlen(recvBuff) - 1] = '\0';}

		printf("%s\n", recvBuff);
		
		token = strtok(recvBuff, "] ");
		token = strtok(NULL, "] ");

		if (strcmp(token, "/close") == 0)
		{
			printf("Close request received. Exiting client...\n");
			condition = -1;
			usleep(20000);
			exit(1);
		}
	}

	pthread_exit(NULL);
}