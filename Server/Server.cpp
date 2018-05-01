#include "Server.h"

HANDLE threads[MAX];
HANDLE mutex = CreateMutex(0, 0, 0);

ClientList *clients = (ClientList*)malloc(sizeof(ClientList));
ClientList *clientT = clients;
int t = 2, handles = 0; //thread and handle counter
bool admin_typing = false;


int main(int argc, char* argv[])
{
	int status = -1;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	WSADATA wsaData;
	clients->next = NULL;
	
	if((status = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup failed with error: %d\n", status);
		return -1;
	}

	//Initialize Server Socket
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo("Flet-Desktop", DEFAULT_PORT, &hints, &result);
	if (status != 0)
	{
		printf("getaddrinfo failed: %d\n", status);
		WSACleanup();
		Sleep(5000);
		return -1;
	}

	SOCKET tcpSocket = INVALID_SOCKET, clientSocket = INVALID_SOCKET;
	ptr = result;

	tcpSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (tcpSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		Sleep(500);
		return -1;
	}

	if((status = bind(tcpSocket, result->ai_addr, (int)result->ai_addrlen)) == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(tcpSocket);
		WSACleanup();
		Sleep(500);
		return -1;
	}

	freeaddrinfo(result);

	if ((status = listen(tcpSocket, SOMAXCONN)) == SOCKET_ERROR)
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(tcpSocket);
		WSACleanup();
		Sleep(500);
		return -1;
	}

	threads[0] = (HANDLE)_beginthread(update, 0, NULL);
	threads[1] = (HANDLE)_beginthread(master, 0, NULL);

	//Continually search for connections
	printf("Searching for connections...\n");
	while(1)
	{
		clientSocket = accept(tcpSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			Sleep(5000);
			//return -1;
		}

		else
		{
			if (t >= MAX)
			{
				if (t%MAX == 0)
				{
					WaitForSingleObject(mutex, INFINITE);
					t+=2;
					ReleaseMutex(mutex);
				}
				WaitForSingleObject(threads[t%MAX], INFINITE);
			}
			
			threads[t%MAX] = (HANDLE)_beginthread(service, 0, &clientSocket);
			printf("New Connection Found. New client handled by thread [%d]\n", GetThreadId(threads[t%MAX]));
			t++;
		}
	}

	return 0;
}

//Service thread spawned for every client connection (Undefined behavior above 1024 connections)
void service(LPVOID param)
{
	SOCKET* new_sock = (SOCKET*)param;
	SOCKET client = *new_sock;

	char sendBuff[BUFFER_S], recvBuff[BUFFER_S], name[24];
	int status = recv(client, recvBuff, BUFFER_S, 0);
	sprintf_s(sendBuff, BUFFER_S, "%s has joined.\n", recvBuff);

	WaitForSingleObject(mutex, INFINITE);
		handles++;
		clients->next = (ClientList*)malloc(sizeof(ClientList));
		clients->next->client = (int)client;
		clients->next->id = (int)GetCurrentThreadId();
		strcpy_s(clients->next->name, 24, recvBuff);
		strcpy_s(name, 24, recvBuff);
		clients->next->next = NULL;
		clients = clients->next;
		broadcast(sendBuff, SERVER);
	ReleaseMutex(mutex);
	report(client);

	while (status != 0)
	{
		status = recv(client, recvBuff, BUFFER_S, 0);
		if (status == 0)
		{
			printf("[%d] Client has terminated connection. Closing thread...\n\n", GetCurrentThreadId());
			break;
		}

		else if (status == SOCKET_ERROR)
		{
			printf("[%d] Receive Error (%d) Closing connection...\n", GetCurrentThreadId(), WSAGetLastError());
			break;
		}

		else
		{
			recvBuff[strlen(recvBuff) - 1] = '\0';
			printf("Message From Client [%d]:\n\"%s\"\n\n", GetCurrentThreadId(), recvBuff);

			if (strcmp(recvBuff, "/help") == 0)
			{
				help(client);
			}

			else if(strcmp(recvBuff, "/close") == 0)
			{
				printf("[%d] Close Command received. Ending connection...\n\n", GetCurrentThreadId());
				strcpy_s(sendBuff, BUFFER_S, "[Server] Thank You!");
				send(client, sendBuff, strlen(sendBuff) + 1, 0);
				break;
			}
			
			else if (strcmp(recvBuff, "/status") == 0)
			{
				WaitForSingleObject(mutex, INFINITE);
					report(client);
				ReleaseMutex(mutex);
			}
			
			else if (strstr(recvBuff, "/pm") != NULL)
			{
				//Two Cases: (1)Client requests pm or (2)Client sends message that happens to contain command
				char* str = recvBuff;
				char* str2 = strstr(recvBuff, "/pm");
				//1
				if (str - str2 == 0)
				{
					str += (sizeof(char) * strlen("/pm "));
					str2 = str;
					str2 = strstr(str, " ");
					str[str2 - str] = '\0';
					str2 += 1;
					privateMsg(client, str, str2);
				}
				//2
				else
				{
					WaitForSingleObject(mutex, INFINITE);
						broadcast(recvBuff, CLIENT);
					ReleaseMutex(mutex);
				}
			}

			else if (strcmp(recvBuff, "/kill") == 0)
			{
				strcpy_s(sendBuff, BUFFER_S, "[Server] Kill Request Confirmed\n");
				send(client, sendBuff, strlen(sendBuff) + 1, 0);
				exit(1);
			}

			else
			{
				WaitForSingleObject(mutex, INFINITE);
					broadcast(recvBuff, CLIENT);
				ReleaseMutex(mutex);
			}
		}
	}
	
	WaitForSingleObject(mutex, INFINITE);
		handles--;
		if(!remove_tid(GetCurrentThreadId()))
		{printf("[%d] Could not remove thread ID\n", GetCurrentThreadId());}

		sprintf_s(sendBuff, BUFFER_S, "%s has left.\n", name);
		broadcast(sendBuff, SERVER);
	ReleaseMutex(mutex);
	
	closesocket(client);
	_endthread();
}

