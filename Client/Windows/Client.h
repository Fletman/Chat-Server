#pragma once
#define WIN32_LEAN_AND_MEAN

/*Header Files*/
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
#define NAME_SIZE 24
#define BUFFER_S 1024
#define DEFAULT_PORT "27015"


/*Function Prototypes*/
//Thread to handle messages sent to server
void toward(void*);

//Thread to handle messages sent from server
void from(void*);