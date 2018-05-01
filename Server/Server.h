#pragma once
/*Header Files*/
#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

/*Macros*/
#define DEFAULT_PORT "27015"
#define MAX 1024 //maximum number of threads
#define BUFFER_S 1024 //max length of socket stream buffer

//just to help differentiate sources
#define SERVER 's'
#define CLIENT 'c'
#define ADMIN 'a'


/*Struct Declarations*/
//Linked List storage of client metadata (File Descriptor, Thread ID, Username)
typedef struct ClientList
{
	int client;
	int id;
	char name[24];
	struct ClientList *next;
}ClientList;

//----------------------------------
/*Function Prototypes*/
//Service thread spawned for every client connection (Undefined behavior above 1024 connections)
void service(LPVOID);

//Thread dedicated to server admin; allows administrative activities (ex. pm, kick)
//NOTE: press enter before typing any commands
void master(LPVOID);

//Infinite-Loop detached thread providing updates on client status
void update(LPVOID);

//Search for and remove a thread ID from tID Linked List. Found in service()
int remove_tid(int);

//Send message to all currently connected clients. Found in service()
int broadcast(char*, char);

//Send to requesting client session info: Current User Count & Usernames
void report(SOCKET);

//Send to requesting client a list of all available commands
void help(SOCKET);

//Send private message to client given a UserName
int privateMsg(SOCKET, char*, char*);

//Get client name given a thread ID
char* getName(int);