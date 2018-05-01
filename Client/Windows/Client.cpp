#include "Client.h"

HANDLE threads[2];
int condition = 0; //global flag to denote end of process

int main(int argc, char* argv[])
{
	char name[24];
	int status = -1;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	WSADATA wsaData;

	if ((status = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup failed with error: %d\n", status);
		Sleep(5000); //just giving user time to read error message
		return -1;
	}

	//intialize socket address & protocol (TCP)
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	status = getaddrinfo("Flet-Desktop", DEFAULT_PORT, &hints, &result);

	if (status != 0)
	{
		printf("Host not found. Exiting...\n");
		WSACleanup();
		Sleep(5000);  //just giving user time to read error message
		return -1;
	}

	SOCKET tcpSocket = INVALID_SOCKET;
	ptr = result;

	tcpSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (tcpSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		Sleep(5000);  //just giving user time to read error message
		return -1;
	}

	printf("Hello and Welcome to Fletchat!\n");

	do
	{
		printf("Enter a Username to get started. (No Spaces)\n");
		fgets(name, NAME_SIZE, stdin);
	}
	while (name[0] == '\n' || name[0] == '\0' || name == NULL || strstr(name, " ") != NULL);

	if(name[strlen(name)-1] == '\n')
	{
		name[strlen(name) - 1] = '\0';
	}

	//Attempt to connect server 3 times
	for (int i = 0; i < 3; i++)
	{
		printf("(%d) Attempting server connection...\n", i + 1);
		status = connect(tcpSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

		if (status == SOCKET_ERROR && i >= 2)
		{
			closesocket(tcpSocket);
			tcpSocket = INVALID_SOCKET;
			printf("Connection Timeout. Exiting.\n");
			Sleep(1000);
			return -1;
		}

		else if (status == SOCKET_ERROR)
		{
			printf("Connection failed. Retrying.\n\n");
			Sleep(1000);  //just giving user time to read error message
		}

		else
		{
			break;
		}
	}
	printf("Connection Established!\n");
	send(tcpSocket, name, strlen(name) + 1, 0);

	threads[0] = (HANDLE)_beginthread(toward, 0, &tcpSocket);
	threads[1] = (HANDLE)_beginthread(from, 0, &tcpSocket);

	WaitForMultipleObjects(2, threads, true, INFINITE); //keep process alive until both threads have finished to avoid send/receive errors

	Sleep(1750);
	WSACleanup();
	return 0;
}

//Thread to handle messages sent to server
void toward(void* param)
{
	SOCKET tcpSocket = *(SOCKET*)param;
	char sendBuff[BUFFER_S];
	int status;

	printf("Enter a message to send to server. Type '/help' for a list of commands.\n");

	while (strcmp(sendBuff, "/close\n") != 0 && strcmp(sendBuff, "/kill\n") != 0 && condition != -1)
	{
		fgets(sendBuff, BUFFER_S, stdin);
		if(strlen(sendBuff) < 1)
		{continue;}

		for (int i = 0; i < 3; i++)
		{
			status = send(tcpSocket, sendBuff, strlen(sendBuff) + 1, 0);
			if (status > -1)
			{
				break;
			}
			printf("Message not sent. Resending data...\n");
			Sleep(750);
		}
		if (status < 0)
		{
			printf("Lost Connection to Host. Exiting...\n");
			Sleep(1750);
			WSACleanup();
			exit(EXIT_FAILURE);
		}
	}

	condition = -1;

	_endthread();
}

//Thread to handle messages sent from server
void from(void* param)
{
	SOCKET tcpSocket = *(SOCKET*)param;
	char recvBuff[BUFFER_S];
	int status;
	
	while (condition != -1)
	{
		status = recv(tcpSocket, recvBuff, BUFFER_S, 0);
		if (status == SOCKET_ERROR)
		{
			printf("Fatal error receiving from server. Closing...\n");
			Sleep(1750);
			exit(EXIT_FAILURE);
		}

		if (strstr(recvBuff, "\n") != NULL)
		{recvBuff[strlen(recvBuff) - 1] = '\0';}

		printf("%s\n", recvBuff);
		char* place;
		char* token = strtok_s(recvBuff, "] ", &place);
		token = strtok_s(NULL, "] ", &place);

		if (strcmp(token, "/close") == 0)
		{
			printf("Close request received. Exiting client...\n");
			condition = -1;
			Sleep(2000);
			exit(EXIT_SUCCESS);
		}
	}

	_endthread();
}