//Infinite-Loop detached thread providing updates on client status
void update(LPVOID param)
{
	ClientList *temp;
	while(true)
	{
		Sleep(5000);
		while (admin_typing)
		{Sleep(3000);}
		
		WaitForSingleObject(mutex, INFINITE);
			temp = clientT;
			printf("Current number of connected clients: %d\n", handles);
			printf("Active threads: ");
			while (clientT->next != NULL)
			{
				printf("[%d] ", clientT->next->id);
				clientT = clientT->next;
			}

			printf("\n\n");
			clientT = temp;
			if (clientT->next == NULL)
			{clients = clientT;}
		ReleaseMutex(mutex);
	}

	//Shouldn't be reached so long as server is running
	_endthread();
}

//Thread dedicated to server admin; allows administrative activities (ex. pm, kick)
void master(LPVOID param)
{
	char cmd[BUFFER_S];
	char *msg;

	while(true)
	{
		fgets(cmd, BUFFER_S, stdin);
		admin_typing = true;
		printf("Ready for command. Type '/help' for command list\n");

		fgets(cmd, BUFFER_S, stdin);

		if (strstr(cmd, "/kick "))
		{
			int bye;
			msg = cmd + strlen("/kick ");
			int tgt = atoi(msg);
			WaitForSingleObject(mutex, INFINITE);
			{
				if (!(bye = remove_tid(tgt)))
				{printf("Target ID not found.\n");}
				else
				{
					for (int i = 2; i < t; i++)
					{
						if (GetThreadId(threads[i]) == tgt)
						{
							sprintf_s(cmd, BUFFER_S, "[Admin] YOU HAVE BEEN KICKED FROM FLETCHAT");
							send(bye, cmd, strlen(cmd) + 1, 0);
							Sleep(10);
							handles--;
							closesocket(bye);
							TerminateThread(threads[i], 12);
							break;
						}
					}

					printf("Target Kicked\n");
					sprintf_s(cmd, BUFFER_S, "ID [%d] has been kicked.", tgt);
					broadcast(cmd, ADMIN);
					
				}
			}
			ReleaseMutex(mutex);
		}

		else if (strstr(cmd, "/pm "))
		{
			//TODO: private message from server
		}

		else if (strstr(cmd, "/broadcast "))
		{
			WaitForSingleObject(mutex, INFINITE);
			{
				msg = cmd + strlen("/broadcast")+1;
				broadcast(msg, ADMIN);
			}
			ReleaseMutex(mutex);
		}

		else if (strcmp(cmd, "/close\n") == 0)
		{
			WSACleanup();
			exit(EXIT_SUCCESS);
		}

		else if (strcmp(cmd, "/help\n") == 0)
		{
			printf("\n/kick [Thread ID]\t\tKick client from server\n");
			printf("/pm [Thread ID] [Message]\tSend private message to client (Coming Soon)\n");
			printf("/broadcast [Message]\t\tSend message to all currently connected clients\n");
			printf("/close\t\t\t\tClose Server and all connections\n\n");
		}
		
		else
		{
			printf("cmd not recognized\n");
		}

		admin_typing = false;
		cmd[0] = '\0';
	}

	return;
}

