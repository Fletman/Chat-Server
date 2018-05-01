#pragma once
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define DEFAULT_PORT 27015
#define BUFFER_S 1024

/*Function Prototypes*/
//Thread to handle messages sent to server
void* toward(void*);

//Thread to handle messages sent from server
void* from(void*);