//Search for and remove a thread ID from tID Linked List
int remove_tid(int id)
{
	//TODO: change temp to clientT->next
	ClientList *temp = clientT;
	char name[24], buffer[BUFFER_S];
	int sock = 0;
	while (clientT->next != NULL)
	{
		if (clientT->next->id == id)
		{
			ClientList *tgt = clientT->next;
			clientT->next = tgt->next;
			strcpy_s(name, strlen(tgt->name) + 1, tgt->name);
			sock = tgt->client;
			free(tgt);
			clientT = temp;

			return sock;
		}
		clientT = clientT->next;
	}

	clientT = temp;

	return 0;
}

//Send message to all currently connected clients
int broadcast(char* msg, char src)
{
	char buffer[BUFFER_S];
	ClientList *temp = clientT;

	if (src == CLIENT)
	{
		while (clientT->next->id != GetCurrentThreadId())
		{clientT = clientT->next;}
		sprintf_s(buffer, BUFFER_S, "[%s] %s", clientT->next->name, msg);
	}

	else if(src == SERVER)
	{sprintf_s(buffer, BUFFER_S, "[Server] %s", msg);}

	else if (src == ADMIN)
	{sprintf_s(buffer, BUFFER_S, "[Admin] %s", msg);}

	clientT = temp;
	while (clientT->next != NULL)
	{
		if(clientT->next->id == GetCurrentThreadId())
		{
			clientT = clientT->next;
			continue;
		}

		send(clientT->next->client, buffer, strlen(buffer) + 1, 0);
		clientT = clientT->next;
	}
	clientT = temp;
	printf("Message Broadcasted\n\n");
	return 1;
}

//Send to requesting client session info: Current User Count & Usernames
void report(SOCKET client)
{
	char buffer[BUFFER_S];
	ClientList *temp = clientT;
	sprintf_s(buffer, BUFFER_S, "Current User Count: %d\n", handles);
	send(client, buffer, strlen(buffer) + 1, 0);

	strcpy_s(buffer, BUFFER_S, "Currently Online:");
	while (clientT->next != NULL && strlen(buffer) < BUFFER_S)
	{
		strcat_s(buffer, BUFFER_S, " \'");
		strcat_s(buffer, BUFFER_S, clientT->next->name);
		strcat_s(buffer, BUFFER_S, "\' ");
		clientT = clientT->next;
	}
	clientT = temp;

	send(client, buffer, strlen(buffer) + 1, 0);

	return;
}

//Send to requesting client a list of all available commands
void help(SOCKET client)
{
	char buffer[BUFFER_S];
	const char *close = "/close\t\t\tLeave Fletchat Server";
	const char *report = "/status\t\t\tGet List of All Current Users";
	const char *pm = "/pm [Name] [Msg]\tSend Private Message to Specified User";
	const char *kill = "/kill\t\t\tSignal Server to Exit (Ends ALL Connections)";
	sprintf_s(buffer, BUFFER_S, "\n%s\n%s\n%s\n%s\n\n", close, report, pm, kill);
	send(client, buffer, strlen(buffer) + 1, 0);
	return;
}

//Send private message to client given a UserName
int privateMsg(SOCKET client, char *name, char *msg)
{
	SOCKET tgt = -1;
	ClientList *temp;
	char buffer[BUFFER_S];
	char *src = NULL;

	int id = GetCurrentThreadId();
	
	WaitForSingleObject(mutex, INFINITE);
	{
		temp = clientT;
		while (clientT->next != NULL && (src == NULL || tgt == -1))
		{
			if (strcmp(name, clientT->next->name) == 0)
			{tgt = clientT->next->client;}
			if (clientT->next->id == id)
			{src = clientT->next->name;}
			clientT = clientT->next;
		}

		if (tgt == -1)
		{
			sprintf_s(buffer, BUFFER_S, "[Server] Name '%s' not found.", name);
			send(client, buffer, strlen(buffer) + 1, 0);
			clientT = temp;
			ReleaseMutex(mutex);
			return -1;
		}
		clientT = temp;
	}
	ReleaseMutex(mutex);

	sprintf_s(buffer, BUFFER_S, "[%s] (Private): %s", src, msg);
	send(tgt, buffer, strlen(buffer) + 1, 0);

	return 1;
}

//Get client name given a thread ID
char* getName(int id)
{
	char *name = NULL;
	ClientList *temp;
	WaitForSingleObject(mutex, INFINITE);
	{temp = clientT;}
	ReleaseMutex(mutex);

	while (temp != NULL)
	{
		if (temp->id == id)
		{
			name = temp->name;
			break;
		}
	}

	return name;
}